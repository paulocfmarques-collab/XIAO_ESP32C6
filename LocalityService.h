#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

struct GpsReading;

struct LocalityReading
{
    bool available = false;
    bool busy = false;
    bool failed = false;
    char city[96] = {};
    char region[96] = {};
    char country[64] = {};
    double latitude = 0;
    double longitude = 0;
};

class LocalityService
{
  public:
    void begin();
    void update(const GpsReading& gps, bool requested);
    const LocalityReading& reading() const { return reading_; }

  private:
    struct Request { double latitude; double longitude; };
    static void worker(void* context);
    QueueHandle_t requests_ = nullptr;
    QueueHandle_t results_ = nullptr;
    LocalityReading reading_;
    bool attempted_ = false;
    unsigned long lastAttemptAt_ = 0;
};
