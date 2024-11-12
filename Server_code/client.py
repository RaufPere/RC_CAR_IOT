import paho.mqtt.client as mqtt

# MQTT configuration
MQTT_BROKER = 'api.allthingstalk.io'
MQTT_PORT = 1883
MQTT_TOPIC = 'device/9iWQhUO01zAzTWerwXReY0F1/asset/MPU6050/state'
MQTT_QOS = 1
MQTT_USERNAME = 'maker:4Q7A7BfBfKjja5HNKxeLUUFXN429PotKxe1WNVCS'  # Replace with your actual username
MQTT_PASSWORD = 'Darren'  # Replace with your actual password

# Callback function when the client connects
def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    # Once connected, publish a simple message
    message = "Hello, MQTT!"
    client.publish(MQTT_TOPIC, message, qos=MQTT_QOS)

# Setup MQTT client
client = mqtt.Client()
client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)  # Set the credentials
client.on_connect = on_connect

# Connect to the broker
client.connect(MQTT_BROKER, MQTT_PORT)

# Start the loop to process messages
client.loop_forever()
