#pragma once

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include "LocalityService.h"
#include "WeatherService.h"

struct GpsReading;

class GpsDisplay
{
  public:
    GpsDisplay(uint8_t sdaPin, uint8_t sclPin);

    void begin();
    void update(const GpsReading& reading, const LocalityReading& locality, const WeatherReading& weather);
    bool wantsLocality(const GpsReading& reading) const;
    bool wantsWeather(const GpsReading& reading) const;
    void nextPage(const GpsReading& reading);
    void turnOff();
    void wake();
    bool isOff() const;

  private:
    enum class Page : uint8_t
    {
        Clock,
        Position,
        Motion,
        Searching,
        Network,
        Locality,
        Weather
    };

    bool drawSplash(unsigned long now);
    void drawScreenSaver(unsigned long elapsed);
    void drawNoData();
    void drawSearching(const GpsReading& reading, unsigned long now);
    void drawClock(const GpsReading& reading);
    void drawPosition(const GpsReading& reading);
    void drawMotion(const GpsReading& reading);
    void drawNetwork(const GpsReading& reading);
    void drawLocality(const GpsReading& reading, const LocalityReading& locality);
    void drawWeather(const GpsReading& reading, const WeatherReading& weather);
    uint8_t pageCount(const GpsReading& reading) const;
    Page currentPage(const GpsReading& reading) const;
    void drawHeader(const char* title, const uint8_t* bitmap, const GpsReading& reading);
    void drawFooter(uint8_t page, uint8_t count);
    void drawText(uint8_t x, uint8_t y, const char* text, uint8_t size = 1);

    static const char* compassPoint(double degrees);
    static const char* precisionLabel(const GpsReading& reading);

    Adafruit_SSD1306 display_;
    uint8_t sdaPin_;
    uint8_t sclPin_;
    bool ready_ = false;
    bool splashDone_ = false;
    bool screenSaverActive_ = false;
    bool displayOff_ = false;
    unsigned long startedAt_ = 0;
    unsigned long lastInteractionAt_ = 0;
    unsigned long lastRefreshAt_ = 0;
    unsigned long searchStartedAt_ = 0;
    uint8_t pageIndex_ = 0;
    uint8_t animationFrame_ = 0;
};
