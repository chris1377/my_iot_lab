GIT:
git add .
git commit -m ""
git push


Avvio connessione WIFI del pc:

Creazione:
nmcli device wifi hotspot ssid "cloud" password "password"

# Progetto 1: Web Server IoT Locale

Questo è il mio primo progetto IoT. Consiste in un web server locale ospitato su un Arduino Uno R4 WiFi che permette di monitorare un sensore e controllare un attuatore tramite una dashboard accessibile da browser.

## 🛠️ Hardware Utilizzato
* Scheda: Arduino Uno R4 WiFi
* Sensore: Sensore di luce (es. modulo TEMT6000)
* Attuatore: LED rosso classico
* Componenti extra: Resistenza (220/330 ohm), breadboard, cavi jumper

## 🔌 Schema dei Collegamenti
* **Sensore di luce:** 
  * VCC -> 5V
  * GND -> GND
  * OUT/Signal -> Pin analogico A0
* **LED:**
  * Gamba lunga (Anodo) -> Pin digitale 8
  * Gamba corta (Catodo) -> GND (tramite resistenza)

## 💻 Funzionalità del Software
Lo sketch in C++ utilizza la libreria `<WiFiS3.h>` per:
1. Connettere l'Arduino a una rete Wi-Fi (o hotspot locale).
2. Avviare un server HTTP sulla porta 80.
3. Servire una pagina HTML con stile CSS integrato.
4. Mostrare il valore della luce in tempo reale (con auto-refresh ogni 2 secondi).
5. Intercettare le richieste `GET /ON` e `GET /OFF` generate dai pulsanti web per accendere e spegnere il LED fisico.

---

# Progetto 2: Comunicazione MQTT Bidirezionale

Questo secondo step evolve il progetto passando da una classica pagina web (HTTP) al protocollo MQTT. Questo permette una comunicazione istantanea, asincrona e molto più leggera, tipica dei sistemi IoT professionali.

