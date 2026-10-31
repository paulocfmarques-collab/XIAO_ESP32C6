#pragma once

#include <Arduino.h>

class PageButton
{
  public:
    enum class Event : uint8_t
    {
        None,
        Pressed,
        ShortPress,
        LongPress
    };

    explicit PageButton(uint8_t pin) : pin_(pin)
    {
    }

    void begin()
    {
        pinMode(pin_, INPUT_PULLUP);
        lastReading_ = digitalRead(pin_);
        stableState_ = lastReading_;
        lastChangedAt_ = millis();
        pressedAt_ = lastChangedAt_;
        longPressSent_ = false;
    }

    Event update()
    {
        unsigned long now = millis();
        int reading = digitalRead(pin_);
        if (reading != lastReading_)
        {
            lastReading_ = reading;
            lastChangedAt_ = now;
        }

        if (reading != stableState_ && now - lastChangedAt_ >= 40)
        {
            stableState_ = reading;
            if (stableState_ == LOW)
            {
                pressedAt_ = now;
                longPressSent_ = false;
                return Event::Pressed;
            }
            return longPressSent_ ? Event::None : Event::ShortPress;
        }
        if (stableState_ == LOW && reading == LOW && !longPressSent_ &&
            now - pressedAt_ >= 2000)
        {
            longPressSent_ = true;
            return Event::LongPress;
        }
        return Event::None;
    }

  private:
    uint8_t pin_;
    int lastReading_ = HIGH;
    int stableState_ = HIGH;
    unsigned long lastChangedAt_ = 0;
    unsigned long pressedAt_ = 0;
    bool longPressSent_ = false;
};
