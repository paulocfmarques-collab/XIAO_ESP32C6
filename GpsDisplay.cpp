#include "GpsDisplay.h"
#include "GpsManager.h"
#include <WiFi.h>

#include <cmath>
#include <cstdio>
#include <cstring>

namespace
{
constexpr uint8_t SCREEN_WIDTH = 128;
constexpr uint8_t SCREEN_HEIGHT = 64;
constexpr unsigned long REFRESH_INTERVAL_MS = 250;
constexpr unsigned long SPLASH_DURATION_MS = 2500;
constexpr unsigned long SCREEN_SAVER_TIMEOUT_MS = 60000;
const uint8_t SATELLITE_ICON[] = {0xC3, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0xC3, 0x01};
const uint8_t CLOCK_ICON[] = {0x3C, 0x42, 0x91, 0x91, 0x9D, 0x81, 0x42, 0x3C};
const uint8_t PIN_ICON[] = {0x3C, 0x42, 0x5A, 0x5A, 0x42, 0x24, 0x18, 0x18};
const uint8_t SPEED_ICON[] = {0x3C, 0x42, 0x81, 0x89, 0x91, 0xA1, 0x81, 0xFF};
const uint8_t WARNING_ICON[] = {0x18, 0x24, 0x24, 0x42, 0x5A, 0x81, 0x99, 0xFF};
} // namespace

GpsDisplay::GpsDisplay(uint8_t sdaPin, uint8_t sclPin)
    : display_(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1), sdaPin_(sdaPin), sclPin_(sclPin)
{
}

void GpsDisplay::begin()
{
    if (!Wire.begin(sdaPin_, sclPin_))
    {
        Serial.println("Falha ao iniciar I2C do OLED. GPS continua pela serial.");
        return;
    }

    uint8_t address = 0;
    constexpr uint8_t addresses[] = {0x3C, 0x3D};
    for (uint8_t candidate : addresses)
    {
        Wire.beginTransmission(candidate);
        if (Wire.endTransmission() == 0)
        {
            address = candidate;
            break;
        }
    }
    if (address == 0)
    {
        Serial.println("OLED nao encontrado em 0x3C/0x3D. Verifique SDA D4, SCL D5 e alimentacao.");
        return;
    }

    if (!display_.begin(SSD1306_SWITCHCAPVCC, address, false, false))
    {
        Serial.println("Falha ao iniciar SSD1306. GPS continua pela serial.");
        return;
    }

    display_.setRotation(2);
    display_.setTextColor(SSD1306_WHITE);
    display_.setTextSize(1);
    display_.setTextWrap(false);
    display_.clearDisplay();

    ready_ = true;
    startedAt_ = millis();
    lastInteractionAt_ = startedAt_;
    lastRefreshAt_ = startedAt_;
    searchStartedAt_ = startedAt_;
    drawSplash(startedAt_);
    Serial.print("OLED SSD1306 128x64 iniciado (I2C 0x");
    Serial.print(address, HEX);
    Serial.println(", SDA D4, SCL D5).");
}

void GpsDisplay::update(const GpsReading& reading, const LocalityReading& locality, const WeatherReading& weather)
{
    if (!ready_)
    {
        return;
    }

    unsigned long now = millis();
    if (reading.hasReliableFix)
    {
        searchStartedAt_ = now;
    }

    if (displayOff_)
    {
        return;
    }

    if (now - lastRefreshAt_ < REFRESH_INTERVAL_MS)
    {
        return;
    }
    lastRefreshAt_ = now;
    ++animationFrame_;

    if (drawSplash(now))
    {
        return;
    }

    display_.clearDisplay();
    if (screenSaverEnabled_ && now - lastInteractionAt_ >= SCREEN_SAVER_TIMEOUT_MS)
    {
        screenSaverActive_ = true;
        drawScreenSaver(now - lastInteractionAt_ - SCREEN_SAVER_TIMEOUT_MS);
        return;
    }

    if (reading.noDataTimedOut && currentPage(reading) != Page::Network &&
        currentPage(reading) != Page::Locality && currentPage(reading) != Page::Weather)
    {
        drawNoData();
        drawFooter(0, pageCount(reading));
        display_.display();
        return;
    }

    switch (currentPage(reading))
    {
    case Page::Clock:
        drawClock(reading);
        break;
    case Page::Position:
        drawPosition(reading);
        break;
    case Page::Motion:
        drawMotion(reading);
        break;
    case Page::Searching:
        drawSearching(reading, now);
        break;
    case Page::Network:
        drawNetwork(reading);
        break;
    case Page::Locality:
        drawLocality(reading, locality);
        break;
    case Page::Weather:
        drawWeather(reading, weather);
        break;
    }
}

