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

i# Progetti 4 e 5: Da Metronomo Offline a Dispositivo IoT

Questo repository contiene due evoluzioni di un metronomo digitale sviluppato su **Arduino UNO R4 WiFi**. Il progetto parte da una logica hardware basata sul calcolo dei tempi di pressione (Progetto 4) per poi evolversi in un vero e proprio dispositivo IoT connesso in rete e controllabile via web (Progetto 5).

---

## 🛠️ Progetto 4: Metronomo Offline (Solo Scheda)

Il Progetto 4 realizza un metronomo stand-alone completamente funzionale. L'utente interagisce fisicamente con la breadboard per impostare il ritmo tramite il tocco ("Tap Tempo") e il sistema restituisce un feedback visivo lampeggiando a tempo.

### Componenti e Cablaggio
*   **Arduino UNO R4 WiFi**
*   **Pulsante Mode (Pin 4):** Alterna tra la modalità di registrazione e quella di riproduzione. Collegato a GND sfruttando la resistenza interna `INPUT_PULLUP`.
*   **Pulsante Tap (Pin 5):** Utilizzato per battere il tempo. Collegato a GND (`INPUT_PULLUP`).
*   **LED Rosso (Pin 9):** Indicatore visivo della modalità "Learn" (Registrazione). Pulsa a ogni tap registrato.
*   **LED Verde (Pin 8):** Indicatore visivo della modalità "Play". Lampeggia a ritmo seguendo i BPM calcolati. (Richiede resistenza da 220Ω/330Ω in serie verso GND).

### Funzionamento e Logica
Il codice gestisce gli input hardware superando il problema del "rimbalzo" (debounce) meccanico tramite il trucco dello *State Change Detection* (rilevamento della variazione di stato) e affidandosi alla funzione `millis()` per operazioni non bloccanti.

1.  **Modalità LEARN:** Entrando in questa modalità, l'utente preme il pulsante Tap a ritmo. Il sistema richiede un **minimo di 4 tocchi validi**. Arduino salva in un array (fino a 10 elementi) i timestamp in millisecondi di ogni pressione.
2.  **Calcolo Matematico:** Passando alla modalità Play, il sistema sottrae il tempo del primo tap dall'ultimo, divide per il numero di intervalli e calcola la media in millisecondi. Da questa estrae i **BPM (Battiti Per Minuto)** dividendo 60.000 per l'intervallo medio.
3.  **Modalità PLAY:** Il LED verde si accende e spegne in modo asincrono (sempre usando `millis()`), garantendo che la scheda sia sempre in ascolto per eventuali nuovi input utente.

---

## 🌐 Progetto 5: Metronomo IoT (API REST)

Il Progetto 5 prende la base hardware ed elettronica del Progetto 4 e la espande integrandola alla rete locale. Grazie al modulo Wi-Fi della UNO R4, il metronomo diventa un Web Server capace di rispondere a richieste HTTP esterne, tracciare le statistiche (BPM minimo e massimo) ed essere controllato da un'interfaccia grafica.

### Prerequisiti
*   Libreria ufficiale: `<WiFiS3.h>`
*   Modifica delle variabili globali nel codice per inserire le credenziali della propria rete (`ssid` e `pass`).
*   Pagina HTML/JS (Dashboard) da avviare in locale per interfacciarsi comodamente con le API.

### Architettura Server e CORS
Il web server su Arduino (in ascolto sulla porta 80) implementa un ciclo di lettura personalizzato che elabora in modo differenziato le richieste GET/DELETE e quelle PUT. Per permettere a una pagina web locale (HTML) di comunicare direttamente con la scheda senza essere bloccata dai sistemi di sicurezza del browser, il server espone intestazioni specifiche:
*   `Access-Control-Allow-Origin: *`
*   Gestione della richiesta preliminare di perlustrazione `OPTIONS`.

### Endpoints API REST Esposti

| Metodo HTTP | Endpoint | Azione Eseguita dal Server Arduino |
| :--- | :--- | :--- |
| **GET** | `/bpm/` | Restituisce il valore del BPM correntemente in riproduzione. |
| **PUT** | `/bpm/` | Riceve un nuovo valore BPM nel corpo della richiesta (Body), aggiorna la velocità di lampeggio del LED verde e aggiorna eventuali record. |
| **GET** | `/bpm/min/` | Restituisce il valore del BPM più basso mai registrato dall'accensione. |
| **DELETE** | `/bpm/min/` | Azzera la variabile globale del record minimo (imposta a 0). |
| **GET** | `/bpm/max/` | Restituisce il valore del BPM più alto mai registrato dall'accensione. |
| **DELETE** | `/bpm/max/` | Azzera la variabile globale del record massimo (imposta a 0). |

# Progetto 6: Metronomo IoT con MQTT e Node-RED Dashboard 2.0 (progetto6_metronomo_mqtt)

