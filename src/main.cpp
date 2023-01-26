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
#include <SPI.h>
#include <helpers.h>
#include <config.h> // Sensitive info
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

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

// Clock 
// std::String is better than const char * here because we wont have to allocate memory and be annoyed 
String weekDays[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
String months[12]  = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

// WiFi
WiFiUDP ntpUDP;
// SU NTP Pool, offset for UTC+8, and interval of once per minute
NTPClient timeClient(ntpUDP, "au.pool.ntp.org", 8*3600, 60000); 

uint16_t temp, hum, co2, tvoc;
uint8_t ok;

void setup() {

	// Setup serial
	Serial.begin(115200);

	// Setup WiFi
	Serial.print("Connecting to ");
	Serial.println(WIFI_SSID);
	WiFi.hostname("Big Slammer");
	WiFi.begin(WIFI_SSID, WIFI_PASS);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print("");
	}
	timeClient.begin();
	// Australian Western Standard Time = UTC+8
	// timeClient.setTimeOffset(8*60*60);

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
	// 24 pixel tall banner across top of screen
	display.setTextSize(2);
	display.fillRect(0, 0, OLED_WIDTH, 24, RED);
	// Banner thickness 24, text height 14, divide by 2 for equal thickness top and bottom of text, minus from 24 to reference bottom for the print
	printCentred("I AM CLOCK", OLED_WIDTH/2, 24 - ((24-14)/2));
}

void loop() {
	// Used to check for change of day
	uint8_t lastDay;

	// Update time and get structure
	timeClient.update();
	uint64_t epoch = timeClient.getEpochTime();
	struct tm *timeinfo = gmtime((time_t *)&epoch);

	uint8_t second_ = timeinfo->tm_sec;
	uint8_t minute_ = timeinfo->tm_min;
	uint8_t hour_   = timeinfo->tm_hour;
	uint8_t day_    = timeinfo->tm_mday;
	String  wday_ 	= weekDays[timeinfo->tm_wday];
	String  month_ 	= months[timeinfo->tm_mon];
	uint16_t year_ 	= 1900 + timeinfo->tm_year; // Years since 1900

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
	
	// Time
	display.setTextSize(2);
	display.fillRect(0, 24, OLED_WIDTH, 32, BLACK);
	String time_str = String(hour_) + String(":") + String(minute_) + String(":") + String(second_);
	printCentred(time_str.c_str(), OLED_WIDTH/2,  24+32-2);

	// Date
	display.setTextSize(1);
	display.fillRect(0, 32+24, OLED_WIDTH, 9, BLACK);
	// Only update date if it's a new day
	String date_str = String(wday_) + String(" ") + String(day_) + String(month_) + String(" ") + String(year_);
	printCentred(date_str.c_str(), OLED_WIDTH/2, (9 - (9 - 7)/2) + 32+24);
	
	// if (day_ != lastDay) {
	// 	String date_str = String(wday_) + String(day_) + String(" - ") + String(month_) + String(year_);
	// 	printCentred(date_str.c_str(), OLED_WIDTH/2, 24+30);
	// }
  
	// TVOC
	display.setTextSize(1);
	display.setCursor(0, 126);
	display.fillRect(0, 113, 40, 127, GREEN);
	display.println(tvoc);

	lastDay = day_;
	delay(1000);
}

void printCentred(const char *stri, int x, int y) {
	display.setTextWrap(false);
	int16_t x0, y0;
	uint16_t w, h;
	display.getTextBounds(stri, x, y, &x0, &y0, &w, &h); //calc width of new string
	display.setCursor(x - (w/2), y);
	display.print(stri);
}
