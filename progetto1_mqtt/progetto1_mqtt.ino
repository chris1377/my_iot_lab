#include <WiFiS3.h>
#include <PubSubClient.h>

char ssid[] = "cloud";
char pass[] = "password";
const char* brokerIP = "10.42.0.1"; 

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// 1. LA NOVITÀ: La funzione che scatta quando arriva un messaggio
void ricezioneMessaggio(char* topic, byte* payload, unsigned int length) {
  // Trasformiamo i byte ricevuti in una Stringa leggibile
  String messaggioRicevuto = "";
  for (int i = 0; i < length; i++) {
    messaggioRicevuto += (char)payload[i];
  }
  
  Serial.print("Messaggio ricevuto sul topic: ");
  Serial.print(topic);
  Serial.print(" -> ");
  Serial.println(messaggioRicevuto);

  // Se il messaggio è "ON", accendi il LED. Se è "OFF", spegnilo.
  if (messaggioRicevuto == "ON") {
    digitalWrite(8, HIGH);
  } else if (messaggioRicevuto == "OFF") {
    digitalWrite(8, LOW);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(8, OUTPUT); // Prepariamo il pin del LED fisico
  
  WiFi.begin(ssid, pass);
  delay(5000); // Questo delay va bene perché siamo nel setup, tutto è fermo
  
  mqttClient.setServer(brokerIP, 1883);
  
  // Diciamo alla libreria MQTT quale funzione usare quando arriva un messaggio
  mqttClient.setCallback(ricezioneMessaggio); 
}

void loop() {
  if (!mqttClient.connected()) {
    if (mqttClient.connect("Arduino_IoT")) {
      Serial.println("Connesso al Broker!");
      
      // 2. APPENA CONNESSO, CI ABBONIAMO AL TOPIC DEL LED
      mqttClient.subscribe("laboratorio/attuatori/led");
      
    } else {
      delay(2000);
      return; 
    }
  }
  
  // Questa funzione controlla in continuazione se ci sono nuovi messaggi in arrivo
  mqttClient.loop(); 
  
  // 3. INVIO DATI SENZA BLOCCARE L'ARDUINO (Usando millis)
  static unsigned long ultimoInvio = 0; // Memoria del cronometro
  
  if (millis() - ultimoInvio > 2000) {  // Se sono passati 2000 millisecondi
    int luce = analogRead(A0);
    String messaggio = String(luce);
    
    mqttClient.publish("laboratorio/sensori", messaggio.c_str());
    
    ultimoInvio = millis(); // Azzeriamo il cronometro
  }
}