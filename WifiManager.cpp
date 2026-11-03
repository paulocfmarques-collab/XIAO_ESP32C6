#include "WifiManager.h"

#include <Preferences.h>
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <ctime>

namespace
{
constexpr unsigned long CONNECT_TIMEOUT = 13000;
constexpr unsigned long RETRY_INTERVAL = 600000;

String escapeHtml(String value)
{
    value.replace("&", "&amp;");
    value.replace("'", "&#39;");
    value.replace("\"", "&quot;");
    value.replace("<", "&lt;");
    value.replace(">", "&gt;");
    return value;
}
} // namespace

void WifiManager::begin()
{
    Preferences prefs;
    if (prefs.begin("xiaowifi", false))
    {
        for (uint8_t i = 0; i < 5; ++i)
        {
            ssids_[i] = prefs.getString(("s" + String(i)).c_str(), "");
            passwords_[i] = prefs.getString(("p" + String(i)).c_str(), "");
        }
        prefs.end();
    }
    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    WiFi.mode(WIFI_STA);
    server_.on("/", HTTP_GET, [this]() { showPage(); });
    server_.on("/config", HTTP_GET, [this]() { showPage(); });
    server_.on("/api/posicao", HTTP_GET, [this]() { showPosition(); });
    server_.on("/salvar", HTTP_POST, [this]() { save(); });
    server_.onNotFound([this]() { server_.send(404, "text/plain", "Pagina nao encontrada"); });
    server_.begin();
    tryNextNetwork();
}

void WifiManager::startPortal()
{
    if (portal_)
        return;
    WiFi.mode(WIFI_AP_STA);
    portal_ = WiFi.softAP("XIAO_ESP32C6_CONFIG");
    if (portal_)
    {
        Serial.print("[WIFI] Conecte a XIAO_ESP32C6_CONFIG (sem senha) e abra http://");
        Serial.println(WiFi.softAPIP());
    }
    else
        Serial.println("[WIFI] Falha ao iniciar rede de configuracao.");
}

void WifiManager::tryNextNetwork()
{
    while (nextNetwork_ < 5)
    {
        uint8_t slot = nextNetwork_++;
        if (ssids_[slot].isEmpty())
            continue;
        WiFi.disconnect();
        WiFi.begin(ssids_[slot].c_str(), passwords_[slot].c_str());
        attemptAt_ = millis();
        connecting_ = true;
        Serial.printf("[WIFI] Conectando a %s...\n", ssids_[slot].c_str());
        return;
    }
    connecting_ = false;
    WiFi.disconnect();
    startPortal();
    retryAt_ = millis();
}

void WifiManager::update()
{
    server_.handleClient();
    unsigned long now = millis();
    if (restarting_)
    {
        if (now - restartAt_ >= 1500)
            ESP.restart();
        return;
    }
    if (WiFi.status() == WL_CONNECTED)
    {
        if (!otaStarted_)
        {
            ArduinoOTA.setHostname("xiao-esp32c6");
            // Sem setPassword: upload pela IDE Arduino sem autenticacao.
            ArduinoOTA.onStart([]() { Serial.println("[OTA] Atualizacao iniciada."); });
            ArduinoOTA.onEnd([]() { Serial.println("\n[OTA] Atualizacao concluida."); });
            ArduinoOTA.onError([](ota_error_t error) {
                Serial.printf("[OTA] Falha na atualizacao: %u\n", static_cast<unsigned>(error));
            });
            ArduinoOTA.begin();
            otaStarted_ = true;
            Serial.println("[OTA] Disponivel na IDE Arduino: xiao-esp32c6 (sem senha).");
        }
        ArduinoOTA.handle();
        if (!connected_)
        {
            Serial.print("[WIFI] Configuracao em http://");
            Serial.print(WiFi.localIP());
            Serial.println("/config");
            if (portal_)
            {
                WiFi.softAPdisconnect(false);
                WiFi.mode(WIFI_STA);
                portal_ = false;
            }
        }
        connected_ = true;
        connecting_ = false;
        return;
    }
    if (connected_)
    {
        if (otaStarted_)
        {
            ArduinoOTA.end();
            otaStarted_ = false;
        }
        connected_ = false;
        startPortal();
        retryAt_ = now;
    }
    if (connecting_ && now - attemptAt_ >= CONNECT_TIMEOUT)
        tryNextNetwork();
    else if (!connecting_ && now - retryAt_ >= RETRY_INTERVAL)
    {
        nextNetwork_ = 0;
        tryNextNetwork();
    }
}

