#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "DHT.h"
#include <SPI.h>
#include <Adafruit_BMP280.h>

#define VERSION "1.0"

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define DHTPIN 0
#define DHTTYPE DHT22

#define BMP_SCK  (13)
#define BMP_MISO (12)
#define BMP_MOSI (11)
#define BMP_CS   (10)


// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
DHT dht(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;


void setup() {
  Serial.begin(115200);
  dht.begin();
  initBMP280();
  initDisplay();
  messaggioBenvenuto();  

}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float p = bmp.readPressure();
  float a = bmp.readAltitude(1015.5);

  if (isnan(h) || isnan(t) || isnan(p) || isnan(a)) {
    Serial.println(F("Failed to read from sensor!"));
    //return;
  }else
  {
    printSensor(h, t, p, a);
  }


  delay(2000);
  
}

void initDisplay() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
}

void messaggioBenvenuto() {
  display.setCursor(0, 10);
  // Display static text
  display.print("Che Aria\nChe Tira\n");
  display.println(VERSION);
  display.display(); 
  delay(3000);
  display.clearDisplay();
}

void printSensor(float h, float t, float p, float a)
{
  display.setCursor(0, 0);
  display.clearDisplay();
  display.print("Umidita': ");
  display.print(h);
  display.println(" %");
  display.println();
  display.print("Temperatura: ");
  display.print(t);
  display.println(" C");
  display.println();
  display.print("Pressione: ");
  display.print(p/100000, 3);
  display.println(" Bar"); 
  display.println();
  display.print("Altitudine: ");
  display.print((int)(a));
  display.print(" mt"); 
  display.println();
  display.display(); 
  delay(2000);
}

void initBMP280()
{
  unsigned status = bmp.begin(0x76);
  if (!status) {
    Serial.println(F("Could not find a valid BMP280 sensor, check wiring or "
                      "try a different address!"));
    Serial.print("SensorID was: 0x"); Serial.println(bmp.sensorID(),16);
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