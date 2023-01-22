#include <Arduino.h>
#include <Wire.h>
#include "CCS811.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <Adafruit_SSD1351.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Font5x7Fixed.h>
// #include <U8g2lib.h>
#include <SPI.h>
#include <helpers.h>

// OLED
#define OLED_WIDTH  128
#define OLED_HEIGHT 128

#define OLED_MOSI 13
#define OLED_CLK  14
#define OLED_DC    0
#define OLED_CS   15
#define OLED_RESET 2

// Color definitions
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0  
#define WHITE   0xFFFF

// U8G2_SH1106_128X64_WINSTAR_1_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ OLED_CS, /* dc=*/ OLED_DC, /* reset=*/ OLED_RESET);
// Adafruit_SSD1351 display = Adafruit_SSD1351(OLED_WIDTH, OLED_HEIGHT, OLED_CS, OLED_DC, OLED_MOSI, OLED_CLK, OLED_RESET);

// Hardware SPI
Adafruit_SSD1351 display = Adafruit_SSD1351(OLED_WIDTH, OLED_HEIGHT, &SPI, OLED_CS, OLED_DC, OLED_RESET);

// DHT11 temp and humidity sensor
#define DHTPIN 3
#define DHTTYPE DHT11
DHT_Unified dht(DHTPIN, DHTTYPE);

// CCS811 air quality sensor
#define CCS_SDA 4
#define CCS_SCL 5
CCS811 ccs811(D3);

uint16_t temp, hum, co2, tvoc;
uint8_t ok;

void setup() {

  // Setup serial
  Serial.begin(115200);

  // Setup U8g2
  // bool ok = u8g2.begin();
  // if ( !ok ) Serial.println(F("u8g2 begin failed"));

  // Setup SSD1351
  display.begin();
  display.setFont(&Font5x7Fixed);
  display.setTextColor(WHITE);

  display.fillRect(0, 0, 128, 128, BLACK);

  // Setup DHT11
  dht.begin();
  
  // Setup I2C
  Wire.begin();

  // Setup CCS811
  ccs811.set_i2cdelay(50);
  ok = ccs811.begin();
  if ( !ok ) Serial.println(F("ccs811 begin failed"));

  // Device information
  // F() causes the string to be stored in flash memory and casts it to a helper class???
  // I believe this means that the string takes longer to retrieve, but doesn't use ram
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("Temperature sensor information"));
  Serial.print(F("Type: ")); Serial.println(sensor.name);
  Serial.print(F("Ver : ")); Serial.println(sensor.version);
  Serial.print(F("ID  : ")); Serial.println(sensor.sensor_id);
  
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity sensor information"));
  Serial.print(F("Type: ")); Serial.println(sensor.name);
  Serial.print(F("Ver : ")); Serial.println(sensor.version);
  Serial.print(F("ID  : ")); Serial.println(sensor.sensor_id);

  Serial.println(F("CCS811 information"));
  Serial.println(CCS811_VERSION);
  Serial.print(F("Hardware ver   : ")); Serial.println(ccs811.hardware_version(), HEX);
  Serial.print(F("Bootloader ver : ")); Serial.println(ccs811.bootloader_version(), HEX);
  Serial.print(F("Application ver: ")); Serial.println(ccs811.application_version(), HEX);

  ok = ccs811.start(CCS811_MODE_1SEC);
  if ( !ok ) Serial.println(F("start failed"));

  // Setup permanent display elements
  display.fillRect(0, 0, OLED_WIDTH, 24, RED);
  printCentred("CUMMER", OLED_WIDTH/2, 20);
}

void loop() {
  // Get event from DHT sensor
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Temperature error"));
  }
  else {
    temp = event.temperature;
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
  }

  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Humidity error"));
  }
  else {
    hum = event.relative_humidity;
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
  }

  // Get information from CCS811
  uint16_t eco2, etvoc, errstat, raw;
  ccs811.read(&eco2, &etvoc, &errstat, &raw);

  if (errstat == CCS811_ERRSTAT_OK) {
    co2 = eco2;
    tvoc = etvoc;

    Serial.print(F("CCS811: "));
    Serial.print(F("eco2 :"));
    Serial.print(co2);
    Serial.println(F(" ppm"));
    Serial.print(F("etvoc: "));
    Serial.print(tvoc);
    Serial.println(F(" ppb"));

  } else if (errstat == CCS811_ERRSTAT_OK_NODATA) {
    Serial.println(F("CCS811: Waiting for data"));
  } else if (errstat & CCS811_ERRSTAT_I2CFAIL) {
    Serial.println(F("CCS811: I2C error"));
  } else {
    Serial.print(F("CCS811: errstat="));
    Serial.print(errstat, HEX);
    Serial.print(F("="));
    Serial.println(ccs811.errstat_str(errstat));
  }

  // SSD1351
  display.setFont(&Font5x7Fixed);

  display.fillRect(0, 127-14, 40, 127, GREEN);
  display.setCursor(0, 127-1);
  display.println(tvoc);

  // Test getTextBounds()

  printCentred("I AM CUM", OLED_WIDTH/2, OLED_HEIGHT/2);

  delay(1000);
}

void printCentred(const char *stri, int x, int y)
{
    display.setTextWrap(false);
    int16_t x0, y0;
    uint16_t w, h;
    display.getTextBounds(stri, x, y, &x0, &y0, &w, &h); //calc width of new string
    display.setCursor(x - (w/2), y);
    display.print(stri);
}
