#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  // put your setup code here, to run once:
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

  tft.drawString("PC SMART DASHBOARD", centerX, margin, 2);

  tft.drawRect(margin, centerY - panelHeight / 2, panelWidth, panelHeight, TFT_BLUE );
  tft.drawString("CPU", margin + panelWidth / 2, 110, 2);
  tft.drawString("25%", margin + panelWidth / 2, 150, 2);

  tft.drawRect(centerX + 10, centerY - panelHeight / 2, panelWidth, panelHeight, TFT_GREEN );
  tft.drawString("RAM", centerX + panelWidth / 2 + 10, 110, 2);
  tft.drawString("46%", centerX + panelWidth / 2 + 10, 150, 2);
}

void loop() {
  // put your main code here, to run repeatedly:
}

