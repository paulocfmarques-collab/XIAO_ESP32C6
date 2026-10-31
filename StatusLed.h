#pragma once

#include <Arduino.h>

class StatusLed
{
  public:
    explicit StatusLed(uint8_t pin);

    void begin();
    void update(bool hasReliableFix);

  private:
    uint8_t pin_;
    unsigned long lastBlinkAt_ = 0;
    bool state_ = false;
};
