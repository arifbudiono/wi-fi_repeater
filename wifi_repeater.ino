#if LWIP_FEATURES && !LWIP_IPV6

#define HAVE_NETDUMP 0

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <lwip/napt.h>
#include <lwip/dns.h>
#include <LwipDhcpServer.h>

#define NAPT 1000
#define NAPT_PORT 10

#if HAVE_NETDUMP

#include <NetDump.h>

void dump(int netif_idx, const char* data, size_t len, int out, int success) {
  (void)success;
  Serial.print(out ? F("out ") : F(" in "));
  Serial.printf("%d ", netif_idx);

  // optional filter example: if (netDump_is_ARP(data))
  {
    netDump(Serial, data, len);
    //netDumpHex(Serial, data, len);
  }
}
#endif

// MY FUNCTIONS
bool testwifi() {
  Serial.printf("\nTesting connection with '%s'\n", WiFi.SSID().c_str());
  int count = 0;
  digitalWrite(2,LOW);
  while (count < 20) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("\nWiFi Connected! \nSTA: %s (dns: %s / %s)\n\n",
                    WiFi.localIP().toString().c_str(),
                    WiFi.dnsIP(0).toString().c_str(),
                    WiFi.dnsIP(1).toString().c_str());

      // give DNS servers to AP side
      dhcpSoftAP.dhcps_set_dns(0, WiFi.dnsIP(0));
      dhcpSoftAP.dhcps_set_dns(1, WiFi.dnsIP(1));
      digitalWrite(2,HIGH);
      return true;
    }
    Serial.print(".");
    delay(1000);
    count++;
  }
  Serial.printf("\nCan't connect to WiFi, connect to AP '%s' and configure...\n\n", WiFi.softAPSSID());
  return false;
}

// SERVER
ESP8266WebServer server(80);
String content;
void serverconfig() {
  server.begin();
 
  server.on("/", []() {
    content = "<!DOCTYPE html><html lang='en'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    content += "<head><title>Portable Wi-Fi Repeater</title>";
    content += "<style>body{font-family: apercu-pro, -apple-system, system-ui, BlinkMacSystemFont, 'Helvetica Neue', sans-serif; padding: 1em;line-height: 2em;font-weight: 100;}h1 {font-size: bold;font-weight: 200;text-align: center;}h2 {font-size: 1.2em;font-weight: 200;margin-left: 7px;}";
    content += "td {font-weight: 200;min-height: 24px;}td:first-child {text-align: right;min-width: 100px;padding-right: 10px;}input {border: 1px solid rgb(196, 196, 196);color: rgb(76, 76, 76);width: 240px;border-radius: 2px;height: 35px;margin: 1px 0px;padding: 0px 14px;}";
    content += "input:focus {border:1px solid black;outline: none !important;box-shadow: 0 0 10px #719ECE;}#config {width:400px;margin:0 auto;}.ok-button {background-color: #0078e7;color: #fff;box-shadow: 0 0 2px #719ECE;}.red-button {background-color: #e72e00;color: #fff;box-shadow: 0 0 2px #719ECE;}</style>";
    content += "</head>";
    content += "<body>";
    content += "<h1>Wi-Fi Repeater Configuration</h1>";
    content += "<div id='config'>";
    content += "<h2>Wireless Station Settings</h2>";
    content += "<form method='post'>";
    content += "<table>";
    content += "<tr><td>SSID</td><td><input name='stassid' placeholder='";
    content += WiFi.SSID();
    content += "' length=32></td></tr>";
    content += "<tr><td>Password</td><td><input type='password' placeholder='********' name='stapsk' minlength=8 maxlength=63></td></tr>";
    content += "<tr><td></td><td><input type='submit' formaction='tempstasettings' value='Connect' class='ok-button'></td></tr>";
    content += "</table>";
    content += "</form>";
    content += "<h2>Wireless Access Point Settings</h2>";
    content += "<form method='post'>";
    content += "<table>";
    content += "<tr><td>SSID</td><td><input name='apssid' placeholder='";
    content += WiFi.softAPSSID();
    content += "' length=32></td></tr>";
    content += "<tr><td>Security</td><td><select><option value='open'>Open</option><option value='psk'>WPA2-PSK</option></select></td></tr>";
    content += "<tr><td>Password</td><td><input type='password' name='appsk' placeholder='********'";
    content += WiFi.softAPPSK();
    content += "' name='appsk' minlength=8 maxlength=63><br><small><i>Must be at least 8 characters or blank!</i></small></td></tr>";
    content += "<tr><td></td><td><input type='submit' formaction='apsettings' value='Save Permanently' class='ok-button'></td<td><input type='submit' formaction='tempapsettings' value='Save Temporarily (Until Reboot)' class='ok-button'></td></tr>";
    content += "</table>";
    content += "</form>";
    content += "<h2>Miscellaneous</h2>";     
    content += "<form method='get' action='reboot'><table><tr><td>Reboot Device</td><td><input type='submit' value='Reboot' class='red-button'></td></tr></table></form>";
    content += "</div>";
    server.send(200, "text/html", content);
  });

  server.onNotFound([]() {
    server.send(404, "text/plain", "How the heck did you get here?");
  });

  server.on("/stasettings", []() {
    String stassid = server.arg("stassid");
    String stapsk = server.arg("stapsk");
    if (stassid.length() > 0) {
      server.send(200, "text/plain", "Settings Recieved");
      Serial.printf("\n\nAttempting to connect to '%s' using password '%s' \n", stassid.c_str(), stapsk.c_str());
      WiFi.persistent(true);
      WiFi.begin(stassid, stapsk);
      testwifi();
    }
  });

  server.on("/tempstasettings", []() {
    String stassid = server.arg("stassid");
    String stapsk = server.arg("stapsk");
    if (stassid.length() > 0) {
      server.send(200, "text/plain", "Settings Recieved");
      Serial.printf("\n\nAttempting to connect to '%s' using password '%s' \n", stassid.c_str(), stapsk.c_str());
      WiFi.persistent(false);
      WiFi.begin(stassid, stapsk);
      testwifi();
    }
  });

  server.on("/apsettings", []() {
    String apssid = server.arg("apssid");
    String appsk = server.arg("appsk");
    if (apssid.length() > 0) {
      server.send(200, "text/plain", "Settings Recieved");
      Serial.printf("\n\nSetting AP Credentials \nSSID: %s \nPassword: %s \n", apssid.c_str(), appsk.c_str());
      WiFi.persistent(true);
      WiFi.softAP(apssid, appsk);
    }
  });

  server.on("/tempapsettings", []() {
    String apssid = server.arg("apssid");
    String appsk = server.arg("appsk");
    if (apssid.length() > 0) {
      server.send(200, "text/plain", "Settings Recieved");
      Serial.printf("\n\nSetting Temporary AP Credentials \nSSID: %s \nPassword: %s \n", apssid.c_str(), appsk.c_str());
      WiFi.persistent(false);
      WiFi.softAP(apssid, appsk);
    }
  });

  server.on("/reboot", []() {
    server.send(200, "text/plain", "Rebooting now...");
    delay(5000);
    ESP.reset();
  });
}

