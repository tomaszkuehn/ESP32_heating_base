#include <Arduino.h>
#include <WiFi.h>
#include <esp_task_wdt.h>
#include <string>
#include <iostream>

using namespace std;

//#define DEBUG

#ifndef STASSID
#define STASSID "NCC"
#define STAPSK  "password"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

WiFiServer server(80);

uint32_t wificonnected = 0;




// define tasks
void TaskTest( void *pvParameters );
void TaskHTTP( void *pvParameters );
void TaskNetwork( void *pvParameters );

void handleRoot(WiFiClient client) {
  char temp[400];
  int sec = millis() / 1000;
  int min = sec / 60;
  int hr = min / 60;

  snprintf(temp, 400,
  "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n\
  <html>\
  <head>\
    <meta http-equiv=\"refresh\" content=\"2\" >\
    <title>ESP32 Demo</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Hello from ESP32!</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
  </body>\
</html>",

           hr, min % 60, sec % 60
          );
  client.print(temp);
}


void setup() {


  Serial.begin(115200);
  Serial.println();
  Serial.println("Booting Sketch...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  delay(4000);

  server.begin();



  // Set up tasks
  xTaskCreate(
    TaskTest
    ,  "TaskTest"
    ,  1024
    ,  NULL
    ,  2
    ,  NULL );

  xTaskCreatePinnedToCore(
    TaskHTTP
    ,  "HTTP"
    ,  16384
    ,  NULL
    ,  2
    ,  NULL
    ,  CONFIG_ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskNetwork
    ,  "Network"
    ,  4096
    ,  NULL
    ,  2
    ,  NULL
    ,  CONFIG_ARDUINO_RUNNING_CORE);

  // Task scheduler starts
}

void loop()
{

}


/*---------------------- Tasks ---------------------*/

void TaskTest(void *pvParameters)
{
  (void) pvParameters;

  for (;;)
  {
    vTaskDelay(1000);
#ifdef DEBUG
    Serial.println("Test");
#endif
  }
}

void TaskHTTP(void *pvParameters)
{
  (void) pvParameters;
  uint32_t timeout;
  std::string header, page;

  for (;;)
  {
    unsigned int temp = uxTaskGetStackHighWaterMark(nullptr);
#ifdef DEBUG
    Serial.printf("HTTP %d\n", temp);
#endif
    if(wificonnected > 10){
      String currentLine;
      WiFiClient client = server.available();
      if(client){
        Serial.println("Client");
        timeout = millis();
        while (client.connected() && (millis() - timeout < 2000)) {
          if (client.available()) {
            char c = client.read();
            Serial.write(c);
            header += c;
            if (c == '\n') {
              if (currentLine.length() == 0) {
                Serial.println("HTTP params");
			    header = header.substr(header.find(' ')+2);
			    std::cout << "Req: " << header << '\n';
			    std::string::size_type pos;
			    pos = header.find('.');
			    page = header.substr(0,pos);
			    std::cout << page << '\n';
            	if (page.find("temp") != string::npos) {
            		Serial.println("TEMP action");
            	}
                handleRoot(client);
                client.println();
                break; // Out of while
              } else {
                currentLine = "";
              }
            } else if (c != '\r') {
              currentLine += c;
            }
          }
        }
        client.stop();
        header = "";
      }
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
    //delay(100);
  }
}

void TaskNetwork(void *pvParameters)
{
  (void) pvParameters;

  for (;;)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
#ifdef DEBUG
    Serial.printf("WiFi %d\n", wificonnected);
#endif
    if (WiFi.status() != WL_CONNECTED) {
      wificonnected = 0;
      WiFi.mode(WIFI_AP_STA);
      WiFi.begin(ssid, password);

      Serial.println("WiFi failed, retrying.");
      vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
    else
    {
        wificonnected++;
    }
  }
}



