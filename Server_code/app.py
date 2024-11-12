from flask import Flask, render_template
import paho.mqtt.client as mqtt
import threading
from flask_socketio import SocketIO, emit
import json
import time

# Flask app initialization
app = Flask(__name__)
socketio = SocketIO(app)

# MQTT configuration
MQTT_BROKER = 'api.allthingstalk.io'
MQTT_PORT = 1883
MQTT_TOPIC = 'device/9iWQhUO01zAzTWerwXReY0F1/asset/MPU6050/state'
MQTT_QOS = 1
MQTT_USERNAME = 'maker:4Q7A7BfBfKjja5HNKxeLUUFXN429PotKxe1WNVCS'  # Replace with your actual username
MQTT_PASSWORD = 'Darren'  # Replace with your actual password

# Variables to hold the latest received data and previous timestamp
mqtt_message = ""
previous_time = None
velocity_x = velocity_y = velocity_z = 0  # Initial velocity components in m/s

# Callback function when a message is received from MQTT broker
def on_message(client, userdata, message):
    global mqtt_message, previous_time, velocity_x, velocity_y, velocity_z
    
    # Decode message payload
    mqtt_message = message.payload.decode('utf-8')
    
    # Check if mqtt_message is not empty and valid JSON
    if mqtt_message:
        try:
            # Parse JSON string to dictionary
            data = json.loads(mqtt_message)
            
            # Get current time and calculate the time difference (delta_t)
            current_time = time.time()
            if previous_time is not None:
                delta_t = current_time - previous_time
            else:
                delta_t = 0  # First message, no time difference
            previous_time = current_time

            # Extract acceleration values (assume values are in m/s²)
            accel_x = data.get("accel_x", 0.0)
            accel_y = data.get("accel_y", 0.0)
            accel_z = data.get("accel_z", 0.0)

            # Update velocity using v = u + at (integrating acceleration over time)
            velocity_x += accel_x * delta_t
            velocity_y += accel_y * delta_t
            velocity_z += accel_z * delta_t

            # Calculate the magnitude of the velocity (speed)
            speed = (velocity_x**2 + velocity_y**2 + velocity_z**2)**0.5

            # Add speed to the data dictionary
            data["speed"] = round(speed, 2)

            # Emit the updated data as JSON string
            socketio.emit('mqtt_message', {'message': json.dumps(data)})
        except json.JSONDecodeError:
            pass
    else:
        print("Received an empty message. Skipping this message.")

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
    return render_template('index.html')

# Start the MQTT listener when the Flask app starts
if __name__ == '__main__':
    start_mqtt_thread()
    socketio.run(app, host='0.0.0.0', port=5000, debug=True)
