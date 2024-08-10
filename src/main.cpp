#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
// #include <tensorflow/lite/experimental/micro/kernels/all_ops_resolver.h>
// #include <tensorflow/lite/experimental/micro/micro_interpreter.h>
// #include <tensorflow/lite/experimental/micro/micro_error_reporter.h>
// #include <tensorflow/lite/experimental/micro/micro_allocator.h>
// #include <tensorflow/lite/schema/schema_generated.h>
// #include <tensorflow/lite/version.h>

// #include "tensorflow/lite/micro/all_ops_resolver.h"
// #include "tensorflow/lite/micro/micro_error_reporter.h"
// #include "tensorflow/lite/micro/micro_interpreter.h"
// #include "tensorflow/lite/schema/schema_generated.h"
// #include "tensorflow/lite/version.h"

#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>


// Include your model header file
#include "../weather_model/model.h"  // Make sure this path is correct

// WiFi credentials
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// MQTT Broker settings
const char* mqtt_server = "broker.mqttdashboard.com";
const char* mqtt_topic_sensor1 = "thierry/sensor1";
const char* mqtt_topic_sensor2 = "thierry/sensor2";

WiFiClient espClient;
PubSubClient client(espClient);

// TensorFlow Lite variables
tflite::MicroErrorReporter micro_error_reporter;
tflite::AllOpsResolver resolver;
tflite::MicroInterpreter* interpreter;
const tflite::Model* model;
tflite::MicroAllocator* allocator;
static const int kTensorArenaSize = 2 * 1024;
uint8_t tensor_arena[kTensorArenaSize];

void setupWifi() {
    delay(100);
    Serial.print("\nConnecting to ");
    Serial.println(ssid);
    
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
        Serial.print("-");
    }
    
    Serial.print("\nConnected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.println(message);

    // Parse the message (assuming CSV format: "temperature,humidity")
    float temperature = 0.0, humidity = 0.0;
    sscanf(message.c_str(), "%f,%f", &temperature, &humidity);

    // Prepare the input tensor
    float* input = interpreter->input(0)->data.f;
    input[0] = temperature;
    input[1] = humidity;

    // Invoke the model
    interpreter->Invoke();

    // Retrieve the prediction
    float* output = interpreter->output(0)->data.f;
    float predicted_temp = output[0];
    float predicted_humidity = output[1];

    // Check predictions against actual values and control LEDs
    if (abs(predicted_temp - temperature) < 1.0) {
        // Light up temperature LED
        digitalWrite(17, HIGH);  // Replace with your LED pin
    } else {
        digitalWrite(17, LOW);
    }

    if (abs(predicted_humidity - humidity) < 5.0) {
        // Light up humidity LED
        digitalWrite(16, HIGH);  // Replace with your LED pin
    } else {
        digitalWrite(16, LOW);
    }
}

void reconnect() {
    String clientId = "ESP32Client-" + String(ESP.getEfuseMac(), HEX);
    
    while (!client.connected()) {
        Serial.print("\nConnecting to MQTT broker...");
        if (client.connect(clientId.c_str())) {
            Serial.println("connected");
            client.subscribe(mqtt_topic_sensor1);
            client.subscribe(mqtt_topic_sensor2);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    setupWifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);

    // Load the TensorFlow Lite model
    model = tflite::GetModel(model);  // Use `model_data` from the included header file
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.println("Model schema version mismatch.");
        while (1);
    }

    // Allocate memory for the model
    allocator = tflite::MicroAllocator::Create(tensor_arena, kTensorArenaSize, &micro_error_reporter);
    interpreter = new tflite::MicroInterpreter(model, resolver, allocator, &micro_error_reporter);
    interpreter->AllocateTensors();
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
}
