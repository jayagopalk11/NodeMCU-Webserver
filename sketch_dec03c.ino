#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "Rocket";
const char* passphrase = "FalconHeavy";
String st;
String content;
int statusCode;
String payload = "";
String webCode = "";
ESP8266WebServer server(80);


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  // put your setup code here, to run once:
  Serial.begin(115200);
  EEPROM.begin(512);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.println("Startup");
  // read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");
  String esid;
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  String epass = "";
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);
  String ssidEeprom = esid.c_str();
  if ( ssidEeprom.length() > 1 ) {
    Serial.print(">SSID: ");
    Serial.print(esid.c_str());
    Serial.print(">PASS: ");
    Serial.print(epass.c_str());
    WiFi.begin(esid.c_str(), epass.c_str());
    if (testWifi()) {
      Serial.println("tested wifi success");
      launchWeb(0);
      return;
    }


  }
  setupAP();
}

bool testWifi(void) {
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED) {
      return true;
    }
    delay(500);
    Serial.print(WiFi.status());
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

void setupAP(void) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0)
    Serial.println("no networks found");
  else
  {
    Serial.print(n);
    Serial.println(" networks found");
    st = "<ol>";
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");

      // Print SSID and RSSI for each network found
      st += "<li>";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
      st += "</li>";


      delay(10);
    }
    st += "</ol>";
  }
  Serial.println("");

  delay(100);
  WiFi.softAP(ssid, passphrase, 6);
  Serial.println("softap");
  launchWeb(1);
  Serial.println("over");
}

void launchWeb(int webtype) {
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer(webtype);
  // Start the server
  server.begin();
  Serial.println("Server started");
}


void createWebServer(int webtype)
{
  if ( webtype == 1 ) {
    server.on("/", []() {
      IPAddress ip = WiFi.softAPIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
      content += ipStr;
      content += "<p>";
      content += st;
      content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
      content += "</html>";
      server.send(200, "text/html", content);
    });
    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");

        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        EEPROM.commit();
        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
      }
      server.send(statusCode, "application/json", content);
    });
  } else if (webtype == 0) {
    server.on("/", []() {
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      server.send(200, "application/json", "{\"IP\":\"" + ipStr + "\"}");
    });
    server.on("/cleareeprom", []() {
      content = "<!DOCTYPE HTML>\r\n<html>";
      content += "<p>Clearing the EEPROM</p></html>";
      server.send(200, "text/html", content);
      Serial.println("clearing eeprom");
      for (int i = 0; i < 96; ++i) {
        EEPROM.write(i, 0);
      }
      EEPROM.commit();
    });
    server.on("/getTemp", []() {
      webCode = "";
      HTTPClient http;
      http.begin("http://api.openweathermap.org/data/2.5/weather?q=Chennai&appid=<appIdHere>");  //Specify request destination
      int httpCode = http.GET();                                                                  //Send the request


      if (httpCode > 0) { //Check the returning code

        payload = http.getString();   //Get the request response payload
        Serial.println(payload);                     //Print the response payload

      }
      digitalWrite(LED_BUILTIN, LOW);   // Turn the LED on (Note that LOW is the voltage level
      // but actually the LED is on; this is because
      // it is acive low on the ESP-01)
      delay(250);                      // Wait for a second
      digitalWrite(LED_BUILTIN, HIGH);  // Turn the LED off by making the voltage HIGH

      http.end();   //Close connection
      if (payload.length() > 0) {
        webCode += "<!DOCTYPE html>";
        webCode += "<html>";
        webCode += "<head>";
        webCode += "</head>";
        webCode += "<body onload=\"myFunction()\">";
        webCode += "<h1></h1>";
        webCode += "<img src=\"https://openweathermap.org/themes/openweathermap/assets/vendor/owm/img/logo_OpenWeatherMap_orange.svg\" alt=\"Mountain View\" width=\"300\" height=\"75\">";
        webCode += "<textarea type=\"text\" rows=\"15\" cols=\"150\" id=\"data\">";
        webCode += "</textarea>";
        webCode += "</body>";
        webCode += "<script>";
        webCode += "function myFunction() {";
        webCode += "var data = \"\";";
        webCode += "var obj = '" + payload + "';";
        webCode += "var x = JSON.parse(obj);  ";
        webCode += "data += \"Coordinates: \"+\"\\n\";";
        webCode += "data += \"Lon: \"+x.coord.lon+\"\\n\";";
        webCode += "data += \"Lat: \"+x.coord.lat+\"\\n\\n\";";
        webCode += "data += \"weather: \"+x.weather[0].main+\"\\n\";";
        webCode += "data += \"Temp: \"+x.main.temp+\"\\n\";";
        webCode += "data += \"humidity: \"+x.main.humidity+\"\\n\";";
        webCode += "data += \"visibility: \"+x.visibility+\"\\n\";";
        webCode += "data += \"wind speed: \"+x.wind.speed+\"\\n\";";
        webCode += "data += \"Wind deg: \"+x.wind.deg+\"\\n\";";
        webCode += "data += \"clouds: \"+x.clouds.all+\"\\n\\n\";";
        webCode += "data += \"sunrise: \"+x.sys.sunrise+\"\\n\";";
        webCode += "data += \"sunset: \"+x.sys.sunset+\"\\n\";";
        webCode += "var s = document.getElementById(\"data\");";
        webCode += "s.value = data;";
        webCode += "}";
        webCode += "</script>";
        webCode += "</html>";
      }
      else {
        webCode = "No response from API";
      }
      Serial.println(webCode);

      server.send(200, "text/html", webCode);

    });


  }
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
}
