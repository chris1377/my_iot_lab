#include <WiFiS3.h>
char ssid[] = "cloud";
char pass[] = "password";
WiFiServer server(80);


int minBPM = 0;
int maxBPM = 0;


const int pinLedVerde = 8;
const int pinLedRosso = 9;
const int pinBtnMode = 4;
const int pinBtnTap = 5;

// Variabili di stato
bool isLearnMode = false;
int ultimoStatoBtn = HIGH;
int ultimoStatoTap = HIGH;

// Variabili per il calcolo dei tap
unsigned long tempiTap[10]; // Una lista che può contenere fino a 10 tempi
int numeroTap = 0;          // Conta quanti tap abbiamo fatto

// Variabili per il ritmo
int currentBPM = 0;
unsigned long playInterval = 0;
unsigned long ultimoBlink = 0;

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, pass);
  delay(5000);
  server.begin();
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());
  
  pinMode(pinLedVerde, OUTPUT);
  pinMode(pinLedRosso, OUTPUT);
  pinMode(pinBtnMode, INPUT_PULLUP);
  pinMode(pinBtnTap, INPUT_PULLUP);
}

void loop() {
  // ==========================================
  // 0. GESTIONE RICHIESTE WEB (API REST)
  // ==========================================
  WiFiClient client = server.available();

  if (client) {
    // Leggiamo la prima riga della richiesta HTTP (es: "GET /bpm/ HTTP/1.1")
    String request = client.readStringUntil('\r'); 
    client.flush(); // Ignoriamo il resto dei dati per ora
    
    // Prepariamo la risposta HTTP standard
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/plain");
    client.println("Access-Control-Allow-Origin: *"); // Fondamentale per farla leggere alla tua Dashboard HTML
    client.println("Access-Control-Allow-Methods: GET, PUT, DELETE, OPTIONS"); 
    client.println("Access-Control-Allow-Headers: Content-Type");
    client.println(); // Riga vuota obbligatoria

    if (request.indexOf("OPTIONS") >= 0) {}

    // Smistiamo la richiesta in base all'endpoint richiesto dal laboratorio
    if (request.indexOf("GET /bpm/ ") >= 0) {
      client.print(currentBPM);
    } 
    else if (request.indexOf("GET /bpm/min/ ") >= 0) {
      client.print(minBPM);
    } 
    else if (request.indexOf("GET /bpm/max/ ") >= 0) {
      client.print(maxBPM);
    } 
    else if (request.indexOf("DELETE /bpm/min/ ") >= 0) {
      minBPM = 0; // Azzera il minimo, come richiesto dal progetto
      client.print(minBPM);
    } 
    else if (request.indexOf("DELETE /bpm/max/ ") >= 0) {
      maxBPM = 0; // Azzera il massimo
      client.print(maxBPM);
    } 
    else if (request.indexOf("PUT /bpm/") >= 0) {
      // 1. Saltiamo tutte le intestazioni HTTP finché non troviamo una riga vuota
      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") { // Riga vuota trovata, i dati veri iniziano qui!
          break;
        }
      }
      
      // 2. Leggiamo il corpo del messaggio (il nuovo BPM)
      String body = client.readStringUntil('\r');
      int nuovoBPM = body.toInt();
      
      // 3. Se è un numero valido, aggiorniamo il metronomo
      if (nuovoBPM > 0) {
        currentBPM = nuovoBPM;
        playInterval = 60000 / currentBPM;
        
        // Aggiorniamo anche i record se necessario
        if (minBPM == 0 || currentBPM < minBPM) minBPM = currentBPM;
        if (currentBPM > maxBPM) maxBPM = currentBPM;
      }
      
      client.print(currentBPM); // Rispondiamo con il valore aggiornato
    }

    delay(1);
    client.stop(); // Chiudiamo la connessione
  }



  // ==========================================
  // 1. LETTURA PULSANTE MODE (Cambio Modalità)
  // ==========================================
  int statoBtnMode = digitalRead(pinBtnMode);
  
  if(statoBtnMode == LOW && ultimoStatoBtn == HIGH){
    
    isLearnMode = !isLearnMode; 
    
    if (isLearnMode) {
      Serial.println("Modalita': LEARN (Registrazione)");
      numeroTap = 0;
      digitalWrite(pinLedVerde, LOW);
      digitalWrite(pinLedRosso, HIGH);
    } else {
      Serial.println("Modalita': PLAY (Riproduzione)");

      // Controlliamo se abbiamo almeno 4 tap, come richiesto dal progetto[cite: 8]
      if (numeroTap >= 4) {
        // Calcolo dell'intervallo medio
        unsigned long tempoTotale = tempiTap[numeroTap - 1] - tempiTap[0];
        unsigned long intervalloMedio = tempoTotale / (numeroTap - 1);

        // Calcolo BPM
        currentBPM = 60000 / intervalloMedio;
        playInterval = intervalloMedio;

        Serial.print("Nuovo BPM calcolato: ");
        Serial.println(currentBPM);

        if (minBPM == 0 || currentBPM < minBPM) {
          minBPM = currentBPM;
        }
        // Se il nuovo BPM è più alto, aggiorna il massimo
        if (currentBPM > maxBPM) {
          maxBPM = currentBPM;
        }

      } else {
        Serial.println("Tap insufficienti (minimo 4). BPM invariato.");
      }

      // Spegniamo il rosso (il verde viene gestito in basso)
      digitalWrite(pinLedRosso, LOW);
    }
    
    delay(250); // Ritardo aumentato per evitare il rimbalzo (doppio scatto)
  }
  
  ultimoStatoBtn = statoBtnMode;

  // ==========================================
  // 2. LETTURA PULSANTE TAP (Registrazione)
  // ==========================================
  int statoBtnTap = digitalRead(pinBtnTap);

  if(isLearnMode && statoBtnTap == LOW && ultimoStatoTap == HIGH) {

    tempiTap[numeroTap] = millis(); // Salva il tempo nella posizione attuale
    numeroTap = numeroTap + 1;      // Aumenta il contatore di 1

    Serial.print("Tap registrato numero: ");
    Serial.println(numeroTap);

    digitalWrite(pinLedRosso, LOW);  // Spegne temporaneamente per l'effetto visivo
    delay(50);
    digitalWrite(pinLedRosso, HIGH);
  }

  ultimoStatoTap = statoBtnTap;

  // ==========================================
  // 3. LOGICA DI RIPRODUZIONE (Lampeggio LED)
  // ==========================================
  if (isLearnMode == false) {

    // Se abbiamo impostato un ritmo (maggiore di 0)
    if (currentBPM > 0) {

      // Controlliamo se è passato il tempo dell'intervallo
      if (millis() - ultimoBlink >= playInterval) {
        ultimoBlink = millis(); // Aggiorniamo il cronometro

        // Inverte lo stato del LED verde
        int statoAttualeLed = digitalRead(pinLedVerde);
        digitalWrite(pinLedVerde, !statoAttualeLed); 
      }

    } else {
      // All'avvio, quando BPM è ancora 0, assicuriamoci che il LED resti spento[cite: 8]
      digitalWrite(pinLedVerde, LOW);
    }
  }
}
