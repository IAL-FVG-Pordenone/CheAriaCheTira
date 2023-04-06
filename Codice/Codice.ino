#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define VERSION "1.0"

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels
#define DHTPIN 0
#define DHTTYPE DHT22

#define BMP_SCK (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS (10)

#define sensor s_serial

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;
SoftwareSerial s_serial(14, 12);  // RX, TX


const unsigned char cmd_get_sensor[] = {
  0xff, 0x01, 0x86, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x79
};
unsigned char dataRevice[9];
int temperature;
int CO2PPM;

int count = 0;

// Update these with values suitable for your network.

const char* ssid = "Polo_Pubblica";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(100)
char msg[MSG_BUFFER_SIZE];
int value = 0;

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  display.setCursor(0, 0);
  display.clearDisplay();
  display.print("Connecting to ");
  display.println(ssid);
  display.display();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    display.print(".");
    display.display();
  }

  randomSeed(micros());

  display.println("");
  display.println("WiFi connected");
  display.println("IP address: ");
  display.println(WiFi.localIP());
  display.display();
  delay(2000);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      //client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("cheariachetirain");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  sensor.begin(9600);
  Serial.begin(115200);
  dht.begin();
  initBMP280();
  initDisplay();
  messaggioBenvenuto();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float p = bmp.readPressure();
  float a = bmp.readAltitude(1015.5);
  int c = getCo2();

  if (isnan(h) || isnan(t) || isnan(p) || isnan(a)) {
    Serial.println(F("Failed to read from sensor!"));
    //return;
  } else {
    printSensor(h, t, p, a, c);
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "data:#%ld,um:#%0.2f%%,T:%0.2f°C,p:%0.3fBar,a:%0.2fmt,c:%dppm", value, h, t, p/100000, a, c);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("cheariachetiraout", msg);
  }

  delay(2000);
}

void initDisplay() {
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

void messaggioBenvenuto() {
  display.setCursor(0, 10);
  // Display static text
  display.print("Che Aria Che Tira ");
  display.println(VERSION);
  display.display();
  delay(3000);
  display.clearDisplay();
}

void printSensor(float h, float t, float p, float a, int c) {
  display.setCursor(0, 0);
  display.clearDisplay();
  display.print("Umidita': ");
  display.print(h);
  display.println(" %");
  //display.println();
  display.print("Temperatura: ");
  display.print(t);
  display.println(" C");
  //display.println();
  display.print("Pressione: ");
  display.print(p / 100000, 3);
  display.println(" Bar");
  //display.println();
  display.print("Altitudine: ");
  display.print((int)(a));
  display.println(" mt");

  display.print("CO2: ");
  display.print(c);
  display.println(" ppm");
  //display.println();
  display.display();
}

void initBMP280() {
  unsigned status = bmp.begin(0x76);
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                     "try a different address!"));
    Serial.print("SensorID was: 0x");
    Serial.println(bmp.sensorID(), 16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
    /* Default settings from datasheet. */
    bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
                    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
                    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
                    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
                    Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */
  }
}

int getCo2(void) {
  byte data[10] = {0,0,0,0,0,0,0,0,0, 0};
  int i = 0;
  //transmit command data
  for (i = 0; i < sizeof(cmd_get_sensor); i++) {
    sensor.write(cmd_get_sensor[i]);
  }
  delay(10);
  //begin reveiceing data
  if (sensor.available()) {
    data[9] = 0x55;
    int i = 0;
    while (sensor.available()) {
      data[i] = sensor.read();
      
      if(i > 0)
      {
        if(data[0] == 0xff && data[1] == 0x86)
        {
          i++;
        }
        else
        {
          i = 1;
          data[0] = data[1];
          data[1] = 0;
        }
      }
      else
      {
        if (data[0] == 0xFF) i++;
      }
    }
  }

  return (int)data[2] * 256 + (int)data[3];
}

void printRX(byte *data) {
  Serial.println("Start\tCommand\tHigh\tLow\tB4\tB5\tB6\tB7\tChs\tTest\tsum");
  for(int i = 0; i < 10; i++)
  {
    Serial.print(data[i], HEX);
    Serial.print("\t");
  }
  Serial.print(1 + (0xFF ^ (byte)(data[1] + data[2] + data[3] + data[4] + data[5] + data[6] + data[7])), HEX);
  Serial.println();
}