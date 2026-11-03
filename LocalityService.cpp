#include "LocalityService.h"
#include "GpsManager.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <NetworkClientSecure.h>
#include <WiFi.h>
#include <cmath>
#include <ctime>

void LocalityService::begin()
{
    requests_ = xQueueCreate(1, sizeof(Request));
    results_ = xQueueCreate(1, sizeof(LocalityReading));
    if (!requests_ || !results_ ||
        xTaskCreate(worker, "locality", 12288, this, 1, nullptr) != pdPASS)
    {
        if (requests_) vQueueDelete(requests_);
        if (results_) vQueueDelete(results_);
        requests_ = results_ = nullptr;
        reading_.failed = true;
    }
}

void LocalityService::update(const GpsReading& gps, bool requested)
{
    if (!requests_ || !results_) return;
    LocalityReading result;
    if (xQueueReceive(results_, &result, 0) == pdTRUE)
    {
        reading_.busy = false;
        reading_.failed = result.failed;
        if (result.available) reading_ = result;
    }
    if (!requested || reading_.busy || !gps.hasReliableFix || !gps.locationValid ||
        WiFi.status() != WL_CONNECTED || time(nullptr) < 1704067200) return;
    unsigned long now = millis();
    // Cache within 500 m; retries and moving updates no more than once per minute.
    if (attempted_ && now - lastAttemptAt_ < 60000UL) return;
    if (reading_.available && !reading_.failed &&
        TinyGPSPlus::distanceBetween(gps.latitude, gps.longitude,
                                     reading_.latitude, reading_.longitude) < 500) return;
    Request request{gps.latitude, gps.longitude};
    if (xQueueSend(requests_, &request, 0) == pdTRUE)
    {
        reading_.busy = true;
        attempted_ = true;
        lastAttemptAt_ = now;
    }
}

void LocalityService::worker(void* context)
{
    auto* self = static_cast<LocalityService*>(context);
    Request request;
    for (;;)
    {
        if (xQueueReceive(self->requests_, &request, portMAX_DELAY) != pdTRUE) continue;
        LocalityReading result;
        result.failed = true;
        result.latitude = request.latitude;
        result.longitude = request.longitude;
        NetworkClientSecure client;
        client.useBuiltinCACertBundle();
        client.setHandshakeTimeout(8);
        HTTPClient http;
        http.setConnectTimeout(5000);
        http.setTimeout(5000);
        String url = "https://api.bigdatacloud.net/data/reverse-geocode-client?latitude=" +
                     String(request.latitude, 6) + "&longitude=" + String(request.longitude, 6) +
                     "&localityLanguage=pt";
        if (http.begin(client, url))
        {
            http.addHeader("User-Agent", "XIAO-ESP32C6-GPS/1.0");
            if (http.GET() == HTTP_CODE_OK)
            {
                // Only keep fields needed on the OLED, even for large responses.
                JsonDocument filter;
                filter["city"] = true;
                filter["locality"] = true;
                filter["principalSubdivision"] = true;
                filter["countryName"] = true;
                JsonDocument data;
                auto error = deserializeJson(data, http.getString(), DeserializationOption::Filter(filter));
                if (!error)
                {
                    const char* city = data["city"] | "";
                    if (!*city) city = data["locality"] | "";
                    snprintf(result.city, sizeof(result.city), "%s", city);
                    snprintf(result.region, sizeof(result.region), "%s", data["principalSubdivision"] | "");
                    snprintf(result.country, sizeof(result.country), "%s", data["countryName"] | "");
                    result.available = result.city[0] || result.region[0] || result.country[0];
                    result.failed = !result.available;
                }
            }
            http.end();
        }
        xQueueSend(self->results_, &result, portMAX_DELAY);
    }
}
