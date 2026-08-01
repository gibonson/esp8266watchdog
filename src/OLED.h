// #ifndef OLED_H
// #define OLED_H

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// LCD Configuration
#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // If not work please try 0x3D
#define OLED_SDA D5         // Stock firmware shows wrong pins
#define OLED_SCL D6         // They swap SDA with SCL ;)
Adafruit_SSD1306 *display;

void initOLED()
{
  display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  // OLED used nonstandard SDA and SCL pins
  Wire.begin(D5, D6);
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!display->begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
}

void updateOLED(String line1, String line2, String line3, String line4, String line5, String line6, String line7, String line8)
{
  if (display == nullptr)
    return;

  display->clearDisplay();
  display->setTextSize(1); // scale 1:1
  display->setTextColor(SSD1306_WHITE);
  display->setCursor(0, 0);
  display->println(line1);
  display->println(line2);
  display->println(line3);
  display->println(line4);
  display->println(line5);
  display->println(line6);
  display->println(line7);
  display->println(line8);

  display->display(); // send to oled
}

// #endif // OLED_H