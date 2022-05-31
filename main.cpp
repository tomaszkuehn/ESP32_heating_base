#include <Arduino.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

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



  // Now set up tasks to run independently.
  xTaskCreate(
    TaskTest
    ,  "TaskTest"   // A name just for humans
    ,  1024  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL );

  xTaskCreatePinnedToCore(
    TaskHTTP
    ,  "HTTP"
    ,  16384  // Stack size
    ,  NULL
    ,  2  // Priority
    ,  NULL
    ,  CONFIG_ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    TaskNetwork
    ,  "Network"
    ,  4096  // Stack size
    ,  NULL
    ,  2  // Priority
    ,  NULL
    ,  CONFIG_ARDUINO_RUNNING_CORE);

  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

void loop()
{
  // Empty. Things are done in Tasks.
}


/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskTest(void *pvParameters)
{
  (void) pvParameters;

  for (;;)
  {
    vTaskDelay(1000);
    Serial.println("Test");
  }
}

void TaskHTTP(void *pvParameters)
{
  (void) pvParameters;

  for (;;)
  {
    unsigned int temp = uxTaskGetStackHighWaterMark(nullptr);
    Serial.printf("HTTP %d\n", temp);
    if(wificonnected > 10){
      String currentLine;
      WiFiClient client = server.available();
      if(client){
        Serial.println("Client");

        while (client.connected()) {
          if (client.available()) {
            char c = client.read();
            Serial.write(c);
            //header += c;
            if (c == '\n') {                    // if the byte is a newline character
              // if the current line is blank, you got two newline characters in a row.
              // that's the end of the client HTTP request, so send a response:
              if (currentLine.length() == 0) {
                // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
                // and a content-type so the client knows what's coming, then a blank line:
                handleRoot(client);
                /*
                client.println("HTTP/1.1 200 OK");
                client.println("Content-type:text/html");
                client.println("Connection: close");
                client.println();


                // Display the HTML web page
                client.println("<!DOCTYPE html><html>");
                client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
                client.println("<link rel=\"icon\" href=\"data:,\">");
                // CSS to style the on/off buttons
                // Feel free to change the background-color and font-size attributes to fit your preferences
                client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
                client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
                client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
                client.println(".button2 {background-color: #555555;}</style></head>");

                // Web Page Heading
                client.println("<body><h1>ESP32 Web Server</h1>");

                client.println("</body></html>");
                */

                // The HTTP response ends with another blank line
                client.println();
                // Break out of the while loop
                break;
              } else { // if you got a newline, then clear currentLine
                currentLine = "";
              }
            } else if (c != '\r') {  // if you got anything else but a carriage return character,
              currentLine += c;      // add it to the end of the currentLine
            }
          }
        }
        client.stop();
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
    Serial.printf("WiFi %d\n", wificonnected);
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