bool GpsDisplay::drawSplash(unsigned long now)
{
    if (splashDone_)
    {
        return false;
    }

    unsigned long elapsed = now - startedAt_;
    if (elapsed >= SPLASH_DURATION_MS)
    {
        splashDone_ = true;
        return false;
    }

    display_.clearDisplay();
    display_.drawBitmap(4, 6, SATELLITE_ICON, 8, 8, SSD1306_WHITE);
    drawText(20, 6, "GPS XIAO C6");
    drawText(10, 23, "EXPLORAR. LOCALIZAR.");
    display_.drawRect(4, 40, 120, 8, SSD1306_WHITE);

    uint8_t progressWidth = static_cast<uint8_t>(elapsed * 116 / SPLASH_DURATION_MS);
    if (progressWidth > 0)
    {
        display_.fillRect(6, 42, progressWidth, 4, SSD1306_WHITE);
    }
    drawText(22, 55, "Iniciando GPS");

    display_.display();
    return true;
}

void GpsDisplay::drawNoData()
{
    display_.drawBitmap(60, 3, WARNING_ICON, 8, 8, SSD1306_WHITE);
    drawText(25, 19, "SEM DADOS GPS");
    display_.drawFastHLine(8, 31, 112, SSD1306_WHITE);
    drawText(16, 38, "Verifique os fios");
    drawText(28, 47, "UART: D6/D7");
    display_.display();
}

void GpsDisplay::drawScreenSaver(unsigned long elapsed)
{
    unsigned long frame = elapsed / REFRESH_INTERVAL_MS;
    uint8_t horizontal = (frame / 2) % 72;
    uint8_t vertical = (frame / 3) % 16;
    int16_t x = 5 + (horizontal <= 36 ? horizontal : 72 - horizontal);
    int16_t y = 9 + (vertical <= 8 ? vertical : 16 - vertical);
    uint8_t expression = (elapsed / 4000) % 4;
    bool blink = frame % 16 == 14;
    int16_t gaze = static_cast<int16_t>((frame / 5) % 3) - 1;

    display_.drawRoundRect(x, y, 80, 40, 8, SSD1306_WHITE);
    display_.drawFastVLine(x + 40, y - 5, 5, SSD1306_WHITE);
    display_.fillCircle(x + 40, y - 6, 2, SSD1306_WHITE);
    display_.drawRoundRect(x - 4, y + 13, 4, 14, 1, SSD1306_WHITE);
    display_.drawRoundRect(x + 80, y + 13, 4, 14, 1, SSD1306_WHITE);

    for (uint8_t eye = 0; eye < 2; ++eye)
    {
        int16_t eyeX = x + 17 + eye * 36;
        if (blink || (expression == 2 && eye == 1))
        {
            display_.drawFastHLine(eyeX, y + 18, 12, SSD1306_WHITE);
            display_.drawFastHLine(eyeX + 2, y + 19, 8, SSD1306_WHITE);
        }
        else if (expression == 1)
        {
            display_.drawLine(eyeX, y + 18, eyeX + 6, y + 12, SSD1306_WHITE);
            display_.drawLine(eyeX + 6, y + 12, eyeX + 12, y + 18, SSD1306_WHITE);
        }
        else
        {
            display_.fillRoundRect(eyeX + gaze, y + 10, 12, 12, 3, SSD1306_WHITE);
            display_.fillRect(eyeX + gaze + 4, y + 13, 4, 6, SSD1306_BLACK);
        }
    }

    if (expression == 3)
    {
        display_.drawCircle(x + 40, y + 30, 4, SSD1306_WHITE);
    }
    else
    {
        display_.drawLine(x + 29, y + 28, x + 34, y + 32, SSD1306_WHITE);
        display_.drawFastHLine(x + 34, y + 32, 12, SSD1306_WHITE);
        display_.drawLine(x + 46, y + 32, x + 51, y + 28, SSD1306_WHITE);
    }
    display_.display();
}

