#pragma once

#include <WiFiUdp.h>
#include "GpsDisplay.h"
#include "StatusLed.h"
#include "WifiManager.h"

class UdpService
{
  public:
    UdpService(GpsDisplay& display, StatusLed& led, WifiManager& wifi,
               LocalityService& locality, WeatherService& weather);
    void update(const GpsReading& gps);
    bool wantsLocality() const;
    bool wantsWeather() const;
    static constexpr uint16_t PORT = 4210;

  private:
    String execute(const String& command, const GpsReading& gps);
    WiFiUDP socket_;
    GpsDisplay& display_;
    StatusLed& led_;
    WifiManager& wifi_;
    LocalityService& locality_;
    WeatherService& weather_;
    IPAddress stationIp_;
    IPAddress portalIp_;
    bool listening_ = false;
    bool bindAttempted_ = false;
    unsigned long bindAt_ = 0;
    bool localityRequested_ = false;
    bool weatherRequested_ = false;
    unsigned long localityAt_ = 0;
    unsigned long weatherAt_ = 0;
    bool restarting_ = false;
    unsigned long restartAt_ = 0;
};