void WifiManager::showPage()
{
    String html =
        "<!DOCTYPE html><html lang='pt-BR'><head><meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        "<title>XIAO ESP32C6 - Configuracao</title><style>"
        "body{font-family:Arial;margin:20px;background:#1a1a1a;color:#fff;text-align:center}"
        ".c{background:#2d2d2d;max-width:340px;margin:auto;padding:20px;border-radius:12px}"
        "input{width:100%;padding:9px;margin:5px 0;box-sizing:border-box;border-radius:6px;"
        "border:1px solid #444;background:#222;color:#fff}"
        "input[type=submit]{background:#ff5e00;border:none;font-weight:bold;cursor:pointer}"
        "a{color:#ff8b47}dl{text-align:left;overflow-wrap:anywhere}dt{color:#bbb;margin-top:8px}"
        "dd{margin:2px 0 0}section{border-bottom:1px solid #555;padding-bottom:16px}"
        "</style></head><body><div class='c'><h2>XIAO ESP32C6</h2>"
        "<section aria-labelledby='position-title'><h3 id='position-title'>Posição e localidade</h3>"
        "<p id='position-status' role='status'>Carregando posição...</p>"
        "<dl><dt>Latitude / Longitude</dt><dd id='coordinates'>--</dd>"
        "<dt>Satélites / Precisão HDOP</dt><dd id='precision'>--</dd>"
        "<dt>Localidade</dt><dd id='place'>--</dd></dl>"
        "<p id='locality-status'>Aguardando consulta</p>"
        "<a id='map' target='_blank' rel='noopener noreferrer' hidden>Ver no mapa</a>"
        "<p><small>Localidade: BigDataCloud. A consulta envia as coordenadas GPS ao serviço.</small></p>"
        "</section><section><h3>Previsão do tempo na posição GPS</h3>"
        "<p id='weather-status' role='status'>Carregando previsão...</p>"
        "<div id='forecast'></div>"
        "<p><small>Temperaturas em °C; chance de chuva diária. Datas no fuso da posição.</small></p>"
        "<p><a href='https://open-meteo.com/' target='_blank' rel='noopener noreferrer'>"
        "Previsão: Open-Meteo</a></p></section><h3>Configuração Wi-Fi</h3>"
        "<p>Ate 5 redes (SSID vazio = remover; senha vazia mantem a atual para o mesmo SSID).</p>"
        "<form action='/salvar' method='POST'>";
    for (uint8_t i = 0; i < 5; ++i)
    {
        String n(i);
        html += "<b>Rede " + String(i + 1) + "</b><input aria-label='SSID da rede " +
                String(i + 1) + "' name='ssid" + n + "' maxlength='32' value='" +
                escapeHtml(ssids_[i]) +
                "' placeholder='SSID'>"
                "<input aria-label='Senha da rede " +
                String(i + 1) + "' type='password' name='senha" + n +
                "' maxlength='63' autocomplete='new-password' placeholder='Senha'>";
    }
    html += R"HTML(<input type='submit' value='Salvar e Reiniciar'></form></div>
<script>
const el=id=>document.getElementById(id);
async function refreshPosition(){
  try{
    const response=await fetch('/api/posicao',{cache:'no-store',signal:AbortSignal.timeout(5000)});
    if(!response.ok)throw new Error('HTTP');
    const data=await response.json(),gps=data.gps,locality=data.localidade;
    el('position-status').textContent=gps.confiavel?'Posição GPS atual':
      gps.sem_dados?'Sem dados do GPS':'Aguardando posição GPS confiável';
    const valid=gps.latitude!==null&&gps.longitude!==null;
    el('coordinates').textContent=valid?gps.latitude.toFixed(6)+' / '+gps.longitude.toFixed(6):'--';
    el('precision').textContent=(gps.satelites??'--')+' / '+(gps.hdop===null?'--':gps.hdop.toFixed(1));
    el('place').textContent=locality.disponivel?
      [locality.cidade,locality.estado,locality.pais].filter(Boolean).join(' · '):'Ainda não disponível';
    el('locality-status').textContent=locality.estado_consulta;
    const weather=data.tempo;
    el('weather-status').textContent=weather.estado_consulta;
    el('forecast').replaceChildren();
    if(weather.disponivel){
      for(const day of weather.dias){
        const row=document.createElement('p');
        const date=day.data.split('-').reverse().join('/');
        row.textContent=date+' · '+day.condicao+' · '+day.minima.toFixed(0)+' a '+
          day.maxima.toFixed(0)+' °C · Chuva: '+(day.chuva===null?'--':day.chuva+'%');
        el('forecast').appendChild(row);
      }
      const age=document.createElement('small');
      age.textContent='Consulta recebida há '+Math.floor(weather.idade_s/60)+' min';
      el('forecast').appendChild(age);
    }
    const map=el('map');map.hidden=!valid;
    if(valid)map.href='https://www.openstreetmap.org/?mlat='+gps.latitude+'&mlon='+gps.longitude+
      '#map=15/'+gps.latitude+'/'+gps.longitude;
    else map.removeAttribute('href');
  }catch(error){
    el('position-status').textContent='Sem comunicação com a placa; dados exibidos podem estar desatualizados';
    el('locality-status').textContent='Aguardando reconexão';
    el('weather-status').textContent='Sem comunicação; previsão exibida pode estar desatualizada';
    el('map').hidden=true;
  }finally{setTimeout(refreshPosition,3000);}
}
refreshPosition();
</script></body></html>)HTML";
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, "text/html; charset=utf-8", html);
}