void GpsDisplay::drawSearching(const GpsReading& reading, unsigned long now)
{
    static const char spinner[] = {'|', '/', '-', '\\'};
    drawHeader("BUSCANDO", SATELLITE_ICON, reading);
    char satelliteLine[22];
    if (reading.satellitesValid)
    {
        snprintf(satelliteLine, sizeof(satelliteLine), "Sat: %02lu",
                 static_cast<unsigned long>(reading.satellites % 100));
    }
    else
    {
        snprintf(satelliteLine, sizeof(satelliteLine), "Sat: --");
    }

    unsigned long seconds = (now - searchStartedAt_) / 1000;
    char elapsedLine[22];
    snprintf(elapsedLine, sizeof(elapsedLine), "%c Espera %02lu:%02lu",
             spinner[animationFrame_ % 4], (seconds / 60) % 100, seconds % 60);

    drawText(0, 20, satelliteLine);
    drawText(0, 33, elapsedLine);
    if (reading.timeValid)
    {
        char timeLine[22];
        snprintf(timeLine, sizeof(timeLine), "Local %02d:%02d UTC%+d", reading.localHour,
                 reading.minute, reading.utcOffsetHours);
        drawText(0, 45, timeLine);
    }
    else
    {
        drawText(0, 45, "Aguarde ao ar livre");
    }
    drawFooter(0, pageCount(reading));
    display_.display();
}

void GpsDisplay::drawClock(const GpsReading& reading)
{
    drawHeader("RELOGIO", CLOCK_ICON, reading);
    char timeLine[22];
    if (reading.timeValid)
    {
        snprintf(timeLine, sizeof(timeLine), "%02d:%02d:%02d", reading.localHour, reading.minute,
                 reading.second);
    }
    else
    {
        snprintf(timeLine, sizeof(timeLine), "--:--:--");
    }

    char satelliteLine[22];
    if (reading.satellitesValid)
    {
        snprintf(satelliteLine, sizeof(satelliteLine), "SAT %02lu",
                 static_cast<unsigned long>(reading.satellites % 100));
    }
    else
    {
        snprintf(satelliteLine, sizeof(satelliteLine), "SAT --");
    }

    drawText(16, 19, timeLine, 2);
    drawText(0, 45, satelliteLine);

    if (reading.dateValid)
    {
        char dateLine[22];
        snprintf(dateLine, sizeof(dateLine), "%02d/%02d/%04d", reading.localDay, reading.localMonth,
                 reading.localYear);
        drawText(34, 36, dateLine);
    }
    else
    {
        drawText(34, 36, "--/--/----");
    }

    char timezoneLine[22];
    snprintf(timezoneLine, sizeof(timezoneLine), "UTC%+03d", reading.utcOffsetHours);
    drawText(86, 45, timezoneLine);
    drawFooter(reading.hasReliableFix ? 0 : 1, pageCount(reading));
    display_.display();
}

