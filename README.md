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