## 📖 Descrizione
Questo progetto consiste in un metronomo basato su Arduino, controllabile e monitorabile in tempo reale tramite un'interfaccia web (Dashboard). Il sistema sfrutta il protocollo MQTT per garantire una comunicazione bidirezionale veloce ed efficiente tra l'hardware e l'interfaccia utente costruita su Node-RED.

## 🛠 Architettura del Sistema

### 1. Hardware (Arduino)
L'unità fisica gestisce la generazione del battito (BPM) e le modalità operative:
* **Pulsante MODE:** Permette di passare dalla modalità di esecuzione (PLAY) alla modalità di acquisizione/apprendimento (REC/Learn).
* **LED di Stato:** Feedback visivo fisico sulla breadboard (es. Verde per PLAY, Rosso per REC).
* **Client MQTT:** Invia lo stato attuale e si mette in ascolto dei nuovi BPM in arrivo.

### 2. Software (Node-RED)
L'interfaccia di controllo è stata sviluppata utilizzando **Node-RED Dashboard 2.0** (`@flowfuse/node-red-dashboard`).
* **Input BPM:** Un modulo `ui-form` ottimizzato graficamente per inviare il valore numerico dei BPM desiderati tramite un pulsante dedicato.
* **Monitoraggio:** Testi dinamici (`ui-text`) che mostrano BPM attuali, BPM Minimo e BPM Massimo.
* **Feedback Visivo:** Un indicatore `ui-led` che si accende di rosso quando l'hardware entra in modalità REC.
* **Stile Custom:** Utilizzo del nodo `ui-template` e classi CSS/Vuetify (es. `d-flex`, `justify-center`) per impaginare i componenti in griglie affiancate e personalizzare l'allineamento.

## 📡 Topic MQTT Configurati

Il broker MQTT fa da ponte tra Node-RED e Arduino utilizzando i seguenti topic:

* `metronomo/bpm/set` (Sottoscrizione Arduino / Pubblicazione Node-RED): 
  Utilizzato per inviare il nuovo valore di BPM dalla dashboard all'hardware.
* `metronomo/stato/rec` (Pubblicazione Arduino / Sottoscrizione Node-RED): 
  Invia le stringhe testuali `"ON"` o `"OFF"` quando si preme il pulsante MODE, permettendo al nodo LED sulla dashboard di aggiornarsi in tempo reale.

## 🚀 Requisiti e Dipendenze (Node-RED)
Per far funzionare correttamente il flusso importato su Node-RED, è necessario installare i seguenti pacchetti dalla *Manage Palette*:
1. `@flowfuse/node-red-dashboard` (Dashboard 2.0)
2. `@flowfuse/node-red-dashboard-2-ui-led` (Nodo LED specifico per la nuova interfaccia)

## ⚙️ Layout e Personalizzazione UI
Il progetto include modifiche al layout standard di Node-RED Dashboard 2.0:
* Utilizzo del pannello **Layout** per l'organizzazione in gruppi e la spaziatura manuale (tramite le proporzioni `Size`, es. 3x1, 6x1).
* Modifiche CSS custom per allineare caselle di testo e pulsanti sulla stessa riga e centrare gli elementi chiave per una migliore User Experience.

# 🎵 Proetto7 - Metronomo IoT (Arduino UNO R4 WiFi + Node-RED)

Questo progetto trasforma un **Arduino UNO R4 WiFi** in un metronomo intelligente e connesso. Può essere controllato interamente tramite pulsanti fisici oppure da remoto attraverso una comoda interfaccia web (Dashboard) basata su **Node-RED** via protocollo MQTT.

Il sistema è progettato per essere **resiliente**: funziona perfettamente anche **offline**. Se la rete WiFi cade o non è disponibile, il metronomo entra in modalità standalone, permettendoti di continuare a suonare senza interruzioni.

---

## ✨ Funzionalità Principali

* 📶 **Controllo Bidirezionale:** Sincronizzazione in tempo reale tra hardware fisico e Dashboard Node-RED.
* 🥁 **Tap Tempo (Smart):** Calcola automaticamente i BPM battendo il tempo (minimo 4 tap). Attivabile da pulsante fisico o da web.
* 📴 **Modalità Offline Nativa:** Codice non bloccante. Se non c'è WiFi, il metronomo si avvia offline e cerca la rete in background ogni 5 secondi senza bloccare l'esecuzione.
* 🔊 **Gestione Audio (Mute):** Cicalino piezoelettrico attivabile/disattivabile a piacimento.
* 💡 **Feedback Visivo Ricco:**
  * **LCD 16x2:** Mostra lo stato del WiFi, la modalità (PLAY/REC), lo stato dell'audio e i BPM.
  * **Matrice LED (UNO R4):** Mostra un'icona a forma di nota musicale quando il suono è attivo; si spegne quando è muto.
  * **LED Esterni:** LED Verde (lampeggia a tempo con i BPM), LED Rosso (indica la modalità REC/Apprendimento attiva).

