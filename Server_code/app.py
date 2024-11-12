from flask import Flask, render_template
import paho.mqtt.client as mqtt
import threading

# Flask app initialization
app = Flask(__name__)

# MQTT configuration
MQTT_BROKER = 'api.allthingstalk.io'
MQTT_PORT = 1883
MQTT_TOPIC = 'device/9iWQhUO01zAzTWerwXReY0F1/asset/MPU6050/state'
MQTT_QOS = 1
MQTT_USERNAME = 'maker:4Q7A7BfBfKjja5HNKxeLUUFXN429PotKxe1WNVCS'  # Replace with your actual username
MQTT_PASSWORD = 'Darren'  # Replace with your actual password

# To hold the latest received message
mqtt_message = ""

# Callback function when a message is received from MQTT broker
def on_message(client, userdata, message):
    global mqtt_message
    mqtt_message = message.payload.decode('utf-8')  # Assuming message is in string format

# Setup MQTT client
def mqtt_setup():
    client = mqtt.Client()
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)  # Set the credentials
    client.on_message = on_message
    client.connect(MQTT_BROKER, MQTT_PORT)
    client.subscribe(MQTT_TOPIC, qos=MQTT_QOS)
    client.loop_forever()

# Start MQTT listener in a separate thread
def start_mqtt_thread():
    mqtt_thread = threading.Thread(target=mqtt_setup)
    mqtt_thread.daemon = True
    mqtt_thread.start()

# Route to display the message on HTML page
@app.route('/')
def index():
    return render_template('index.html', message=mqtt_message)

# Start the MQTT listener when the Flask app starts
if __name__ == '__main__':
    start_mqtt_thread()
    app.run(host='0.0.0.0', port=5000, debug=True)
