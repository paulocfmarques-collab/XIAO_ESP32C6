#include "GpsManager.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sys/time.h>

GpsManager::GpsManager(uint8_t rxPin, uint8_t txPin, bool daylightSavingTime)
    : serial_(1), rxPin_(rxPin), txPin_(txPin), daylightSavingTime_(daylightSavingTime)
{
}

void GpsManager::begin()
{
    if (setenv("TZ", daylightSavingTime_ ? "BRT2" : "BRT3", 1) != 0)
    {
        Serial.printf("Falha ao configurar fuso do relogio: %s\n", strerror(errno));
    }
    tzset();
    serial_.begin(9600, SERIAL_8N1, rxPin_, txPin_);
}

void GpsManager::update()
{
    while (serial_.available())
    {
        char character = static_cast<char>(serial_.read());
        if (character == '$')
        {
            echoingNmea_ = !hasReliableFix();
        }

        if (echoingNmea_)
        {
            Serial.write(static_cast<uint8_t>(character));
        }

        if (gps_.encode(character))
        {
            syncClock();
        }

        if (character == '\n')
        {
            echoingNmea_ = false;
        }
    }
}

void GpsManager::syncClock()
{
    unsigned long now = millis();
    if ((clockSynced_ && now - lastClockSyncAt_ < 60000) ||
        !gps_.date.isValid() || !gps_.time.isValid() ||
        !gps_.date.isUpdated() || !gps_.time.isUpdated() ||
        gps_.date.age() > 1000 || gps_.time.age() > 1000)
    {
        return;
    }

    int year = gps_.date.year();
    int month = gps_.date.month();
    int day = gps_.date.day();
    int hour = gps_.time.hour();
    int minute = gps_.time.minute();
    int second = gps_.time.second();
    bool leapYear = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    static const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (year < 2020 || year > 2099 || month < 1 || month > 12 || day < 1 ||
        day > daysInMonth[month - 1] + (month == 2 && leapYear ? 1 : 0) ||
        hour > 23 || minute > 59 || second > 59)
    {
        Serial.println("Data/hora GPS invalida; relogio nao sincronizado.");
        return;
    }

    int64_t days = 0;
    for (int currentYear = 1970; currentYear < year; ++currentYear)
    {
        days += (currentYear % 4 == 0 && currentYear % 100 != 0) ||
                        currentYear % 400 == 0 ? 366 : 365;
    }
    for (int currentMonth = 1; currentMonth < month; ++currentMonth)
    {
        days += daysInMonth[currentMonth - 1] + (currentMonth == 2 && leapYear ? 1 : 0);
    }
    days += day - 1;
    timeval value = {};
    value.tv_sec = static_cast<time_t>(days * 86400 + hour * 3600 + minute * 60 + second);
    value.tv_usec = gps_.time.centisecond() * 10000;
    if (settimeofday(&value, nullptr) != 0)
    {
        Serial.printf("Falha ao sincronizar relogio com GPS: %s\n", strerror(errno));
        return;
    }
    if (!clockSynced_)
    {
        Serial.println("Relogio do ESP32 sincronizado com GPS (UTC).");
    }
    clockSynced_ = true;
    lastClockSyncAt_ = now;
}

bool GpsManager::hasReliableFix()
{
    return gps_.location.isValid() && gps_.location.age() < 5000 &&
           gps_.satellites.isValid() && gps_.satellites.age() < 5000 &&
           gps_.satellites.value() >= 3;
}

bool GpsManager::hasNoDataTimedOut() const
{
    return millis() > 5000 && gps_.charsProcessed() < 10;
}

GpsReading GpsManager::getReading()
{
    GpsReading reading;
    reading.locationValid = gps_.location.isValid();
    reading.latitude = gps_.location.lat();
    reading.longitude = gps_.location.lng();
    reading.timeValid = gps_.time.isValid();
    reading.localHour = localHour();
    reading.minute = gps_.time.minute();
    reading.second = gps_.time.second();
    reading.dateValid = gps_.date.isValid() && gps_.time.isValid();
    localDate(reading.localDay, reading.localMonth, reading.localYear);
    if (clockSynced_)
    {
        time_t now = time(nullptr);
        tm local = {};
        if (localtime_r(&now, &local) != nullptr)
        {
            reading.timeValid = true;
            reading.dateValid = true;
            reading.localHour = local.tm_hour;
            reading.minute = local.tm_min;
            reading.second = local.tm_sec;
            reading.localDay = local.tm_mday;
            reading.localMonth = local.tm_mon + 1;
            reading.localYear = local.tm_year + 1900;
        }
        else
        {
            Serial.println("Falha ao converter relogio do ESP32 para hora local.");
        }
    }
    reading.utcOffsetHours = utcOffsetHours();
    reading.satellitesValid = gps_.satellites.isValid();
    reading.satellites = gps_.satellites.value();
    reading.altitudeValid = gps_.altitude.isValid();
    reading.altitudeMeters = gps_.altitude.meters();
    reading.speedValid = gps_.speed.isValid();
    reading.speedKmph = gps_.speed.kmph();
    reading.courseValid = gps_.course.isValid();
    reading.courseDegrees = gps_.course.deg();
    reading.hdopValid = gps_.hdop.isValid();
    reading.hdop = gps_.hdop.hdop();
    reading.hasReliableFix = hasReliableFix();
    reading.noDataTimedOut = hasNoDataTimedOut();
    return reading;
}

void GpsManager::printReportIfDue()
{
    if (millis() - lastReportAt_ < 1000)
    {
        return;
    }

    lastReportAt_ = millis();
    if (hasReliableFix() && !echoingNmea_)
    {
        printReport();
    }
}

int GpsManager::utcOffsetHours() const
{
    return daylightSavingTime_ ? -2 : -3;
}

int GpsManager::localHour()
{
    return (gps_.time.hour() + utcOffsetHours() + 24) % 24;
}

void GpsManager::localDate(int& day, int& month, int& year)
{
    day = gps_.date.day();
    month = gps_.date.month();
    year = gps_.date.year();

    // Brazil is behind UTC, so local time may still be on the previous day.
    if (gps_.time.hour() + utcOffsetHours() >= 0 || month < 1 || month > 12)
    {
        return;
    }

    if (--day > 0)
    {
        return;
    }

    if (--month == 0)
    {
        month = 12;
        --year;
    }

    static const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool leapYear = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    day = daysInMonth[month - 1] + (month == 2 && leapYear ? 1 : 0);
}

void GpsManager::printReport()
{
    GpsReading reading = getReading();
    int hour = reading.localHour;
    Serial.println();
    Serial.println("===== GPS =====");

    Serial.print("Latitude : ");
    Serial.println(gps_.location.lat(), 6);

    Serial.print("Longitude: ");
    Serial.println(gps_.location.lng(), 6);

    Serial.print("Satelites: ");
    Serial.println(gps_.satellites.value());

    Serial.print("Altitude : ");
    Serial.print(gps_.altitude.meters());
    Serial.println(" m");

    Serial.print("Data UTC : ");
    Serial.print(gps_.date.day());
    Serial.print("/");
    Serial.print(gps_.date.month());
    Serial.print("/");
    Serial.println(gps_.date.year());

    Serial.print("Hora BR  : ");
    if (hour < 10)
    {
        Serial.print("0");
    }
    Serial.print(hour);
    Serial.print(":");

    if (reading.minute < 10)
    {
        Serial.print("0");
    }
    Serial.print(reading.minute);
    Serial.print(":");

    if (reading.second < 10)
    {
        Serial.print("0");
    }
    Serial.println(reading.second);

    Serial.println("================");
}