---

## 🛠️ Requisiti Hardware

* **Scheda:** Arduino UNO R4 WiFi
* **Display:** LCD 16x2 (collegamento parallelo standard) con potenziometro per il contrasto.
* **Audio:** 1x Cicalino Piezoelettrico (Piezo Buzzer).
* **Indicatori Visivi:** 1x LED Verde, 1x LED Rosso (con resistenze da 220Ω).
* **Controlli:** 3x Pulsanti tattili (per Mode, Tap e Suono).

### 🔌 Schema Collegamenti (Pinout)

| Componente | Pin Arduino | Note |
| :--- | :--- | :--- |
| **Buzzer Piezo** | `A0` | Collegare l'altro pin a GND |
| **Pulsante Suono (Mute)** | `2` | Collegare l'altro pin a GND (Usa `INPUT_PULLUP`) |
| **Pulsante Mode (Rec/Play)**| `4` | Collegare l'altro pin a GND (Usa `INPUT_PULLUP`) |
| **Pulsante Tap Tempo** | `5` | Collegare l'altro pin a GND (Usa `INPUT_PULLUP`) |
| **LED Verde (Battito)** | `8` | Tramite resistenza 220Ω verso GND |
| **LED Rosso (Mod. REC)** | `9` | Tramite resistenza 220Ω verso GND |
| **LCD RS** | `12` | - |
| **LCD EN** | `11` | - |
| **LCD D4, D5, D6, D7** | `10, 7, 6, 3`| - |

*(Nota per l'LCD: assicurarsi che i Pin 15 e 16 siano alimentati per la retroilluminazione e il Pin 3 sia collegato a un potenziometro per il contrasto).*

---

## 💻 Software e Librerie

Assicurati di avere installato le seguenti librerie tramite il Library Manager dell'IDE di Arduino:

1. `WiFiS3` (Inclusa nel core dell'UNO R4)
2. `PubSubClient` (Per la comunicazione MQTT)
3. `LiquidCrystal` (Per il display LCD 16x2)
4. `Arduino_LED_Matrix` (Inclusa nel core dell'UNO R4, per i LED integrati)

---

## 🌐 Configurazione Node-RED / MQTT

Il sistema si interfaccia con un broker MQTT (es. Mosquitto) e si aspetta la seguente struttura di Topic per la comunicazione con Node-RED:

### 📥 Topic sottoscritti da Arduino (Comandi RX)
* `metronomo/bpm/set` : Riceve un numero (es. `120`) per impostare i BPM in riproduzione.
* `metronomo/suono/set` : Riceve `ON`, `OFF`, `TRUE`, `FALSE`, `1` o `0` per attivare/disattivare il buzzer.
* `metronomo/stato/set` : Riceve qualsiasi messaggio (es. `TOGGLE`) per entrare o uscire dalla modalità REC.
* `metronomo/tap/set` : Riceve qualsiasi messaggio per registrare un battito del Tap Tempo (solo se in mod. REC).

### 📤 Topic pubblicati da Arduino (Telemetria TX)
* `metronomo/bpm` : Invia il BPM attuale (utile per la Dashboard).
* `metronomo/bpm/min` & `metronomo/bpm/max` : Invia statistiche dei BPM registrati.
* `metronomo/suono/stato` : Invia `ON` o `OFF` quando lo stato del suono cambia fisicamente (sincronizza l'interruttore web).
* `metronomo/stato/rec` : Invia `ON` o `OFF` per indicare lo stato della modalità Apprendimento.

### 📦 Importare la Dashboard
Se hai esportato il flusso di Node-RED in formato `.json`, puoi reimportarlo facilmente dal menu di Node-RED andando su **Menu (tre linee) > Import** e selezionando il file del progetto.

---

## 🚀 Guida all'uso

1. **Accensione:** Alimenta l'Arduino. Sul display apparirà "Avvio Metronomo". La scheda proverà a connettersi al WiFi per 5 secondi. Se ci riesce, si collega ad MQTT; in caso contrario, avvia la **Modalità Offline** e il metronomo è subito utilizzabile.
2. **Impostare i BPM:** Puoi inviare il valore numerico dalla Dashboard Node-RED.
3. **Tap Tempo:** 
   * Premi il tasto fisico **MODE** (o il bottone *Modalità REC/PLAY* sulla Dashboard). Il LED Rosso si accende.
   * Premi il tasto fisico **TAP** (o il bottone *Batti il tempo* sulla Dashboard) a tempo di musica per **almeno 4 volte**.
   * Premi nuovamente **MODE**. Il sistema calcolerà la media dei tuoi battiti, applicherà i nuovi BPM e inizierà a lampeggiare/suonare.
4. **Muto:** Premi il tasto fisico collegato al Pin 2, oppure usa l'interruttore Switch sulla Dashboard per zittire il metronomo (il lampeggio del LED verde continuerà visivamente).
