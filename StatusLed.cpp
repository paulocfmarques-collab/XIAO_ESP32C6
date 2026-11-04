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
    if (mode_ != Mode::Auto)
    {
        state_ = mode_ == Mode::On;
        digitalWrite(pin_, state_ ? HIGH : LOW);
        lastBlinkAt_ = millis();
        return;
    }
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

const char* StatusLed::modeName() const
{
    if (mode_ == Mode::On) return "ligado";
    if (mode_ == Mode::Off) return "desligado";
    return "automatico";
}
