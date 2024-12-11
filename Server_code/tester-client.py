import paho.mqtt.client as mqtt
import json
import time
import random

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

# Function to generate random data for each sensor value
def generate_random_data():
    return {
        "accel_x": round(random.uniform(-3.0, 3.0), 2),
        "accel_y": round(random.uniform(-3.0, 3.0), 2),
        "accel_z": round(random.uniform(-3.0, 3.0), 2),
        "magnitude": round(random.uniform(0.0, 4.0), 2),
        "gyro_x": round(random.uniform(-180.0, 180.0), 2),
        "gyro_y": round(random.uniform(-180.0, 180.0), 2),
        "gyro_z": round(random.uniform(-180.0, 180.0), 2),
        "temperature": round(random.uniform(20.0, 30.0), 2)
    }

# Setup MQTT client
client = mqtt.Client()
client.username_pw_set(MQTT_USERNAME, MQTT_PASSWORD)
client.on_connect = on_connect

# Connect to the broker
client.connect(MQTT_BROKER, MQTT_PORT)

# Publish data in a loop every 1 second
client.loop_start()  # Start a separate thread to handle network events

try:
    while True:
        # Generate random sensor data
        message_data = generate_random_data()
        # Convert to JSON string
        message = json.dumps(message_data)
        # Publish the message
        client.publish(MQTT_TOPIC, message, qos=MQTT_QOS)
        print(f"Published message: {message}")
        # Wait 1 second before sending the next message
        time.sleep(1)
except KeyboardInterrupt:
    print("Stopped publishing data.")
finally:
    client.loop_stop()
    client.disconnect()
