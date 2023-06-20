#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char *ssid = "Daff";                     // Sesuaikan dengan SSID WiFi Anda
const char *password = "lengser12";            // Sesuaikan dengan password WiFi Anda
const char *mqtt_server = "broker.hivemq.com"; // Sesuaikan dengan alamat server MQTT Anda

WiFiClient espClient;
PubSubClient client(espClient);

bool pumpStatus = false;

const int moisturePin = A0; // Digital pin connected to the moisture sensor
const int pumpPin = D6;     // Digital pin connected to the pump

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client"))
    {
      Serial.println("connected");
      client.subscribe("kelompok2/onof"); // Subscribing to the topic for pump control
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void handlePumpControl(char *payload, int length)
{
  String message = "";
  for (int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  if (message.equals("pump_on"))
  {
    digitalWrite(pumpPin, HIGH);
    pumpStatus = true;
  }
  else if (message.equals("pump_off"))
  {
    digitalWrite(pumpPin, LOW);
    pumpStatus = false;
  }
}

void callback(char *topic, byte *payload, int length)
{
  handlePumpControl((char *)payload, length);
}

void setup()
{
  Serial.begin(9600);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  pinMode(moisturePin, INPUT);
  pinMode(pumpPin, OUTPUT);

  lcd.begin(16, 2);
  lcd.init();
  lcd.clear();
  lcd.init();
  lcd.backlight();
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  int moistureValue = analogRead(moisturePin);
  int moisturePercentage = map(moistureValue, 0, 1023, 0, 100);
  int moistureThreshold = 75;

  Serial.print("Moisture: ");
  Serial.print(moisturePercentage);
  lcd.setCursor(0, 0);
  lcd.print("KELEMBABAN");
  lcd.setCursor(11, 0);
  lcd.print(moisturePercentage);
  lcd.setCursor(13, 0);
  lcd.print("%");
  Serial.println("%");
  delay(100);

  if (moisturePercentage >= moistureThreshold)
  {
    if (!pumpStatus)
    {
      pumpStatus = true;
      digitalWrite(pumpPin, HIGH);
      client.publish("kelompok2/onof", "pump_on");
    }
    Serial.println("kering");
    lcd.setCursor(2, 1);
    lcd.print("KERING");
    digitalWrite(pumpPin, HIGH);
    delay(100);

    char buffer[10];
    sprintf(buffer, "%d", moisturePercentage);
    client.publish("kelompok2/moistureStatus", buffer);
  }
  else if (moisturePercentage <= moistureThreshold)
  {
    if (pumpStatus)
    {
      pumpStatus = false;
      digitalWrite(pumpPin, LOW);
      client.publish("kelompok2/onof", "pump_off");
    }
    Serial.println("basah");
    lcd.setCursor(2, 1);
    lcd.print("BASAHH");
    digitalWrite(pumpPin, LOW);
    delay(1000);

    char buffer[10];
    sprintf(buffer, "%d", moisturePercentage);
    client.publish("kelompok2/moistureStatus", buffer);
  }
}
