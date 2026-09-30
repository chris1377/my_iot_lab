#include <WiFiS3.h>
char ssid[] = "cloud";
char pass[] = "password";
WiFiServer server(80);
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  WiFi.begin(ssid, pass);
  delay(5000);
  server.begin();
  Serial.println("CIAO");
  Serial.println(WiFi.localIP());

  pinMode(LED_BUILTIN,OUTPUT);
  pinMode(8,OUTPUT);


}

void loop() {
  WiFiClient client = server.available();
  
  if (client) {
    String currentLine = ""; // Creiamo un "taccuino" per appuntare cosa dice il browser
    
    while (client.connected()) {
      if (client.available()) {     // Se ci sono caratteri da leggere
        char c = client.read();     // Leggiamo un carattere alla volta
        
        if (c == '\n') { // Se il browser va "a capo" (nuova riga)
          
          if (currentLine.length() == 0) {
            // Se la riga è completamente vuota, significa che il browser 
            // ha finito la sua richiesta HTTP. ORA tocca a noi rispondere!

            int valoreSensore = analogRead(A0);
            
            // Inizio a scrivere la pagina web avanzata
            client.println("<html><head>");
            // Dico al browser di aggiornare la pagina ogni 2 secondi
            client.println("<meta http-equiv=\"refresh\" content=\"2\">");
            // Aggiungo un po' di stile (CSS) per centrare il testo e colorare i bottoni
            client.println("<style>");
            client.println("body { font-family: Arial; text-align: center; margin-top: 50px; }");
            client.println(".btn { padding: 15px 30px; font-size: 20px; text-decoration: none; color: white; border-radius: 5px; margin: 10px; }");
            client.println(".verde { background-color: #4CAF50; }");
            client.println(".rosso { background-color: #f44336; }");
            client.println("</style></head><body>");

            // Mostro il sensore
            client.print("<h1>Valore Luce: ");
            client.print(valoreSensore);
            client.println("</h1><br><br>");

            // Disegno i bottoni usando le classi CSS appena create
            client.println("<a href=\"/ON\" class=\"btn verde\">ACCENDI LED</a>");
            client.println("<a href=\"/OFF\" class=\"btn rosso\">SPEGNI LED</a>");

            client.println("</body></html>");
            
            break; // Usciamo dal ciclo 'while', abbiamo finito di parlare
            
          } else {
            // Se la riga non era vuota, la cancelliamo per leggere quella successiva
            currentLine = ""; 
          }
          
        } else if (c != '\r') {  
          // Se non è un "a capo", aggiungiamo il carattere al nostro taccuino
          currentLine += c;

          if(currentLine.endsWith("GET /ON")){
            digitalWrite(8,HIGH);
          }
          if(currentLine.endsWith("GET /OFF")){
            digitalWrite(8,LOW);
          }

        }
      }
    }
    
    // Ora possiamo salutare e chiudere in modo pulito
    client.stop(); 
  }
}
