#include "GpsDisplay.h"
#include "GpsManager.h"
#include "PageButton.h"
#include "StatusLed.h"

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
unsigned long lastNoDataReportAt = 0;
bool wakingPress = false;
} // namespace

void setup()
{
    Serial.begin(115200);
    delay(500);
    gpsManager.begin();
    statusLed.begin();
    pageButton.begin();

    Serial.println("GPS iniciado...");
    gpsDisplay.begin();
}

void loop()
{
    gpsManager.update();
    statusLed.update(gpsManager.hasReliableFix());
    gpsManager.printReportIfDue();
    GpsReading reading = gpsManager.getReading();
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
    gpsDisplay.update(reading);

    unsigned long now = millis();
    if (reading.noDataTimedOut && now - lastNoDataReportAt >= 1000)
    {
        lastNoDataReportAt = now;
        Serial.println("Nenhum dado recebido do GPS!");
    }
}
