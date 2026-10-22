#include "weather.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// Weather comes from Open-Meteo, the location from OpenStreetMap (both free, no API key).

Sky skyFromCode(int c) {
  if ((c >= 71 && c <= 77) || c == 85 || c == 86) return Sky::Snow;
  if ((c >= 51 && c <= 67) || (c >= 80 && c <= 82) || c >= 95) return Sky::Rain;
  if (c >= 2) return Sky::Cloud;  // overcast and fog
  return Sky::Sun;
}

static String urlEncode(const String& s) {
  static const char hex[] = "0123456789ABCDEF";
  String out;
  for (size_t i = 0; i < s.length(); ++i) {
    uint8_t c = s[i];
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      out += char(c);
    } else {
      out += '%';
      out += hex[c >> 4];
      out += hex[c & 15];
    }
  }
  return out;
}

static bool getJson(const String& url, JsonDocument& doc) {
  HTTPClient http;
  http.setTimeout(8000);
  if (!http.begin(url)) return false;
  bool ok = false;
  int code = http.GET();
  if (code == HTTP_CODE_OK) {
    ok = !deserializeJson(doc, http.getString());
  } else {
    Serial.printf("HTTP %d for %s\n", code, url.c_str());
  }
  http.end();
  return ok;
}

// The display fonts are plain ASCII.
static String asciiOnly(const String& s) {
  String out = s;
  out.replace("ä", "ae"); out.replace("ö", "oe"); out.replace("ü", "ue");
  out.replace("Ä", "Ae"); out.replace("Ö", "Oe"); out.replace("Ü", "Ue");
  out.replace("ß", "ss");
  String clean;
  for (size_t i = 0; i < out.length(); ++i) {
    if (uint8_t(out[i]) < 128) clean += out[i];
  }
  return clean;
}

bool geocodeCity(const String& query, float& lat, float& lon, String& found) {
  String q = query;
  q.trim();
  if (q.length() < 2) return false;

  // OpenStreetMap's Nominatim knows the actual area of a postcode, and also
  // takes full addresses. Open-Meteo's own search maps some German postcodes to France.
  bool isPlz = q.length() == 5;
  for (size_t i = 0; isPlz && i < q.length(); ++i) isPlz = isdigit(uint8_t(q[i]));
  String url = "https://nominatim.openstreetmap.org/search?format=json&limit=1&addressdetails=1&accept-language=de&";
  url += isPlz ? "country=de&postalcode=" + q : "q=" + urlEncode(q);

  // Nominatim is HTTPS-only and requires an identifying User-Agent.
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(10000);
  if (!http.begin(client, url)) return false;
  http.setUserAgent("Kulleraugen-ESP32/1.0");
  int code = http.GET();
  JsonDocument doc;
  bool ok = code == HTTP_CODE_OK && !deserializeJson(doc, http.getString());
  if (!ok) Serial.printf("HTTP %d for %s\n", code, url.c_str());
  http.end();
  if (!ok) return false;

  JsonObject r = doc[0];
  if (r.isNull()) return false;
  lat = atof(r["lat"] | "0");
  lon = atof(r["lon"] | "0");

  JsonObject a = r["address"];
  const char* place = a["city_district"] | a["suburb"] | a["city"] | a["town"] | a["village"] |
                      a["municipality"] | r["name"] | "?";
  String cc = a["country_code"] | "";
  cc.toUpperCase();
  found = asciiOnly(place);
  if (cc.length()) found += " (" + cc + ")";
  return true;
}

bool fetchWeather(float lat, float lon, WeatherData& out) {
  String url = "http://api.open-meteo.com/v1/forecast?latitude=" + String(lat, 4) +
               "&longitude=" + String(lon, 4) +
               "&current=temperature_2m,weather_code"
               "&hourly=temperature_2m,weather_code&forecast_hours=13&timezone=auto";
  JsonDocument doc;
  if (!getJson(url, doc)) return false;

  JsonArray times = doc["hourly"]["time"];
  JsonArray temps = doc["hourly"]["temperature_2m"];
  JsonArray codes = doc["hourly"]["weather_code"];
  if (times.size() < 13 || temps.size() < 13 || codes.size() < 13) return false;

  out.temp = lroundf(doc["current"]["temperature_2m"].as<float>());
  out.sky = skyFromCode(doc["current"]["weather_code"] | 0);
  for (int i = 0; i < 4; ++i) {
    int idx = (i + 1) * 3;
    const char* t = times[idx] | "0000-00-00T00:00";  // ISO local time
    out.next[i].hour = atoi(t + 11);
    out.next[i].temp = lroundf(temps[idx].as<float>());
    out.next[i].sky = skyFromCode(codes[idx] | 0);
  }
  out.utcOffset = doc["utc_offset_seconds"] | 0;
  out.valid = true;
  return true;
}
