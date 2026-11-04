#pragma once

#include <Arduino.h>

class StatusLed
{
  public:
    explicit StatusLed(uint8_t pin);

    void begin();
    void update(bool hasReliableFix);
    enum class Mode : uint8_t { Auto, On, Off };
    void setMode(Mode mode) { mode_ = mode; }
    const char* modeName() const;
    bool isOn() const { return state_; }

  private:
    uint8_t pin_;
    unsigned long lastBlinkAt_ = 0;
    bool state_ = false;
    Mode mode_ = Mode::Auto;
};
