#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include <secrets.h>

const int ledPin = 25;            // Digital output pin that the LED is attached to
const int pumpPin = 4;            // Digital output pin that the water pump is attached to
const int waterLevelPin = 35;     // Analoge pin water level sensor is connected to
const int moistureSensorPin = 34; // Digital input pin used to check the moisture level of the soil

double checkInterval = 3600000; // time to wait before checking the soil moisture level - default it to an hour
int waterLevelThreshold = 380;  // threshold at which we flash the LED to warn you of a low water level in the pump tank
int emptyReservoirBlinks = 900; // how mant times the LED will flash to tell us the water tank needs topping up - default it to an hour (900 times * 2 seconds)
int amountToPump = 2000;        // how long the pump should pump water for when the plant needs it

int waterLevelValue = 0; // somewhere to store the value read from the waterlevel sensor
int moistureValue = 0;   // somewhere to store the value read from the soil moisture sensor

String ssid = WIFI_SSID;
String password = WIFI_PASSWORD;

String apiUrl = API_URL;
String apiKey = API_KEY;
String metricsTable = "metrics";

int deviceId = 1;

int waterLevelMetricType = 1;
int moistureMetricType = 2;

HTTPClient https;
WiFiClientSecure client;

void send_metric(int metricType, int metricValue)
{
  https.begin(client, apiUrl + "/rest/v1/" + metricsTable);

  https.addHeader("apikey", apiKey);
  https.addHeader("Authorization", "Bearer " + apiKey);
  https.addHeader("Content-Type", "application/json");
  https.addHeader("Prefer", "return=representation");

  int httpCode = https.POST("{\"device_id\":" + String(deviceId) + ",\"type\":" + String(metricType) + ",\"value\":" + String(metricValue) + "}");
  String payload = https.getString();

  Serial.println("Response code: " + String(httpCode));
  Serial.println("Response: " + payload);

  https.end();
}

void setup()
{
  Serial.begin(115200);

  pinMode(ledPin, OUTPUT);
  pinMode(pumpPin, OUTPUT);
  pinMode(moistureSensorPin, INPUT);

  client.setInsecure();

  WiFi.begin(ssid, password);
  Serial.println("Attempting to connect to wifi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected");

  // flash the LED five times to confirm power on and operation of code:
  for (int i = 0; i <= 4; i++)
  {
    digitalWrite(ledPin, HIGH);
    delay(300);
    digitalWrite(ledPin, LOW);
    delay(300);
  }

  delay(2000);

  digitalWrite(ledPin, HIGH);
}

void loop()
{
  waterLevelValue = analogRead(waterLevelPin);
  Serial.print("Water level sensor value: ");
  Serial.println(waterLevelValue);
  send_metric(waterLevelMetricType, waterLevelValue);

  if (waterLevelValue < waterLevelThreshold)
  {
    Serial.print("Notifying low water level");
    for (int i = 0; i <= emptyReservoirBlinks; i++)
    {
      digitalWrite(ledPin, LOW);
      delay(1000);
      digitalWrite(ledPin, HIGH);
      delay(1000);
    }
  }
  else
  {
    digitalWrite(ledPin, HIGH);
    delay(checkInterval); // wait before checking the soil moisture level
  }

  // check soil moisture level
  moistureValue = analogRead(moistureSensorPin);
  Serial.print("(Analog) Soil moisture sensor is currently: ");
  Serial.print(moistureValue);
  send_metric(moistureMetricType, moistureValue);

  moistureValue = digitalRead(moistureSensorPin);
  Serial.print("(Digital) Soil moisture sensor is currently: ");
  Serial.print(moistureValue);
  Serial.println(" ('1' means soil is too dry and '0' means the soil is moist enough.)");

  if (moistureValue == 1)
  {
    digitalWrite(pumpPin, HIGH);
    Serial.println("Pump on");
    delay(amountToPump); // keep pumping water
    digitalWrite(pumpPin, LOW);
    Serial.println("Pump off");
    delay(800); // delay to allow the moisture in the soil to spread through to the sensor
  }
}