void GpsDisplay::drawPosition(const GpsReading& reading)
{
    drawHeader("POSICAO", PIN_ICON, reading);
    drawText(0, 18, "LAT");
    drawText(0, 32, "LON");

    if (reading.locationValid)
    {
        char latitudeLine[22];
        snprintf(latitudeLine, sizeof(latitudeLine), "%c %.6f", reading.latitude < 0 ? 'S' : 'N',
                 fabs(reading.latitude));
        drawText(26, 18, latitudeLine);

        char longitudeLine[22];
        snprintf(longitudeLine, sizeof(longitudeLine), "%c %.6f", reading.longitude < 0 ? 'W' : 'E',
                 fabs(reading.longitude));
        drawText(26, 32, longitudeLine);
    }
    else
    {
        drawText(26, 18, "--");
        drawText(26, 32, "--");
    }

    char satelliteLine[22];
    if (reading.satellitesValid)
    {
        snprintf(satelliteLine, sizeof(satelliteLine), "Sat: %lu",
                 static_cast<unsigned long>(reading.satellites));
    }
    else
    {
        snprintf(satelliteLine, sizeof(satelliteLine), "Sat: --");
    }
    drawText(0, 45, satelliteLine);

    char hdopLine[22];
    if (reading.hdopValid)
    {
        snprintf(hdopLine, sizeof(hdopLine), "HDOP: %.1f", reading.hdop);
    }
    else
    {
        snprintf(hdopLine, sizeof(hdopLine), "HDOP: --");
    }
    drawText(62, 45, hdopLine);
    drawFooter(1, pageCount(reading));
    display_.display();
}

void GpsDisplay::drawMotion(const GpsReading& reading)
{
    drawHeader("MOVIMENTO", SPEED_ICON, reading);

    char speedLine[22];
    if (reading.speedValid)
    {
        snprintf(speedLine, sizeof(speedLine), "%.1f", reading.speedKmph);
    }
    else
    {
        snprintf(speedLine, sizeof(speedLine), "--.-");
    }
    drawText(0, 18, speedLine, strlen(speedLine) <= 7 ? 2 : 1);
    drawText(96, 25, "km/h");

    char directionLine[22];
    if (reading.speedValid && reading.courseValid && reading.speedKmph >= 2.0)
    {
        snprintf(directionLine, sizeof(directionLine), "Dir: %s",
                 compassPoint(reading.courseDegrees));
    }
    else
    {
        snprintf(directionLine, sizeof(directionLine), "Dir: --");
    }
    drawText(0, 36, directionLine);

    char altitudeLine[22];
    if (reading.altitudeValid)
    {
        snprintf(altitudeLine, sizeof(altitudeLine), "Alt: %.0f m", reading.altitudeMeters);
    }
    else
    {
        snprintf(altitudeLine, sizeof(altitudeLine), "Alt: ---- m");
    }
    drawText(56, 36, altitudeLine);

    char precisionLine[22];
    snprintf(precisionLine, sizeof(precisionLine), "HDOP:%s", precisionLabel(reading));
    drawText(0, 45, precisionLine);
    drawFooter(2, pageCount(reading));
    display_.display();
}

void GpsDisplay::turnOff()
{
    if (!ready_ || displayOff_)
    {
        return;
    }
    display_.ssd1306_command(SSD1306_DISPLAYOFF);
    displayOff_ = true;
}

void GpsDisplay::wake()
{
    if (!ready_)
    {
        return;
    }
    screenSaverActive_ = false;
    lastInteractionAt_ = millis();
    lastRefreshAt_ = lastInteractionAt_ - REFRESH_INTERVAL_MS;
    if (displayOff_)
    {
        // Keep the panel off until its old framebuffer has been cleared.
        display_.clearDisplay();
        display_.display();
        display_.ssd1306_command(SSD1306_DISPLAYON);
        displayOff_ = false;
    }
}

bool GpsDisplay::isOff() const
{
    return displayOff_;
}

void GpsDisplay::setScreenSaverEnabled(bool enabled)
{
    screenSaverEnabled_ = enabled;
    screenSaverActive_ = false;
    lastInteractionAt_ = millis();
    lastRefreshAt_ = lastInteractionAt_ - REFRESH_INTERVAL_MS;
}

bool GpsDisplay::isScreenSaverActive() const
{
    return ready_ && !displayOff_ && splashDone_ && screenSaverEnabled_ &&
           millis() - lastInteractionAt_ >= SCREEN_SAVER_TIMEOUT_MS;
}

