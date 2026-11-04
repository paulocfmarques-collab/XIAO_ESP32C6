# 1 "C:\\Users\\paulo\\AppData\\Local\\Temp\\tmp5njlni_t"
#include <Arduino.h>
# 1 "C:/PC/ESP32/XIAO_ESP32C6/XIAO_ESP32C6.ino"
#include "GpsDisplay.h"
#include "GpsManager.h"
#include "PageButton.h"
#include "StatusLed.h"
#include "WifiManager.h"
#include "UdpService.h"

namespace
{
constexpr uint8_t GPS_RX_PIN = D7;
constexpr uint8_t GPS_TX_PIN = D6;
constexpr uint8_t OLED_SDA_PIN = D4;
constexpr uint8_t OLED_SCL_PIN = D5;
constexpr uint8_t STATUS_LED_PIN = D8;
constexpr uint8_t PAGE_BUTTON_PIN = D9;
constexpr bool DAYLIGHT_SAVING_TIME = false;

GpsManager gpsManager(GPS_RX_PIN, GPS_TX_PIN, DAYLIGHT_SAVING_TIME);
StatusLed statusLed(STATUS_LED_PIN);
PageButton pageButton(PAGE_BUTTON_PIN);
GpsDisplay gpsDisplay(OLED_SDA_PIN, OLED_SCL_PIN);
WifiManager wifiManager;
LocalityService localityService;
WeatherService weatherService;
UdpService udpService(gpsDisplay, statusLed, wifiManager, localityService, weatherService);
unsigned long lastNoDataReportAt = 0;
bool wakingPress = false;
}
void setup();
void loop();
#line 30 "C:/PC/ESP32/XIAO_ESP32C6/XIAO_ESP32C6.ino"
void setup()
{
    Serial.begin(115200);
    delay(500);
    gpsManager.begin();
    statusLed.begin();
    pageButton.begin();

    Serial.println("GPS iniciado...");
    gpsDisplay.begin();
    wifiManager.begin();
    localityService.begin();
    weatherService.begin();
}

void loop()
{
    gpsManager.update();
    wifiManager.update();
    statusLed.update(gpsManager.hasReliableFix());
    gpsManager.printReportIfDue();
    GpsReading reading = gpsManager.getReading();
    udpService.update(reading);
    PageButton::Event buttonEvent = pageButton.update();
    if (buttonEvent == PageButton::Event::Pressed)
    {
        wakingPress = gpsDisplay.isOff();
        if (wakingPress)
        {
            gpsDisplay.wake();
        }
    }
    else if (buttonEvent == PageButton::Event::LongPress && !wakingPress)
    {
        gpsDisplay.turnOff();
    }
    else if (buttonEvent == PageButton::Event::ShortPress && !wakingPress)
    {
        gpsDisplay.nextPage(reading);
    }
    localityService.update(reading, !weatherService.reading().busy &&
                           (gpsDisplay.wantsLocality(reading) || wifiManager.wantsLocality() || udpService.wantsLocality()));

    weatherService.update(reading, !localityService.reading().busy &&
                          (gpsDisplay.wantsWeather(reading) || wifiManager.wantsLocality() || udpService.wantsWeather()));
    wifiManager.setPosition(reading, localityService.reading());
    wifiManager.setWeather(weatherService.reading());
    gpsDisplay.update(reading, localityService.reading(), weatherService.reading());

    unsigned long now = millis();
    if (reading.noDataTimedOut && now - lastNoDataReportAt >= 1000)
    {
        lastNoDataReportAt = now;
        Serial.println("Nenhum dado recebido do GPS!");
    }
}