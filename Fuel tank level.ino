#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ---------------- WiFi ----------------
const char* ssid = "HF Electronotics 2.4Ghz";
const char* password = "HF@007219";

// ---------------- Firebase ----------------
#define FIREBASE_HOST "water-tank-53006-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "13pZRPU4japDjXGXHe1BeOapG4LtjBZ6qgCychG2"

FirebaseData fbData;

// -------- Tank Settings --------
#define TANK_HEIGHT_CM      35.0
#define TOTAL_TANK_LITERS    5.0   // ✅ 5 liter tank

// -------- DS18B20 --------
#define SENSOR_PIN D2
OneWire oneWire(SENSOR_PIN);
DallasTemperature DS18B20(&oneWire);

// -------- Ultrasonic --------
#define TRIG_PIN D7
#define ECHO_PIN D6

float temperature_C;
float temperature_F;
long duration;
float distance_cm;
float waterPercent;
float fuelLiters;
String tankStatus;

// --------------------------------------------------
void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  DS18B20.begin();
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

// --------------------------------------------------
void loop() {

  // ===== Temperature =====
  DS18B20.requestTemperatures();
  temperature_C = DS18B20.getTempCByIndex(0);
  temperature_F = temperature_C * 9 / 5 + 32;

  // ===== Ultrasonic =====
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  duration = pulseIn(ECHO_PIN, HIGH, 30000);
  distance_cm = duration * 0.034 / 2;

  if (distance_cm < 1) distance_cm = 1;
  if (distance_cm > TANK_HEIGHT_CM) distance_cm = TANK_HEIGHT_CM;

  // ===== Percentage =====
  waterPercent = ((TANK_HEIGHT_CM - distance_cm) / TANK_HEIGHT_CM) * 100.0;

  // ===== Liters =====
  fuelLiters = (waterPercent / 100.0) * TOTAL_TANK_LITERS;

  // ===== ONE FINAL STATUS (liter-based) =====
  if (fuelLiters >= 4.0)
    tankStatus = "FULL";
  else if (fuelLiters >= 2.0)
    tankStatus = "HALF";
  else
    tankStatus = "EMPTY";

  // ===== Firebase Upload (CLEAN) =====
  Firebase.setFloat(fbData, "/tankData/distanceCM", distance_cm);
  Firebase.setFloat(fbData, "/tankData/fuelLiters", fuelLiters);
  Firebase.setFloat(fbData, "/tankData/percentage", waterPercent);
  Firebase.setString(fbData, "/tankData/status", tankStatus);
  Firebase.setFloat(fbData, "/tankData/temperatureC", temperature_C);
  Firebase.setFloat(fbData, "/tankData/temperatureF", temperature_F);

  // ===== Serial Monitor =====
  Serial.println("===== FINAL TANK DATA =====");
  Serial.print("Distance: "); Serial.print(distance_cm); Serial.println(" cm");
  Serial.print("Fuel: "); Serial.print(fuelLiters); Serial.println(" Liters");
  Serial.print("Percentage: "); Serial.print(waterPercent); Serial.println(" %");
  Serial.print("STATUS: "); Serial.println(tankStatus);
  Serial.println("===========================\n");

  delay(2000);
}
