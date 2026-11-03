#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

struct GpsReading;
struct ForecastDay
{
    char date[11] = {};
    float minimum = 0;
    float maximum = 0;
    int rain = -1;
    int code = -1;
};
struct WeatherReading
{
    bool available = false;
    bool busy = false;
    bool failed = false;
    double latitude = 0;
    double longitude = 0;
    unsigned long receivedAt = 0;
    ForecastDay days[3];
};

class WeatherService
{
  public:
    void begin();
    void update(const GpsReading& gps, bool requested);
    const WeatherReading& reading() const { return reading_; }
    static const char* description(int code);
    static bool previous(const GpsReading& gps, const WeatherReading& weather);
    static String status(const GpsReading& gps, const WeatherReading& weather);
  private:
    struct Request { double latitude; double longitude; };
    static void worker(void* context);
    QueueHandle_t requests_ = nullptr;
    QueueHandle_t results_ = nullptr;
    WeatherReading reading_;
    bool attempted_ = false;
    unsigned long lastAttemptAt_ = 0;
};
