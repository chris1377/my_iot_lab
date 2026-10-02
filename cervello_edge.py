import paho.mqtt.client as mqtt

BROKER = "127.0.0.1"
TOPIC_SENSORE = "laboratorio/sensori"
TOPIC_ATTUATORE = "laboratorio/attuatori/led"
SOGLIA_BUIO = 15

# Funzione eseguita appena lo script si connette al Broker
def on_connect(client, userdata, flags, rc):
    print("Cervello Edge connesso! In attesa di dati dal sensore...")
    client.subscribe(TOPIC_SENSORE)

# Funzione eseguita ogni volta che arriva un messaggio dall'Arduino
def on_message(client, userdata, msg):
    # Decodifichiamo il messaggio in un numero intero
    luce = int(msg.payload.decode())
    print(f"Luce ambientale rilevata: {luce}")
    
    # Logica decisionale dell'Edge
    if luce < SOGLIA_BUIO:
        print("-> Troppo buio! Comando accensione LED.")
        client.publish(TOPIC_ATTUATORE, "ON")
        # Nota: se nel tuo sketch hai mantenuto la logica a topic, usa:
        # client.publish("laboratorio/attuatori/led/accendi", "")
    else:
        print("-> Luce sufficiente. Comando spegnimento LED.")
        client.publish(TOPIC_ATTUATORE, "OFF")
        # Nota: se nel tuo sketch hai mantenuto la logica a topic, usa:
        # client.publish("laboratorio/attuatori/led/spegni", "")

# Configurazione e avvio
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message

client.connect(BROKER, 1883, 60)

# Mantiene lo script in esecuzione per sempre
try:
    client.loop_forever()
except KeyboardInterrupt:
    print("\nCervello Edge disattivato.")
