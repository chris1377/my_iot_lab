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
