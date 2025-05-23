from flask import Flask, render_template, jsonify
import paho.mqtt.client as mqtt
import threading
from flask_socketio import SocketIO, emit
import json
import time
import logging

# Flask app initialization
app = Flask(__name__)
socketio = SocketIO(app, cors_allowed_origins="*")  # Enable CORS for WebSocket

# MQTT configuration
MQTT_BROKER = '1f921750cd3c40f8a5b596c38018d775.s1.eu.hivemq.cloud'
MQTT_PORT = 8883
MQTT_TOPIC = 'MPU6050'
MQTT_QOS = 0
MQTT_USERNAME = 'hivemq.webclient.1732014074548'  # Replace with your actual username
MQTT_PASSWORD = 'GA9EB182CwsHfevb:,<;'  # Replace with your actual password

# Path to certificate files
CLIENT_CERT = 'client.crt'
CLIENT_KEY = 'client.key'
CA_CERT = 'isrgrootx1.pem'

# Variables to hold the latest received data and previous timestamp
mqtt_message = ""
client = None  # Declare the MQTT client globally

# Callback function when a message is received from MQTT broker
def on_message(client, userdata, message):
    global mqtt_message

    mqtt_message = message.payload.decode('utf-8')
    # print(f"Received MQTT Message: {mqtt_message}")

    if mqtt_message:
        try:
            # Parse JSON string to dictionary
            data = json.loads(mqtt_message)

            # Emit the updated data to the frontend
            socketio.emit('mqtt_message', {'message': json.dumps(data)})
        except json.JSONDecodeError as e:
            print(f"Error: Invalid JSON received: {mqtt_message}. Error: {e}")
    else:
        print("Received an empty message. Skipping this message.")

# MQTT setup function
def mqtt_setup():
    global client  # Access the global client variable
    client = mqtt.Client()
    client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)  # Set the credentials

    try:
        # Configure TLS/SSL with certificates
        client.tls_set(
            ca_certs=CA_CERT,       # Path to CA certificate
            certfile=CLIENT_CERT,   # Path to client certificate
            keyfile=CLIENT_KEY      # Path to client private key
        )
        client.tls_insecure_set(False)  # Enforce server certificate validation

        # Enable logging for debugging
        # logging.basicConfig(level=logging.DEBUG)
        client.enable_logger()

        client.on_message = on_message

        # Connect to MQTT broker
        client.connect(MQTT_BROKER, MQTT_PORT)
        client.subscribe(MQTT_TOPIC, qos=MQTT_QOS)
        print(f"Subscribed to MQTT topic '{MQTT_TOPIC}' at {MQTT_BROKER}:{MQTT_PORT}")

        # Start the MQTT loop
        client.loop_forever()
    except Exception as e:
        print(f"Error: Failed to connect to MQTT broker. {e}")

# Start MQTT listener in a separate thread
def start_mqtt_thread():
    mqtt_thread = threading.Thread(target=mqtt_setup)
    mqtt_thread.daemon = True
    mqtt_thread.start()

# Flask route to display the latest MQTT message
@app.route('/latest-message')
def latest_message():
    global mqtt_message
    if mqtt_message:
        return jsonify({"latest_message": mqtt_message})
    return jsonify({"message": "No messages received yet."}), 404

# Route to display the frontend
@app.route('/')
def index():
    return render_template('index.html')

@socketio.on('test_connection')
def handle_test_connection(data):
    print("Test connection received:", data)
    emit('test_response', {'message': 'Connection successful'})

# New callback function to handle "reset_orientation" event
@socketio.on('reset_orientation')
def handle_reset_orientation(data):
    print(f"Reset Orientation Triggered: {data['state']}")
    # Send MQTT message with reset orientation command
    if client:
        payload = json.dumps({'reset_orientation': data['state']})
        client.publish('GYROrst', payload)  # Publish on the "MPU6050/reset" topic

# New callback function to handle "toggle_motor_state" event
@socketio.on('toggle_motor_state')
def handle_toggle_motor_state(data):
    print(f"Motor State Changed: {data['state']}")
    # Send MQTT message with motor state command
    if client:
        payload = json.dumps({'motor_state': data['state']})
        client.publish('MOTOR', payload)  # Publish on the "MPU6050/motor" topic

# New callback function to handle "toggle_low_power_mode" event
@socketio.on('toggle_low_power_mode')
def handle_toggle_low_power_mode(data):
    print(f"LP State Changed: {data['state']}")
    # Send MQTT message with motor state command
    if client:
        payload = json.dumps({'LP_mode': data['state']})
        client.publish('LP', payload)  # Publish on the "MPU6050/LP" topic

# Start the MQTT listener when the Flask app starts
if __name__ == '__main__':
    start_mqtt_thread()
    socketio.run(app, host='0.0.0.0', port=5000, debug=True)