void WifiManager::setPosition(const GpsReading& gps, const LocalityReading& locality)
{
    gps_ = gps;
    locality_ = locality;
}

bool WifiManager::wantsLocality() const
{
    return webPositionSeen_ && millis() - webPositionAt_ < 15000UL;
}

void WifiManager::showPosition()
{
    webPositionSeen_ = true;
    webPositionAt_ = millis();
    JsonDocument data;
    JsonObject gps = data["gps"].to<JsonObject>();
    bool valid = gps_.hasReliableFix && gps_.locationValid && !gps_.noDataTimedOut;
    gps["confiavel"] = valid;
    gps["sem_dados"] = gps_.noDataTimedOut;
    gps["latitude"] = nullptr;
    gps["longitude"] = nullptr;
    if (valid)
    {
        gps["latitude"] = gps_.latitude;
        gps["longitude"] = gps_.longitude;
    }
    gps["satelites"] = nullptr;
    gps["hdop"] = nullptr;
    if (gps_.satellitesValid) gps["satelites"] = gps_.satellites;
    if (gps_.hdopValid) gps["hdop"] = gps_.hdop;
    JsonObject locality = data["localidade"].to<JsonObject>();
    locality["disponivel"] = locality_.available;
    locality["cidade"] = locality_.city;
    locality["estado"] = locality_.region;
    locality["pais"] = locality_.country;
    bool moved = valid && locality_.available &&
        TinyGPSPlus::distanceBetween(gps_.latitude, gps_.longitude,
                                    locality_.latitude, locality_.longitude) >= 500;
    const char* status = locality_.busy ? "Consultando localidade..." :
        WiFi.status() != WL_CONNECTED ? "Sem Wi-Fi com internet" :
        !valid ? "Aguardando posição GPS confiável" :
        time(nullptr) < 1704067200 ? "Aguardando hora GPS" :
        locality_.failed ? "Falha na consulta; nova tentativa após o intervalo de 1 minuto" :
        moved ? "Aguardando consulta da nova posição" :
        locality_.available ? "Localidade da posição (aproximada)" : "Aguardando consulta";
    String message(status);
    if (locality_.available && (!valid || moved || locality_.failed || locality_.busy ||
                              WiFi.status() != WL_CONNECTED))
        message += " — exibindo consulta anterior";
    locality["estado_consulta"] = message;
    JsonObject weather = data["tempo"].to<JsonObject>();
    weather["disponivel"] = weather_.available;
    weather["estado_consulta"] = WeatherService::status(gps_, weather_);
    weather["idade_s"] = weather_.available ? (millis() - weather_.receivedAt) / 1000 : 0;
    JsonArray days = weather["dias"].to<JsonArray>();
    if (weather_.available)
    {
        for (const ForecastDay& day : weather_.days)
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
    String json;
    serializeJson(data, json);
    server_.sendHeader("Cache-Control", "no-store");
    server_.send(200, "application/json; charset=utf-8", json);
}

void WifiManager::save()
{
    String ssids[5], passwords[5];
    for (uint8_t i = 0; i < 5; ++i)
    {
        String n(i);
        if (!server_.hasArg("ssid" + n) || !server_.hasArg("senha" + n))
        {
            server_.send(400, "text/plain; charset=utf-8", "Formulario incompleto.");
            return;
        }
        ssids[i] = server_.arg("ssid" + n);
        passwords[i] = server_.arg("senha" + n);
        if (ssids[i].length() > 32 || passwords[i].length() > 63 ||
            (passwords[i].length() > 0 && passwords[i].length() < 8))
        {
            server_.send(400, "text/plain; charset=utf-8",
                         "Use SSID de ate 32 bytes e senha de 8 a 63 bytes (ou vazia).");
            return;
        }
        if (ssids[i].isEmpty())
            passwords[i] = "";
        else if (passwords[i].isEmpty() && ssids[i] == ssids_[i])
            passwords[i] = passwords_[i];
    }
    Preferences prefs;
    bool ok = prefs.begin("xiaowifi", false);
    if (ok)
    {
        for (uint8_t i = 0; i < 5; ++i)
        {
            String n(i);
            auto write = [&prefs](const String& key, const String& value)
            {
                if (value.isEmpty())
                    return !prefs.isKey(key.c_str()) || prefs.remove(key.c_str());
                return prefs.putString(key.c_str(), value) == value.length();
            };
            ok = write("s" + n, ssids[i]) && ok;
            ok = write("p" + n, passwords[i]) && ok;
        }
        prefs.end();
    }
    if (!ok)
    {
        server_.send(503, "text/plain; charset=utf-8", "Falha ao salvar. Tente novamente.");
        return;
    }
    server_.send(200, "text/html; charset=utf-8",
                 "<!DOCTYPE html><html lang='pt-BR'><meta charset='UTF-8'>"
                 "<h2>Configuracao salva! Reiniciando XIAO ESP32C6...</h2></html>");
    restarting_ = true;
    restartAt_ = millis();
}
