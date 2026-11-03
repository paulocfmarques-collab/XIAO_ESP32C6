#include "WeatherService.h"
#include "GpsManager.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <WiFi.h>
#include <ctime>
#include <cmath>
#include <cstring>

namespace { constexpr unsigned long CACHE_MS = 1800000UL; }

void WeatherService::begin()
{
    requests_ = xQueueCreate(1, sizeof(Request));
    results_ = xQueueCreate(1, sizeof(WeatherReading));
    if (!requests_ || !results_ || xTaskCreate(worker, "weather", 12288, this, 1, nullptr) != pdPASS)
    {
        if (requests_) vQueueDelete(requests_);
        if (results_) vQueueDelete(results_);
        requests_ = results_ = nullptr;
        reading_.failed = true;
    }
}

void WeatherService::update(const GpsReading& gps, bool requested)
{
    if (!requests_ || !results_) return;
    WeatherReading result;
    if (xQueueReceive(results_, &result, 0) == pdTRUE)
    {
        reading_.busy = false;
        reading_.failed = result.failed;
        if (result.available)
        {
            result.receivedAt = millis();
            reading_ = result;
        }
    }
    if (!requested || reading_.busy || !gps.hasReliableFix || !gps.locationValid ||
        gps.noDataTimedOut || WiFi.status() != WL_CONNECTED || time(nullptr) < 1704067200) return;
    unsigned long now = millis();
    if (attempted_ && now - lastAttemptAt_ < 300000UL) return;
    if (reading_.available && !reading_.failed && now - reading_.receivedAt < CACHE_MS &&
        TinyGPSPlus::distanceBetween(gps.latitude, gps.longitude, reading_.latitude, reading_.longitude) < 5000)
        return;
    Request request{gps.latitude, gps.longitude};
    if (xQueueSend(requests_, &request, 0) == pdTRUE)
    {
        reading_.busy = true;
        attempted_ = true;
        lastAttemptAt_ = now;
    }
}

bool WeatherService::previous(const GpsReading& gps, const WeatherReading& weather)
{
    return weather.available && (weather.failed || weather.busy ||
        WiFi.status() != WL_CONNECTED || !gps.hasReliableFix || !gps.locationValid || gps.noDataTimedOut ||
        millis() - weather.receivedAt >= CACHE_MS ||
        TinyGPSPlus::distanceBetween(gps.latitude, gps.longitude, weather.latitude, weather.longitude) >= 5000);
}

String WeatherService::status(const GpsReading& gps, const WeatherReading& weather)
{
    String value = weather.busy ? "Consultando..." : WiFi.status() != WL_CONNECTED ? "Sem Wi-Fi" :
        !gps.hasReliableFix || !gps.locationValid || gps.noDataTimedOut ? "Aguardando GPS" :
        time(nullptr) < 1704067200 ? "Aguardando hora GPS" :
        weather.failed ? "Falha; tenta em 5min" : weather.available ? "Previsao do tempo" : "Aguardando consulta";
    if (previous(gps, weather)) value += " (anterior)";
    return value;
}

const char* WeatherService::description(int code)
{
    if (code == 0) return "Ceu limpo";
    if (code == 1) return "Poucas nuvens";
    if (code == 2) return "Parcialmente nublado";
    if (code == 3) return "Nublado";
    if (code == 45 || code == 48) return "Nevoeiro";
    if (code >= 51 && code <= 57) return "Garoa";
    if (code >= 61 && code <= 67) return "Chuva";
    if ((code >= 71 && code <= 77) || code == 85 || code == 86) return "Neve";
    if (code >= 80 && code <= 82) return "Pancadas de chuva";
    if (code == 95 || code == 96 || code == 99) return "Trovoadas";
    return "Nao informado";
}

void WeatherService::worker(void* context)
{
    auto* self = static_cast<WeatherService*>(context);
    Request request;
    for (;;)
    {
        if (xQueueReceive(self->requests_, &request, portMAX_DELAY) != pdTRUE) continue;
        WeatherReading result;
        result.failed = true;
        result.latitude = request.latitude;
        result.longitude = request.longitude;
        NetworkClientSecure client;
        client.useBuiltinCACertBundle();
        client.setHandshakeTimeout(8);
        HTTPClient http;
        http.setConnectTimeout(5000);
        http.setTimeout(5000);
        String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(request.latitude, 6) +
            "&longitude=" + String(request.longitude, 6) +
            "&daily=weather_code,temperature_2m_min,temperature_2m_max,precipitation_probability_max"
            "&timezone=auto&forecast_days=3";
        if (http.begin(client, url))
        {
            if (http.GET() == HTTP_CODE_OK)
            {
                JsonDocument data;
                if (!deserializeJson(data, http.getString()))
                {
                    JsonObject daily = data["daily"];
                    bool valid = true;
                    for (int i = 0; i < 3; ++i)
                    {
                        ForecastDay& day = result.days[i];
                        const char* date = daily["time"][i] | "";
                        day.minimum = daily["temperature_2m_min"][i] | NAN;
                        day.maximum = daily["temperature_2m_max"][i] | NAN;
                        day.rain = daily["precipitation_probability_max"][i] | -1;
                        day.code = daily["weather_code"][i] | -1;
                        snprintf(day.date, sizeof(day.date), "%s", date);
                        valid = valid && strlen(date) == 10 && isfinite(day.minimum) && isfinite(day.maximum);
                    }
                    result.available = valid;
                    result.failed = !valid;
                }
            }
            http.end();
        }
        xQueueSend(self->results_, &result, portMAX_DELAY);
    }
}
