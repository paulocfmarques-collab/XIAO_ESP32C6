#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "GpsManager.h"
#include "LocalityService.h"
#include "WeatherService.h"

class WifiManager
{
  public:
    void begin();
    void update();
    void setPosition(const GpsReading& gps, const LocalityReading& locality);
    void setWeather(const WeatherReading& weather) { weather_ = weather; }
    bool wantsLocality() const;
    String savedSsid(uint8_t index) const { return index < 5 ? ssids_[index] : String(); }
    bool otaAvailable() const { return otaStarted_; }

  private:
    void showPage();
    void showPosition();
    void save();
    void startPortal();
    void tryNextNetwork();
    WebServer server_{80};
    String ssids_[5];
    String passwords_[5];
    uint8_t nextNetwork_ = 0;
    bool connecting_ = false;
    bool connected_ = false;
    bool portal_ = false;
    bool restarting_ = false;
    bool otaStarted_ = false;
    GpsReading gps_;
    LocalityReading locality_;
    WeatherReading weather_;
    bool webPositionSeen_ = false;
    unsigned long webPositionAt_ = 0;
    unsigned long attemptAt_ = 0;
    unsigned long retryAt_ = 0;
    unsigned long restartAt_ = 0;
};