void setup() {
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  delay(1000);
  Serial.printf("\n\nNAPT Range extender\n");
  Serial.printf("Heap on start: %d\n", ESP.getFreeHeap());

#if HAVE_NETDUMP
  phy_capture = dump;
#endif

  WiFi.setPhyMode(WIFI_PHY_MODE_11N); // Set radio type to N
  WiFi.mode(WIFI_AP_STA);
  WiFi.persistent(false);
  WiFi.begin(); // Use stored credentials to connect to network
  testwifi();
  WiFi.softAPConfig(  // Set IP Address, Gateway and Subnet
    IPAddress(192, 168, 4, 1),
    IPAddress(192, 168, 4, 1),
    IPAddress(255, 255, 255, 0));
  WiFi.softAP(WiFi.softAPSSID(), WiFi.softAPPSK()); // Use stored credentials to create AP

  Serial.printf("Heap before: %d\n", ESP.getFreeHeap());
  err_t ret = ip_napt_init(NAPT, NAPT_PORT);
  Serial.printf("ip_napt_init(%d,%d): ret=%d (OK=%d)\n", NAPT, NAPT_PORT, (int)ret, (int)ERR_OK);
  if (ret == ERR_OK) {
    ret = ip_napt_enable_no(SOFTAP_IF, 1);
    Serial.printf("ip_napt_enable_no(SOFTAP_IF): ret=%d (OK=%d)\n", (int)ret, (int)ERR_OK);
    if (ret == ERR_OK) {
      Serial.printf("\nWiFi Network '%s' with Passowrd '%s' and IP '%s' is now setup\n", WiFi.softAPSSID(), WiFi.softAPPSK().c_str(), WiFi.softAPIP().toString().c_str());
    }
  }
  Serial.printf("Heap after napt init: %d\n", ESP.getFreeHeap());
  if (ret != ERR_OK) {
    Serial.printf("NAPT initialization failed\n");
  }

  serverconfig();
}

#else

void setup() {
  Serial.begin(115200);
  Serial.printf("\n\nNAPT not supported in this configuration\n");
}

#endif

void loop() {
  server.handleClient();
  if (WiFi.status() != WL_CONNECTED) {
    digitalWrite(2, LOW);
    delay(1000);
    digitalWrite(2, HIGH);
    delay(1000);
  }
  else {
    digitalWrite(2, HIGH);
  }
}