const char* GpsDisplay::pageName(const GpsReading& reading) const
{
    switch (currentPage(reading))
    {
    case Page::Clock: return "relogio";
    case Page::Position: return "posicao";
    case Page::Motion: return "movimento";
    case Page::Searching: return reading.noDataTimedOut ? "sem_dados" : "busca";
    case Page::Network: return "rede";
    case Page::Locality: return "localidade";
    case Page::Weather: return "tempo";
    }
    return "desconhecida";
}

bool GpsDisplay::selectPage(const String& name, const GpsReading& reading)
{
    if (!ready_) return false;
    uint8_t previous = pageIndex_;
    for (uint8_t index = 0; index < pageCount(reading); ++index)
    {
        pageIndex_ = index;
        if (name == pageName(reading))
        {
            splashDone_ = true;
            wake();
            return true;
        }
    }
    pageIndex_ = previous;
    return false;
}

void GpsDisplay::nextPage(const GpsReading& reading)
{
    if (!ready_ || displayOff_)
    {
        return;
    }

    unsigned long now = millis();
    bool waking = screenSaverEnabled_ &&
                  (screenSaverActive_ || now - lastInteractionAt_ >= SCREEN_SAVER_TIMEOUT_MS);
    lastInteractionAt_ = now;
    lastRefreshAt_ = now - REFRESH_INTERVAL_MS;
    if (waking)
    {
        screenSaverActive_ = false;
        return;
    }

    if (!splashDone_)
    {
        return;
    }

    pageIndex_ = (pageIndex_ + 1) % pageCount(reading);
}

uint8_t GpsDisplay::pageCount(const GpsReading& reading) const
{
    if (reading.noDataTimedOut)
        return 4;
    return reading.hasReliableFix ? 6 : (reading.timeValid ? 5 : 4);
}

GpsDisplay::Page GpsDisplay::currentPage(const GpsReading& reading) const
{
    uint8_t index = pageIndex_ % pageCount(reading);
    if (index == pageCount(reading) - 1)
        return Page::Weather;
    if (index == pageCount(reading) - 2)
        return Page::Locality;
    if (index == pageCount(reading) - 3)
        return Page::Network;
    if (reading.noDataTimedOut)
        return Page::Searching;
    if (reading.hasReliableFix)
    {
        static const Page fixPages[] = {Page::Clock, Page::Position, Page::Motion};
        return fixPages[index];
    }

    if (reading.timeValid && index == 1)
    {
        return Page::Clock;
    }
    return Page::Searching;
}

bool GpsDisplay::wantsLocality(const GpsReading& reading) const
{
    return ready_ && splashDone_ && !displayOff_ &&
           (!screenSaverEnabled_ || millis() - lastInteractionAt_ < SCREEN_SAVER_TIMEOUT_MS) &&
           currentPage(reading) == Page::Locality;
}

