#include <ESP8266WiFi.h> // Libreria corretta per la tua scheda
#include <PubSubClient.h>

const char* ssid = "cloud";
const char* password = "password";
const char* brokerIP = "10.42.0.1"; // IP del tuo PC Linux

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void setup() {
  Serial.begin(115200);
  
  WiFi.begin(ssid, password);
  Serial.print("Connessione al Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Connesso!");
  
  mqttClient.setServer(brokerIP, 1883);
}

void loop() {
  if (!mqttClient.connected()) {
    // Usiamo un ID univoco per il Broker
    if (mqttClient.connect("ESP8266_Nodo1")) { 
      Serial.println("Connesso a Mosquitto!");
    } else {
      delay(500);
      return;
    }
  }
  
  mqttClient.loop();
  
  static unsigned long ultimoInvio = 0;
  if (millis() - ultimoInvio > 500) { 
    
    // Generiamo un numero finto per testare la comunicazione
    int fintaTemperatura = random(20, 30); 
    String payload = String(fintaTemperatura);
    
    // Pubblichiamo i dati sul nuovo topic
    mqttClient.publish("laboratorio/esp/temperatura", payload.c_str());
    
    Serial.print("Inviato dato ESP8266: ");
    Serial.println(payload);
    
    ultimoInvio = millis();
  }
}