## 🏗️ Architettura di Rete
* **Broker MQTT (Postino):** Ospitato sul PC Linux (Edge) tramite il software `mosquitto` (IP locale dell'hotspot: 10.42.0.1).
* **Nodo IoT:** Arduino Uno R4 WiFi connesso all'hotspot.

## 📡 Librerie e Funzionamento
Il codice C++ utilizza la libreria `PubSubClient` di Nick O'Leary.
* Abbiamo eliminato la funzione bloccante `delay()`, sostituendola con il timer `millis()`, permettendo all'Arduino di essere costantemente in ascolto.

## 💬 Topic MQTT Utilizzati
* **`laboratorio/sensori` (Publisher):** L'Arduino pubblica automaticamente il valore del sensore di luce ogni 2 secondi.
* **`laboratorio/attuatori/led` (Subscriber):** L'Arduino è "abbonato" a questo canale. Se riceve il testo `ON` accende il LED sul pin 8, se riceve `OFF` lo spegne immediatamente.

## 💻 Comandi Edge (da Terminale Linux)
* Per leggere i sensori: `mosquitto_sub -t "laboratorio/sensori"`
* Per accendere il LED: `mosquitto_pub -t "laboratorio/attuatori/led" -m "ON"`
* Per spegnere il LED: `mosquitto_pub -t "laboratorio/attuatori/led" -m "OFF"`

## 📝 Appunti di Sviluppo e Best Practice

Durante lo sviluppo sono emerse due considerazioni architettoniche fondamentali:

* **Struttura dei Topic (Payload vs Topic):** Esistono due approcci per inviare comandi. 
  * *Basato sul Payload* (usato nel progetto base): un solo topic a cui inviare messaggi di stato.
  * *Basato sul Topic:* creare percorsi specifici per l'azione, ignorando il contenuto del messaggio. È molto utile per scalare i comandi usando l'abbonamento jolly (`#`).
    In C++, l'abbonamento si fa così:
    ```cpp
    mqttClient.subscribe("laboratorio/attuatori/led/#");
    ```
    Da terminale Linux, i comandi (con messaggio vuoto) diventano:
    ```bash
    mosquitto_pub -t "laboratorio/attuatori/led/accendi" -m ""
    mosquitto_pub -t "laboratorio/attuatori/led/spegni" -m ""
    ```
* **Resilienza di Rete (Il Subscribe nel Loop):** L'istruzione `mqttClient.subscribe()` non si trova nel `setup()`, ma all'interno del `loop()` nel blocco di controllo della connessione `if (!mqttClient.connected())`. Questa è una regola d'oro per l'IoT: in caso di caduta momentanea del Wi-Fi o del Broker, al momento della riconnessione l'Arduino rinnova automaticamente i suoi abbonamenti. Se fosse stato nel `setup()`, a seguito di una disconnessione l'Arduino sarebbe diventato cieco ai nuovi comandi fino al riavvio fisico.

 ## 🧠 Automazione Edge (Python)

Per rendere il sistema intelligente e autonomo, abbiamo affiancato ad Arduino uno script in Python in esecuzione sul nodo Edge (il PC Linux). Questo script funge da "Cervello": ascolta costantemente i dati del sensore di luminosità tramite MQTT e prende decisioni automatiche.

**Requisiti:**
Libreria Paho MQTT. Su sistemi basati su Debian/Ubuntu si installa con:
`sudo apt install python3-paho-mqtt`

**Logica di funzionamento (`cervello_edge.py`):**
Lo script si connette al Broker locale (`127.0.0.1`), si abbona al topic `laboratorio/sensori` e decodifica il payload dei messaggi.
Se il valore della luce scende sotto una soglia prestabilita (es. 300), lo script pubblica in automatico il comando `ON` sul topic `laboratorio/attuatori/led` per accendere la luce. Quando il valore torna sopra la soglia, pubblica `OFF`.


## 🌐 Interfaccia Grafica e Logica Avanzata (Node-RED)

L'automazione in Python è stata sostituita e potenziata tramite **Node-RED**, un motore logico visivo che funge sia da "cervello" del sistema che da interfaccia grafica SCADA (Supervisory Control And Data Acquisition).

**Accesso al Sistema:**
* Motore: si avvia da terminale con il comando `node-red`
* Editor Visivo: `http://127.0.0.1:1880`
* Dashboard Web: `http://127.0.0.1:1880/dashboard` (creata tramite il pacchetto `@flowfuse/node-red-dashboard`)

**Logica del Flusso (Macchina a Stati e Override):**
Node-RED gestisce in contemporanea le decisioni automatiche e i comandi manuali, prevenendo conflitti tramite un sistema di priorità:
1. **Automazione:** I valori in arrivo dal topic `laboratorio/sensori` vengono passati in un nodo `switch`. Se il `msg.payload` è `< 300`, viene inviato il comando `ON` al LED, altrimenti `OFF`.
2. **Memoria di Stato:** Quando l'utente aziona l'interruttore dalla Dashboard, il nuovo stato ("ON" o "OFF") viene salvato nella memoria a lungo termine di Node-RED, all'interno della variabile globale `flow.stato_manuale`.
3. **Cancello Logico (Override):** I dati in arrivo dal sensore passano prima attraverso un nodo `switch` di blocco che legge la memoria `flow`. Se la variabile contiene "ON", la regola di sbarramento (`!= ON`) distrugge i dati del sensore. Questo meccanismo garantisce che il comando manuale abbia la priorità assoluta sull'automazione ambientale.


### 📱 Accesso da Smartphone (Rete Locale)

Poiché il server Node-RED e il Broker MQTT sono ospitati sul PC Linux (il nostro nodo Edge), l'interfaccia grafica è accessibile da qualsiasi dispositivo (come uno smartphone o un tablet) connesso alla stessa rete Wi-Fi.
    
1. Trova l'indirizzo IP locale del PC Linux eseguendo nel terminale: 
   `hostname -I`
2. Apri il browser sullo smartphone e digita l'indirizzo usando l'IP appena trovato: 
   `http://<IP_DEL_PC_LINUX>:1880/dashboard`


## 📡 Nodo Sensore Distribuito (ESP8266)

L'architettura è stata espansa verso un vero ecosistema IoT distribuito ("Triangolo IoT") introducendo un microcontrollore **ESP8266 (NodeMCU)** con connettività Wi-Fi nativa.

* **Ruolo:** Agisce come nodo sensore remoto puro. Raccoglie dati ambientali (es. temperatura) e li pubblica sul Broker via Wi-Fi.
* **Topic MQTT:** `laboratorio/esp/temperatura`
* **Logica Distribuita:** L'ESP8266 funziona solo come *Publisher*. Il PC Linux (Node-RED) riceve le letture, popola la Dashboard e gestisce la logica decisionale, mentre l'Arduino (in ascolto come *Subscriber*) si occupa solo di eseguire le azioni fisiche.

*Nota hardware: Il modulo ESP8266 utilizza l'antenna a 2.4 GHz, pertanto l'infrastruttura di rete (router o hotspot) deve supportare questa banda per consentire la connessione.*


