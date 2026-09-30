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
