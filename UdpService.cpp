#include "UdpService.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ctime>
#include <cstring>

namespace
{
const char* const COMMANDS[] = {
    "ajuda", "ping", "status", "gps", "posicao", "relogio", "satelites", "movimento", "precisao",
    "wifi", "redes", "localidade", "tempo", "consultar_localidade", "consultar_tempo", "sistema", "ota",
    "tela", "tela_ligar", "tela_desligar", "tela_proxima", "tela_relogio", "tela_posicao",
    "tela_movimento", "tela_busca", "tela_sem_dados", "tela_rede", "tela_localidade", "tela_tempo",
    "led", "led_ligar", "led_desligar", "led_auto", "reiniciar"
};
}

UdpService::UdpService(GpsDisplay& display, StatusLed& led, WifiManager& wifi,
                       LocalityService& locality, WeatherService& weather)
    : display_(display), led_(led), wifi_(wifi), locality_(locality), weather_(weather)
{
}

bool UdpService::wantsLocality() const
{
    return localityRequested_ && millis() - localityAt_ < 30000UL;
}

bool UdpService::wantsWeather() const
{
    return weatherRequested_ && millis() - weatherAt_ < 30000UL;
}

void UdpService::update(const GpsReading& gps)
{
    if (restarting_ && millis() - restartAt_ >= 1000UL) ESP.restart();
    IPAddress station = WiFi.status() == WL_CONNECTED ? WiFi.localIP() : IPAddress();
    IPAddress portal = (WiFi.getMode() & WIFI_AP) ? WiFi.softAPIP() : IPAddress();
    if (station != stationIp_ || portal != portalIp_)
    {
        socket_.stop();
        listening_ = false;
        bindAttempted_ = false;
        stationIp_ = station;
        portalIp_ = portal;
    }
    if (static_cast<uint32_t>(station) == 0 && static_cast<uint32_t>(portal) == 0) return;
    if (!listening_)
    {
        if (bindAttempted_ && millis() - bindAt_ < 1000UL) return;
        bindAttempted_ = true;
        bindAt_ = millis();
        listening_ = socket_.begin(PORT) == 1;
        if (listening_) Serial.printf("[UDP] comandos em minusculas na porta %u\n", PORT);
        return;
    }
    int size = socket_.parsePacket();
    if (size <= 0) return;
    IPAddress remote = socket_.remoteIP();
    uint16_t remotePort = socket_.remotePort();
    char buffer[64];
    String reply;
    if (size >= static_cast<int>(sizeof(buffer)))
    {
        reply = "{\"ok\":false,\"erro\":\"comando_longo\"}";
    }
    else
    {
        int read = socket_.read(buffer, size);
        if (read != size || memchr(buffer, '\0', read) != nullptr)
            reply = "{\"ok\":false,\"erro\":\"datagrama_invalido\"}";
        else
        {
            buffer[read] = '\0';
            String command(buffer);
            command.trim();
            reply = execute(command, gps);
        }
    }
    // Drain this datagram only. Handle at most one command per firmware loop.
    while (socket_.available()) socket_.read();
    if (socket_.beginPacket(remote, remotePort))
    {
        socket_.write(reinterpret_cast<const uint8_t*>(reply.c_str()), reply.length());
        socket_.endPacket();
    }
}