void GpsDisplay::drawLocality(const GpsReading& reading, const LocalityReading& locality)
{
    drawText(0, 1, "LOCALIDADE");
    display_.drawFastHLine(0, 12, SCREEN_WIDTH, SSD1306_WHITE);
    const bool wifi = WiFi.status() == WL_CONNECTED;
    const bool fix = reading.hasReliableFix && reading.locationValid;
    const bool moved = locality.available && fix &&
        TinyGPSPlus::distanceBetween(reading.latitude, reading.longitude,
                                    locality.latitude, locality.longitude) >= 500;
    const char* status = locality.busy ? "Consultando..." :
                         !wifi ? "Sem Wi-Fi" : !fix ? "Aguardando GPS" :
                         locality.failed ? "Falha; tenta em 1min" :
                         moved ? "Aguarde nova consulta" :
                         !reading.timeValid ? "Aguardando hora GPS" :
                         locality.available ? "Localidade da posicao" : "Aguardando consulta";
    drawText(0, 16, status);
    if (locality.available)
    {
        // The built-in OLED font is ASCII; normalize common Portuguese accents.
        auto line = [this](uint8_t y, const char* text) {
            String value(text);
            const char* accented[] = {"á", "à", "â", "ã", "ä", "Á", "À", "Â", "Ã", "Ä",
                                     "é", "è", "ê", "ë", "É", "È", "Ê", "Ë",
                                     "í", "ì", "î", "ï", "Í", "Ì", "Î", "Ï",
                                     "ó", "ò", "ô", "õ", "ö", "Ó", "Ò", "Ô", "Õ", "Ö",
                                     "ú", "ù", "û", "ü", "Ú", "Ù", "Û", "Ü", "ç", "Ç", "ñ", "Ñ"};
            const char* plain = "aaaaaAAAAAeeeeEEEEiiiiIIIIoooooOOOOOuuuuUUUUcCnN";
            for (size_t i = 0; i < sizeof(accented) / sizeof(accented[0]); ++i)
                value.replace(accented[i], String(plain[i]));
            // Scroll long place names so the whole name can be read.
            unsigned offset = value.length() > 21 ? (millis() / 700) % (value.length() + 5) : 0;
            if (offset)
                value = (value + "     " + value).substring(offset);
            drawText(0, y, value.c_str());
        };
        line(26, locality.city[0] ? locality.city : "Local nao identificado");
        line(36, locality.region);
        if (!wifi || !fix || moved || locality.failed || locality.busy)
            drawText(0, 46, "Consulta anterior");
        else
            line(46, locality.country);
    }
    else
    {
        drawText(0, 31, "Requer GPS e internet");
        drawText(0, 46, "Fonte: BigDataCloud");
    }
    drawFooter(pageCount(reading) - 2, pageCount(reading));
    display_.display();
}

bool GpsDisplay::wantsWeather(const GpsReading& reading) const
{
    return ready_ && splashDone_ && !displayOff_ &&
           (!screenSaverEnabled_ || millis() - lastInteractionAt_ < SCREEN_SAVER_TIMEOUT_MS) &&
           currentPage(reading) == Page::Weather;
}

void GpsDisplay::drawWeather(const GpsReading& reading, const WeatherReading& weather)
{
    drawText(0, 1, WeatherService::previous(reading, weather) ? "TEMPO (ANTERIOR)" : "PREVISAO DO TEMPO");
    display_.drawFastHLine(0, 12, SCREEN_WIDTH, SSD1306_WHITE);
    if (weather.available)
    {
        unsigned index = (millis() / 5000) % 3;
        const ForecastDay& day = weather.days[index];
        char text[32];
        snprintf(text, sizeof(text), "%c%c/%c%c  Dia %u/3", day.date[8], day.date[9], day.date[5], day.date[6], index + 1);
        drawText(0, 17, text);
        drawText(0, 27, WeatherService::description(day.code));
        snprintf(text, sizeof(text), "Min %.0f Max %.0f C", day.minimum, day.maximum);
        drawText(0, 37, text);
        if (day.rain >= 0) snprintf(text, sizeof(text), "Chuva: %d%%", day.rain);
        else snprintf(text, sizeof(text), "Chuva: --");
        drawText(0, 47, text);
    }
    else
    {
        drawText(0, 20, WeatherService::status(reading, weather).c_str());
        drawText(0, 33, "Requer GPS e internet");
        drawText(0, 46, "Fonte: Open-Meteo");
    }
    drawFooter(pageCount(reading) - 1, pageCount(reading));
    display_.display();
}

