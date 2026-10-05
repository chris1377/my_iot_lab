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
  
  pinMode(pinLedVerde, OUTPUT);
  pinMode(pinLedRosso, OUTPUT);
  pinMode(pinBtnMode, INPUT_PULLUP);
  pinMode(pinBtnTap, INPUT_PULLUP);
}

void loop() {
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
