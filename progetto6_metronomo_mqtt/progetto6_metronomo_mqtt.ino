#include <WiFiS3.h>
#include <PubSubClient.h>

char ssid[] = "cloud";
char pass[] = "password";
const char* brokerIP = "10.42.0.1"; 

// 1. Istanze di rete mancanti
WiFiClient espClient;
PubSubClient mqttClient(espClient);

int minBPM = 0;
int maxBPM = 0;

const int pinLedVerde = 8;
const int pinLedRosso = 9;
const int pinBtnMode = 4;
const int pinBtnTap = 5;

bool isLearnMode = false;
int ultimoStatoBtn = HIGH;
int ultimoStatoTap = HIGH;

unsigned long tempiTap[10];
int numeroTap = 0;         

int currentBPM = 0;
unsigned long playInterval = 0;
unsigned long ultimoBlink = 0;

// ==========================================
// RICEZIONE MESSAGGI MQTT
// ==========================================
void ricezioneMessaggio(char* topic, byte* payload, unsigned int length) {
  String messaggioRicevuto = "";
  for (unsigned int i = 0; i < length; i++) {
    messaggioRicevuto += (char)payload[i];
  }
  
  Serial.print("Messaggio ricevuto sul topic: ");
  Serial.print(topic);
  Serial.print(" -> ");
  Serial.println(messaggioRicevuto);

  // Topic per impostare un nuovo BPM dall'esterno (es: mosquitto_pub)
  if (String(topic) == "metronomo/bpm/set") {
    int nuovoBPM = messaggioRicevuto.toInt();
    if (nuovoBPM > 0) {
      currentBPM = nuovoBPM;
      playInterval = 60000 / currentBPM; // Ricalcola il tempo di lampeggio
      
      if (minBPM == 0 || currentBPM < minBPM) minBPM = currentBPM;
      if (currentBPM > maxBPM) maxBPM = currentBPM;

      // Invia la conferma dello stato aggiornato
      mqttClient.publish("metronomo/bpm", String(currentBPM).c_str());
    }
  }
}

// ==========================================
// RICONNESSIONE AL BROKER
// ==========================================
void riconnetti() {
  while (!mqttClient.connected()) {
    Serial.print("Connessione al Broker MQTT...");
    if (mqttClient.connect("Arduino_UNO_R4")) {
      Serial.println(" Connesso!");
      // Ci iscriviamo al topic comandi
      mqttClient.subscribe("metronomo/bpm/set");
    } else {
      Serial.print(" Fallita, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" Riprovo tra 3 secondi");
      delay(3000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  Serial.print("Connessione al Wi-Fi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Connesso!");

  mqttClient.setServer(brokerIP, 1883);
  mqttClient.setCallback(ricezioneMessaggio);
  
  pinMode(pinLedVerde, OUTPUT);
  pinMode(pinLedRosso, OUTPUT);
  pinMode(pinBtnMode, INPUT_PULLUP);
  pinMode(pinBtnTap, INPUT_PULLUP);
}

void loop() {
  if (!mqttClient.connected()) {
    riconnetti();
  }
  mqttClient.loop();

  // Invia i dati di telemetria ogni 3 secondi (evita di bloccare la rete)
  static unsigned long ultimoInvio = 0;
  if (millis() - ultimoInvio > 3000) {
    mqttClient.publish("metronomo/bpm", String(currentBPM).c_str());
    mqttClient.publish("metronomo/bpm/min", String(minBPM).c_str());
    mqttClient.publish("metronomo/bpm/max", String(maxBPM).c_str());
    ultimoInvio = millis();
  }

  // 1. PULSANTE MODE
  int statoBtnMode = digitalRead(pinBtnMode);
  if(statoBtnMode == LOW && ultimoStatoBtn == HIGH){
    isLearnMode = !isLearnMode; 
    
    if (isLearnMode) {
      Serial.println("Modalita': LEARN");
      mqttClient.publish("metronomo/stato/rec", "ON");
      numeroTap = 0;
      digitalWrite(pinLedVerde, LOW);
      digitalWrite(pinLedRosso, HIGH);
    } else {
      Serial.println("Modalita': PLAY");
      mqttClient.publish("metronomo/stato/rec", "OFF");
      if (numeroTap >= 4) {
        unsigned long tempoTotale = tempiTap[numeroTap - 1] - tempiTap[0];
        unsigned long intervalloMedio = tempoTotale / (numeroTap - 1);

        currentBPM = 60000 / intervalloMedio;
        playInterval = intervalloMedio;

        if (minBPM == 0 || currentBPM < minBPM) minBPM = currentBPM;
        if (currentBPM > maxBPM) maxBPM = currentBPM;

        // Pubblica subito il nuovo BPM appena calcolato
        mqttClient.publish("metronomo/bpm", String(currentBPM).c_str());
      }
      digitalWrite(pinLedRosso, LOW);
    }
    delay(250); 
  }
  ultimoStatoBtn = statoBtnMode;

  // 2. PULSANTE TAP
  int statoBtnTap = digitalRead(pinBtnTap);
  if(isLearnMode && statoBtnTap == LOW && ultimoStatoTap == HIGH) {
    if (numeroTap < 10) {
      tempiTap[numeroTap] = millis();
      numeroTap++;
      
      digitalWrite(pinLedRosso, LOW);
      delay(50);
      digitalWrite(pinLedRosso, HIGH);
    }
  }
  ultimoStatoTap = statoBtnTap;

  // 3. LAMPEGGIO LED VERDE (Riproduzione)
  if (!isLearnMode) {
    if (currentBPM > 0) {
      if (millis() - ultimoBlink >= playInterval) {
        ultimoBlink = millis();
        digitalWrite(pinLedVerde, !digitalRead(pinLedVerde));
      }
    } else {
      digitalWrite(pinLedVerde, LOW);
    }
  }
}