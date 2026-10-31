#include "StatusLed.h"

StatusLed::StatusLed(uint8_t pin) : pin_(pin)
{
}

void StatusLed::begin()
{
    pinMode(pin_, OUTPUT);
    state_ = false;
    digitalWrite(pin_, LOW);
    lastBlinkAt_ = millis();
}

void StatusLed::update(bool hasReliableFix)
{
    if (hasReliableFix)
    {
        state_ = true;
        lastBlinkAt_ = millis();
        digitalWrite(pin_, HIGH);
        return;
    }

    if (millis() - lastBlinkAt_ >= 500)
    {
        lastBlinkAt_ = millis();
        state_ = !state_;
        digitalWrite(pin_, state_);
    }
}
