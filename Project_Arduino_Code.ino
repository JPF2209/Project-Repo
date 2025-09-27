#include <Firebase_Arduino_WiFiNINA.h>
#include <Arduino.h>
#include <DHT.h>
#include <Servo.h>
#include <ArduinoJson.h> 
#include <WiFiNINA.h> 

// WiFi credentials
char ssid[] = "TelstraE78C95";     
char pass[] = "mnnu9vh7x7"; 

// Firebase project info
#define FIREBASE_HOST "sit314-aircon-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "f7mr47MjbkM95HXluVoQaGEIoSa0u8LivmFtG0U0"

// DHT Sensor
#define DHTPIN 2        
#define DHTTYPE DHT22   
DHT dht(DHTPIN, DHTTYPE);

// LED Control
#define LED_PIN 3       

// Firebase object
FirebaseData firebaseData;

// Servo object
// Servo motor;

// Timing and State Variables
unsigned long previousMillisMotor = 0;
unsigned long previousMillisTemp = 0;
unsigned long previousMillisFirebase = 0; 

// For fan
// const long motorInterval = 1000;  

// For LED
const long motorInterval = 500;  

const long tempInterval = 30000;   
const long firebaseInterval = 5000; 

float temperature = 0; 
float minimum = 18.0; 
bool fan = false;      
bool left = true;      

String ID = "Air-Con1"; 

// Function prototypes
void connectToWiFi();
void connectToFirebase();
float getTelemetry(); 
void sendTelemetry(float temperature);
void tempProcessing();

void setup() {
    Serial.begin(9600);
    while (!Serial);

    // Pin 9 for the servo motor
    // motor.attach(9);

    // For LED instead
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW); 

    dht.begin();

    connectToWiFi();
    connectToFirebase();
}

void loop() {
    unsigned long currentMillis = millis();

    // Fan logic (highest priority)
    if (currentMillis - previousMillisMotor >= motorInterval) {
        previousMillisMotor = currentMillis; 
        
        //Servo motor code
        // if (fan && left) {
        //     motor.write(180); // Turn motor full speed in one direction
        //     left = false;
        // } else if (fan) {
        //     motor.write(0);   // Turn motor full speed in the other direction
        //     left = true;
        // }

        // LED blink code
        if (fan) {
            // Blink logic
            if (left) {
                digitalWrite(LED_PIN, HIGH);
                left = false;
            } else {
                digitalWrite(LED_PIN, LOW);
                left = true;
            }
        } else {
            // Keep LED off
            digitalWrite(LED_PIN, LOW);
            left = true; 
        }
    }

    // This reads changed values for change temp
    if (currentMillis - previousMillisTemp >= tempInterval) {
        previousMillisTemp = currentMillis;
        tempProcessing();
    }

    // Check if Firebase is ready and if it's time to process
    if (currentMillis - previousMillisFirebase >= firebaseInterval) { 
        previousMillisFirebase = currentMillis; 
        
        Serial.println("--- Attempting Firebase GET ---"); 
        float value = getTelemetry();
        
        if (value > -999.0f) 
        {
            if (value != minimum)
            {
                minimum = value; 
                Serial.print("New minimum set: ");
                Serial.println(minimum);
                // Re-evaluate fan state
                fan = (temperature > minimum);
            }
        }
    }
}

void connectToWiFi() {
    Serial.print("Connecting to Wi-Fi...");
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}

void connectToFirebase() {
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH, ssid, pass);
    Firebase.reconnectWiFi(true);
    Serial.println("Connected to Firebase.");
}

float getTelemetry() {
    String path = ID + "/change_temperature/temperature"; 

    // Use getFloat() to retrieve the temperature directly.
    if (Firebase.getFloat(firebaseData, path)) {
      if (firebaseData.dataType() == "float" || firebaseData.dataType() == "int") {
        Serial.print("Received value: ");
        Serial.println(firebaseData.floatData()); 
        return firebaseData.floatData();
      } else {
         Serial.print("Firebase GET failed due to wrong data type: ");
         Serial.println(firebaseData.dataType());
         return -999.0f; 
      }
    } else {
      Serial.print("Firebase GET failed: ");
      Serial.println(firebaseData.errorReason());
    }
    return -999.0f;
}

void sendTelemetry(float currentTemp) {
    // Define the path where we will store the single, most recent JSON object.
    String path = ID + "/current_data";

    // Create a JSON string with the temperature and a server-side timestamp.
    String jsonStr = "{\"temperature\":" + String(currentTemp) + ",\"timestamp\":{\".sv\":\"timestamp\"}}";

    if (Firebase.setJSON(firebaseData, path, jsonStr)) {
    Serial.println("Telemetry sent successfully.");
    } else {
    Serial.print("Failed to send data: ");
    Serial.println(firebaseData.errorReason());
    }
}

void tempProcessing() {
    float currentTemp = dht.readTemperature();

    if (!isnan(currentTemp)) {
        temperature = currentTemp; 
        Serial.print("Current Temp: ");
        Serial.println(temperature);
        
        fan = (temperature > minimum);
        Serial.print("Fan State: ");
        Serial.println(fan ? "ON" : "OFF");
        
        sendTelemetry(temperature);
        
    } else {
        Serial.println("Failed to read temperature from DHT sensor.");
    }
}