String UdpService::execute(const String& command, const GpsReading& gps)
{
    JsonDocument data;
    data["ok"] = true;
    data["comando"] = command;
    bool known = false;
    for (const char* name : COMMANDS) known = known || command == name;
    if (!known)
    {
        data["ok"] = false;
        data["erro"] = "comando_invalido_use_ajuda_em_minusculas";
    }
    else if (command == "ajuda")
    {
        data["porta"] = PORT;
        JsonArray commands = data["comandos"].to<JsonArray>();
        for (const char* name : COMMANDS) commands.add(name);
    }
    else if (command == "ping") data["resposta"] = "pong";
    else if (command == "reiniciar")
    {
        if (!restarting_) restartAt_ = millis();
        restarting_ = true;
        data["resposta"] = "reinicio_agendado";
    }
    else if (command == "sistema")
    {
        data["chip"] = ESP.getChipModel();
        data["cpu_mhz"] = ESP.getCpuFreqMHz();
        data["uptime_s"] = millis() / 1000;
        data["heap_livre"] = ESP.getFreeHeap();
        data["heap_minimo"] = ESP.getMinFreeHeap();
        data["flash_bytes"] = ESP.getFlashChipSize();
        data["firmware_bytes"] = ESP.getSketchSize();
    }
    else if (command == "status")
    {
        data["uptime_s"] = millis() / 1000;
        data["gps_fix"] = gps.hasReliableFix;
        data["wifi_conectado"] = WiFi.status() == WL_CONNECTED;
        data["tela_ligada"] = display_.isReady() && !display_.isOff();
        data["pagina"] = display_.pageName(gps);
        data["led_modo"] = led_.modeName();
        data["localidade_disponivel"] = locality_.reading().available;
        data["tempo_disponivel"] = weather_.reading().available;
        data["ota_disponivel"] = wifi_.otaAvailable();
    }
    else if (command == "wifi")
    {
        bool connected = WiFi.status() == WL_CONNECTED;
        data["conectado"] = connected;
        data["ssid"] = connected ? WiFi.SSID() : String();
        data["ip"] = connected ? WiFi.localIP().toString() : String();
        data["portal_ip"] = portalIp_.toString();
        data["portal_ativo"] = static_cast<uint32_t>(portalIp_) != 0;
        data["rssi"] = nullptr;
        if (connected) data["rssi"] = WiFi.RSSI();
        data["mac"] = WiFi.macAddress();
    }
    else if (command == "redes")
    {
        JsonArray networks = data["redes"].to<JsonArray>();
        for (uint8_t index = 0; index < 5; ++index)
        {
            JsonObject network = networks.add<JsonObject>();
            network["slot"] = index + 1;
            network["ssid"] = wifi_.savedSsid(index);
        }
    }
    else if (command == "ota")
    {
        data["disponivel"] = wifi_.otaAvailable();
        data["hostname"] = "xiao-esp32c6";
        data["porta"] = 3232;
        data["senha"] = false;
    }
    else if (command.startsWith("tela"))
    {
        data["disponivel"] = display_.isReady();
        if (!display_.isReady())
        {
            data["ok"] = false;
            data["erro"] = "oled_indisponivel";
        }
        else if (command == "tela_ligar") display_.wake();
        else if (command == "tela_desligar") display_.turnOff();
        else if (command == "tela_proxima")
        {
            display_.selectPage(display_.pageName(gps), gps);
            display_.nextPage(gps);
        }
        else if (command != "tela" && !display_.selectPage(command.substring(5), gps))
        {
            data["ok"] = false;
            data["erro"] = "pagina_indisponivel_no_estado_atual_do_gps";
        }
        data["ligada"] = display_.isReady() && !display_.isOff();
        data["pagina"] = display_.pageName(gps);
    }
    else if (command.startsWith("led"))
    {
        if (command == "led_ligar") led_.setMode(StatusLed::Mode::On);
        if (command == "led_desligar") led_.setMode(StatusLed::Mode::Off);
        if (command == "led_auto") led_.setMode(StatusLed::Mode::Auto);
        led_.update(gps.hasReliableFix);
        data["modo"] = led_.modeName();
        data["ligado"] = led_.isOn();
    }
    else if (command == "localidade" || command == "consultar_localidade")
    {
        if (command == "consultar_localidade")
        {
            localityRequested_ = true;
            localityAt_ = millis();
        }
        const LocalityReading& locality = locality_.reading();
        data["disponivel"] = locality.available;
        data["consultando"] = locality.busy;
        data["falhou"] = locality.failed;
        data["solicitada"] = wantsLocality();
        data["cidade"] = locality.city;
        data["estado"] = locality.region;
        data["pais"] = locality.country;
        data["anterior"] = locality.available && (!gps.hasReliableFix || locality.failed ||
            WiFi.status() != WL_CONNECTED || TinyGPSPlus::distanceBetween(gps.latitude, gps.longitude,
                locality.latitude, locality.longitude) >= 500);
    }
    else if (command == "tempo" || command == "consultar_tempo")
    {
        if (command == "consultar_tempo")
        {
            weatherRequested_ = true;
            weatherAt_ = millis();
        }
        const WeatherReading& weather = weather_.reading();
        data["disponivel"] = weather.available;
        data["consultando"] = weather.busy;
        data["falhou"] = weather.failed;
        data["solicitada"] = wantsWeather();
        data["anterior"] = WeatherService::previous(gps, weather);
        data["idade_s"] = weather.available ? (millis() - weather.receivedAt) / 1000 : 0;
        JsonArray days = data["dias"].to<JsonArray>();
        if (weather.available) for (const ForecastDay& day : weather.days)
        {
            JsonObject item = days.add<JsonObject>();
            item["data"] = day.date;
            item["condicao"] = WeatherService::description(day.code);
            item["minima"] = day.minimum;
            item["maxima"] = day.maximum;
            item["chuva"] = nullptr;
            if (day.rain >= 0) item["chuva"] = day.rain;
        }
    }
    else
    {
        // Each GPS query returns the same complete snapshot, with explicit validity.
        data["fix"] = gps.hasReliableFix;
        data["sem_dados"] = gps.noDataTimedOut;
        data["latitude"] = nullptr;
        data["longitude"] = nullptr;
        if (gps.hasReliableFix && gps.locationValid && !gps.noDataTimedOut)
        {
            data["latitude"] = gps.latitude;
            data["longitude"] = gps.longitude;
        }
        data["satelites"] = nullptr;
        data["hdop"] = nullptr;
        data["velocidade_kmh"] = nullptr;
        data["rumo_graus"] = nullptr;
        data["altitude_m"] = nullptr;
        if (gps.satellitesValid) data["satelites"] = gps.satellites;
        if (gps.hdopValid) data["hdop"] = gps.hdop;
        if (gps.speedValid) data["velocidade_kmh"] = gps.speedKmph;
        if (gps.courseValid) data["rumo_graus"] = gps.courseDegrees;
        if (gps.altitudeValid) data["altitude_m"] = gps.altitudeMeters;
        data["hora_valida"] = gps.timeValid;
        data["data_valida"] = gps.dateValid;
        data["hora"] = nullptr;
        data["data"] = nullptr;
        char text[24];
        if (gps.timeValid)
        {
            snprintf(text, sizeof(text), "%02d:%02d:%02d", gps.localHour, gps.minute, gps.second);
            data["hora"] = text;
        }
        if (gps.dateValid)
        {
            snprintf(text, sizeof(text), "%04d-%02d-%02d", gps.localYear, gps.localMonth, gps.localDay);
            data["data"] = text;
        }
        data["fuso_horas"] = gps.utcOffsetHours;
    }
    String reply;
    serializeJson(data, reply);
    return reply;
}
