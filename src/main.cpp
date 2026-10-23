#include <Arduino.h>
#include <TFT_eSPI.h>
#include <ArduinoJson.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(27, OUTPUT);
  digitalWrite(27, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);

  const int centerX = tft.width() / 2;
  const int centerY = tft.height() / 2; 
  const int margin = 20;
  const int panelWidth = 210;
  const int panelHeight = 180;

  const int bar_width = 170;
  const int bar_height = 12;
  const int innerWidth = bar_width - 2;
  const int innerHeight = bar_height - 2;
  const float cpu_usage = 0; // Example CPU usage percentage 
  const float ram_usage = 0; // Example RAM usage percentage

  tft.drawString("PC SMART DASHBOARD", centerX, margin, 2);

  tft.drawRect(margin, centerY - panelHeight / 2, panelWidth, panelHeight, TFT_BLUE );
  tft.drawString("CPU", margin + panelWidth / 2, 110, 2);
  tft.drawString(String(cpu_usage, 0) + "%", margin + panelWidth / 2, 150, 2);

  tft.drawRect(centerX + 10, centerY - panelHeight / 2, panelWidth, panelHeight, TFT_GREEN );
  tft.drawString("RAM", centerX + panelWidth / 2 + 10, 110, 2);
  tft.drawString(String(ram_usage, 0) + "%", centerX + panelWidth / 2 + 10, 150, 2);

  tft.drawRect(margin + (panelWidth - bar_width) / 2, 210, bar_width, bar_height, TFT_BLUE);
  if (cpu_usage > 0)
  {
    tft.fillRect(margin + (panelWidth - bar_width) / 2 + 1, 210 + 1, (innerWidth * cpu_usage) / 100, innerHeight, TFT_BLUE);
  }
  tft.drawRect(centerX + 10 + (panelWidth - bar_width) / 2, 210, bar_width, bar_height, TFT_GREEN);
  if (ram_usage > 0)
  {
    tft.fillRect(centerX + 10 + (panelWidth - bar_width) / 2 + 1, 210 + 1, (innerWidth * ram_usage) / 100, innerHeight, TFT_GREEN);
  }
}


void loop() {
  if (Serial.available() > 0) {
      String message = Serial.readStringUntil('\n');
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, message);
      if (error) {
        tft.drawString("JSON ERROR", tft.width() / 2, 300, 2);
        return;
      }
      float cpu = doc["cpu"].as<float>();
      float ram = doc["ram"].as<float>();
      String info = String("CPU: ") + String(cpu, 0) + "% RAM: " + String(ram, 0) + "%";
      tft.fillRect(0, 280, tft.width(), 40, TFT_BLACK);
      tft.drawString(info, tft.width() / 2, 300, 2);
  }
}