void GpsDisplay::drawNetwork(const GpsReading& reading)
{
    bool connected = WiFi.status() == WL_CONNECTED;
    bool portal = (WiFi.getMode() & WIFI_AP) && WiFi.softAPIP() != IPAddress(0, 0, 0, 0);
    drawText(0, 1, "REDE WI-FI");
    display_.drawFastHLine(0, 12, SCREEN_WIDTH, SSD1306_WHITE);
    drawText(0, 17, connected ? "Conectado" : (portal ? "Portal de config." : "Conectando..."));
    String ssid = connected ? WiFi.SSID() : (portal ? "XIAO_ESP32C6_CONFIG" : WiFi.SSID());
    drawText(0, 27, ssid.isEmpty() ? "Aguardando rede" : ssid.c_str());
    String ip = connected ? WiFi.localIP().toString() :
                (portal ? WiFi.softAPIP().toString() : "--");
    drawText(0, 37, ("IP: " + ip).c_str());
    char line[24];
    if (connected)
        snprintf(line, sizeof(line), "Sinal: %ld dBm", static_cast<long>(WiFi.RSSI()));
    else
        snprintf(line, sizeof(line), "%s", portal ? "Abra /config" : "Tentando redes salvas");
    drawText(0, 47, line);
    drawFooter(pageCount(reading) - 3, pageCount(reading));
    display_.display();
}

void GpsDisplay::drawHeader(const char* title, const uint8_t* bitmap, const GpsReading& reading)
{
    display_.drawBitmap(0, 1, bitmap, 8, 8, SSD1306_WHITE);
    drawText(12, 1, title);
    drawText(88, 1, reading.hasReliableFix ? "FIX" : "---");
    for (uint8_t bar = 0; bar < 4; ++bar)
    {
        uint8_t height = 2 + bar * 2;
        uint8_t x = 112 + bar * 4;
        uint8_t y = 9 - height;
        if (reading.satellitesValid && reading.satellites >= 3 + bar * 3)
        {
            display_.fillRect(x, y, 3, height, SSD1306_WHITE);
        }
        else
        {
            display_.drawRect(x, y, 3, height, SSD1306_WHITE);
        }
    }
    if (reading.hasReliableFix)
    {
        display_.drawFastHLine(0, 12, SCREEN_WIDTH, SSD1306_WHITE);
    }
    else
    {
        constexpr uint8_t segmentWidth = 24;
        constexpr uint8_t travel = SCREEN_WIDTH - segmentWidth - 2;
        uint16_t phase = (millis() / 25) % (travel * 2);
        uint8_t offset = phase <= travel ? phase : travel * 2 - phase;
        display_.drawRect(0, 11, SCREEN_WIDTH, 4, SSD1306_WHITE);
        display_.fillRect(1 + offset, 12, segmentWidth, 2, SSD1306_WHITE);
    }
}

void GpsDisplay::drawFooter(uint8_t page, uint8_t count)
{
    display_.drawFastHLine(0, 55, SCREEN_WIDTH, SSD1306_WHITE);
    drawText(0, 57, count > 1 ? "D9: proxima" : "Aguardando GPS");
    for (uint8_t index = 0; index < count; ++index)
    {
        uint8_t x = 80 + index * 8;
        if (index == page)
        {
            display_.fillCircle(x, 60, 2, SSD1306_WHITE);
        }
        else
        {
            display_.drawCircle(x, 60, 2, SSD1306_WHITE);
        }
    }
}

void GpsDisplay::drawText(uint8_t x, uint8_t y, const char* text, uint8_t size)
{
    display_.setTextSize(size);
    display_.setCursor(x, y);
    uint8_t available = (SCREEN_WIDTH - x) / (6 * size);
    for (uint8_t column = 0; column < available && text[column] != '\0'; ++column)
    {
        display_.write(static_cast<uint8_t>(text[column]));
    }
    display_.setTextSize(1);
}

const char* GpsDisplay::compassPoint(double degrees)
{
    static const char* const points[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
    int index = static_cast<int>((degrees + 22.5) / 45.0) % 8;
    if (index < 0)
    {
        index += 8;
    }
    return points[index];
}

const char* GpsDisplay::precisionLabel(const GpsReading& reading)
{
    if (!reading.hdopValid)
    {
        return " --";
    }
    if (reading.hdop <= 1.0)
    {
        return " Otima";
    }
    if (reading.hdop <= 2.0)
    {
        return " Boa";
    }
    if (reading.hdop <= 5.0)
    {
        return " Media";
    }
    return " Fraca";
}
