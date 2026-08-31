/*
 * Kremų Namai – offline parduotuvė ant ESP32
 * ------------------------------------------
 * ESP32 sukuria WiFi prieigos tašką (AP) ir per captive portal
 * pateikia svetainę iš LittleFS. Užsakymai saugomi /orders.jsonl faile.
 *
 * Arduino IDE nustatymai:
 *   Board: ESP32 Dev Module
 *   Flash Size: 4MB
 *   Partition Scheme: Custom (naudokite šalia esantį partitions.csv –
 *     nukopijuokite jį į sketch'o aplanką kaip "partitions.csv")
 *
 * Svetainės įkėlimas: "ESP32 LittleFS Data Upload" įrankiu įkelkite
 * data/ aplanko turinį (arba per arduino-cli / PlatformIO).
 *
 * Prisijungimas: WiFi "KremuNamai" (be slaptažodžio arba su žemiau
 * nurodytu), naršyklėje atsidaro automatiškai arba http://192.168.4.1
 *
 * Užsakymų peržiūra: http://192.168.4.1/api/orders?key=admin123
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <Preferences.h>

// ---------- konfigūracija ----------
const char* AP_SSID = "KremuNamai";
const char* AP_PASS = "";            // "" = atviras tinklas; min. 8 simboliai jei norite slaptažodžio
const char* ADMIN_KEY = "admin123";  // užsakymų peržiūros raktas
const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_MASK(255, 255, 255, 0);

// --- Savininko WiFi (internetas Telegram žinutėms). Palikite tuščią jei nereikia ---
const char* STA_SSID = "";           // pvz. "NamuWiFi"
const char* STA_PASS = "";

// --- Telegram botas ---
// 1. @BotFather -> /newbot -> gausite token'ą
// 2. Parašykite botui žinutę, tada chat_id sužinokite per
//    https://api.telegram.org/bot<TOKEN>/getUpdates  (laukas "chat":{"id":...})
const char* TG_TOKEN  = "";          // pvz. "123456789:AAH..."
const char* TG_CHATID = "";          // pvz. "987654321"

WebServer server(80);
DNSServer dns;
Preferences prefs;   // NVS: užsakymo numerio skaitiklis

// ---------- pagalbinės ----------
String contentType(const String& path) {
  if (path.endsWith(".html")) return "text/html";
  if (path.endsWith(".css"))  return "text/css";
  if (path.endsWith(".js"))   return "application/javascript";
  if (path.endsWith(".svg"))  return "image/svg+xml";
  if (path.endsWith(".png"))  return "image/png";
  if (path.endsWith(".jpg"))  return "image/jpeg";
  if (path.endsWith(".json")) return "application/json";
  if (path.endsWith(".ico"))  return "image/x-icon";
  return "text/plain";
}

bool serveFile(String path) {
  if (path.endsWith("/")) path += "index.html";
  if (!LittleFS.exists(path)) return false;
  File f = LittleFS.open(path, "r");
  server.sendHeader("Cache-Control", "max-age=86400");
  server.streamFile(f, contentType(path));
  f.close();
  return true;
}

// Captive portal: nežinomus hostus nukreipiam į mūsų IP
bool captiveRedirect() {
  if (server.hostHeader() == AP_IP.toString()) return false;
  server.sendHeader("Location", "http://" + AP_IP.toString() + "/", true);
  server.send(302, "text/plain", "");
  return true;
}

// ---------- Telegram ----------
// Ištraukia "tg" lauką iš užsakymo JSON (frontend'as atsiunčia paruoštą tekstą)
String extractTg(const String& body) {
  int s = body.indexOf("\"tg\":\"");
  if (s < 0) return "";
  s += 6;
  String out;
  for (int i = s; i < (int)body.length(); i++) {
    char c = body[i];
    if (c == '\\' && i + 1 < (int)body.length()) {
      char n = body[++i];
      if (n == 'n') out += '\n';
      else if (n == 'u' && i + 4 < (int)body.length()) { i += 4; out += '?'; } // praleidžiam \uXXXX
      else out += n;
    } else if (c == '"') break;
    else out += c;
  }
  return out;
}

String urlEncode(const String& s) {
  String out;
  const char* hex = "0123456789ABCDEF";
  for (size_t i = 0; i < s.length(); i++) {
    unsigned char c = s[i];
    if (isalnum(c) || c=='-' || c=='_' || c=='.' || c=='~') out += (char)c;
    else { out += '%'; out += hex[c >> 4]; out += hex[c & 15]; }
  }
  return out;
}

bool sendTelegram(const String& text) {
  if (!strlen(TG_TOKEN) || !strlen(TG_CHATID)) return false;
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[TG] Nera interneto (STA neprisijunges) - zinute nesiusta.");
    return false;
  }
  WiFiClientSecure client;
  client.setInsecure();               // be sertifikato tikrinimo (paprastumui)
  client.setTimeout(8000);
  if (!client.connect("api.telegram.org", 443)) {
    Serial.println("[TG] Nepavyko prisijungti prie api.telegram.org");
    return false;
  }
  String req = "chat_id=" + String(TG_CHATID) + "&text=" + urlEncode(text);
  client.printf("POST /bot%s/sendMessage HTTP/1.1\r\n", TG_TOKEN);
  client.print("Host: api.telegram.org\r\n"
               "Content-Type: application/x-www-form-urlencoded\r\n"
               "Connection: close\r\n");
  client.printf("Content-Length: %u\r\n\r\n", req.length());
  client.print(req);
  String status = client.readStringUntil('\n');
  client.stop();
  bool ok = status.indexOf("200") > 0;
  Serial.printf("[TG] %s\n", ok ? "Zinute issiusta." : ("Klaida: " + status).c_str());
  return ok;
}

// ---------- API ----------
void handleOrder() {
  if (server.method() != HTTP_POST) { server.send(405, "text/plain", "POST only"); return; }
  String body = server.arg("plain");
  if (body.length() == 0 || body.length() > 4096) {
    server.send(400, "application/json", "{\"err\":\"bad body\"}");
    return;
  }
  uint32_t id = prefs.getUInt("orderId", 0) + 1;
  prefs.putUInt("orderId", id);

  File f = LittleFS.open("/orders.jsonl", "a");
  if (f) {
    f.printf("{\"id\":%u,\"t\":%lu,\"o\":%s}\n", id, millis() / 1000, body.c_str());
    f.close();
  }
  Serial.printf("[UZSAKYMAS #%u] %s\n", id, body.c_str());
  server.send(200, "application/json", "{\"id\":" + String(id) + "}");

  // Telegram žinutė savininkui (po atsakymo, kad klientas nelauktų)
  String tg = extractTg(body);
  if (tg.length()) sendTelegram("№" + String(id) + "\n" + tg);
}

void handleOrdersList() {
  if (server.arg("key") != ADMIN_KEY) { server.send(403, "text/plain", "Neteisingas raktas"); return; }
  if (!LittleFS.exists("/orders.jsonl")) { server.send(200, "text/plain", "Uzsakymu nera."); return; }
  File f = LittleFS.open("/orders.jsonl", "r");
  server.streamFile(f, "application/json");
  f.close();
}

void handleOrdersClear() {
  if (server.arg("key") != ADMIN_KEY) { server.send(403, "text/plain", "Neteisingas raktas"); return; }
  LittleFS.remove("/orders.jsonl");
  server.send(200, "text/plain", "Isvalyta.");
}

// ---------- Admin: produktai ir nuotraukos ----------
// Produktų sąrašą (JSON masyvą) valdo admin.html; ESP32 jį tik saugo /products.json.
void handleProducts() {
  server.sendHeader("Cache-Control", "no-store");
  if (!LittleFS.exists("/products.json")) { server.send(404, "application/json", "[]"); return; }
  File f = LittleFS.open("/products.json", "r");
  server.streamFile(f, "application/json");
  f.close();
}

void handleAdminCheck() {
  if (server.arg("key") != ADMIN_KEY) { server.send(403, "text/plain", "Neteisingas raktas"); return; }
  server.send(200, "application/json", "{\"ok\":1}");
}

void handleAdminSave() {
  if (server.arg("key") != ADMIN_KEY) { server.send(403, "text/plain", "Neteisingas raktas"); return; }
  if (server.method() != HTTP_POST) { server.send(405, "text/plain", "POST only"); return; }
  String body = server.arg("plain");
  if (body.length() < 2 || body.length() > 24576 || body[0] != '[') {
    server.send(400, "application/json", "{\"err\":\"bad body\"}");
    return;
  }
  File f = LittleFS.open("/products.json", "w");
  if (!f) { server.send(500, "application/json", "{\"err\":\"fs\"}"); return; }
  f.print(body);
  f.close();
  Serial.printf("[ADMIN] products.json issaugotas (%u B)\n", body.length());
  server.send(200, "application/json", "{\"ok\":1}");
}

// Nuotraukos įkėlimas: POST /api/admin/upload?key=...&name=p3.jpg (multipart "file")
File upFile;
bool upOk = false;
void handleUpload() {
  HTTPUpload& up = server.upload();
  if (up.status == UPLOAD_FILE_START) {
    upOk = false;
    if (server.arg("key") != ADMIN_KEY) return;
    String name = server.arg("name");
    // saugumo dėlei: tik paprastas failo vardas, be kelių
    if (!name.length() || name.length() > 40) return;
    for (size_t i = 0; i < name.length(); i++) {
      char c = name[i];
      if (!isalnum(c) && c != '.' && c != '-' && c != '_') return;
    }
    if (!LittleFS.exists("/img")) LittleFS.mkdir("/img");
    upFile = LittleFS.open("/img/" + name, "w");
    upOk = (bool)upFile;
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (upFile) upFile.write(up.buf, up.currentSize);
  } else if (up.status == UPLOAD_FILE_END) {
    if (upFile) { upFile.close(); Serial.printf("[ADMIN] ikelta /img/%s (%u B)\n", server.arg("name").c_str(), up.totalSize); }
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    if (upFile) upFile.close();
    upOk = false;
  }
}
void handleUploadDone() {
  if (server.arg("key") != ADMIN_KEY) { server.send(403, "text/plain", "Neteisingas raktas"); return; }
  server.send(upOk ? 200 : 500, "application/json", upOk ? "{\"ok\":1}" : "{\"err\":\"upload\"}");
}

void handleRmImg() {
  if (server.arg("key") != ADMIN_KEY) { server.send(403, "text/plain", "Neteisingas raktas"); return; }
  String name = server.arg("name");
  for (size_t i = 0; i < name.length(); i++) {
    char c = name[i];
    if (!isalnum(c) && c != '.' && c != '-' && c != '_') { server.send(400, "text/plain", "bad name"); return; }
  }
  LittleFS.remove("/img/" + name);
  server.send(200, "application/json", "{\"ok\":1}");
}

// ---------- setup / loop ----------
void setup() {
  Serial.begin(115200);
  delay(200);

  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS klaida!");
  } else {
    Serial.printf("LittleFS: %u KB uzimta / %u KB\n",
                  (unsigned)(LittleFS.usedBytes() / 1024),
                  (unsigned)(LittleFS.totalBytes() / 1024));
  }

  prefs.begin("shop", false);

  // AP + STA: dalinam savo tinklą klientams ir jungiames prie namų WiFi (internetui)
  bool useSta = strlen(STA_SSID) > 0;
  WiFi.mode(useSta ? WIFI_AP_STA : WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_IP, AP_MASK);
  if (strlen(AP_PASS) >= 8) WiFi.softAP(AP_SSID, AP_PASS);
  else                      WiFi.softAP(AP_SSID);
  Serial.printf("AP paleistas: %s -> http://%s\n", AP_SSID, AP_IP.toString().c_str());

  if (useSta) {
    WiFi.begin(STA_SSID, STA_PASS);
    Serial.printf("Jungiamasi prie %s (Telegram zinutems)...\n", STA_SSID);
    for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) delay(500);
    if (WiFi.status() == WL_CONNECTED)
      Serial.printf("STA prisijungta, IP: %s\n", WiFi.localIP().toString().c_str());
    else
      Serial.println("STA neprisijungta - uzsakymai bus saugomi tik LittleFS.");
  }

  dns.start(53, "*", AP_IP);  // visi DNS užklausimai -> mes

  server.on("/api/order", handleOrder);
  server.on("/api/orders", handleOrdersList);
  server.on("/api/orders/clear", handleOrdersClear);

  // Admin
  server.on("/api/products", handleProducts);
  server.on("/api/admin/check", handleAdminCheck);
  server.on("/api/admin/save", handleAdminSave);
  server.on("/api/admin/upload", HTTP_POST, handleUploadDone, handleUpload);
  server.on("/api/admin/rmimg", handleRmImg);
  server.on("/admin", []() { serveFile("/admin.html"); });

  // Captive portal patikros (Android/iOS/Windows)
  server.on("/generate_204", []() { captiveRedirect(); });
  server.on("/hotspot-detect.html", []() { captiveRedirect(); });
  server.on("/connecttest.txt", []() { captiveRedirect(); });
  server.on("/ncsi.txt", []() { captiveRedirect(); });

  server.onNotFound([]() {
    if (captiveRedirect()) return;
    if (!serveFile(server.uri()))
      serveFile("/index.html");  // SPA fallback
  });

  server.begin();
  Serial.println("Web serveris paleistas.");
}

void loop() {
  dns.processNextRequest();
  server.handleClient();
}
