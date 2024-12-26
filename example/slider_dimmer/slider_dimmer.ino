#include <arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>

#include <WiFi.h> 
#include <ESPAsyncWebServer.h> 
const char* ssid = "Trox_2.4G"; 
const char* password = "Zxcvbnm1983"; 

// s3
// #define SCR_Pin 9
// #define RELAY_PIN 20
// #define LED_PIN 19
// #define ZCD_PIN 10

// ESP32
#define SCR_Pin 25
#define RELAY_PIN 26
#define LED_PIN 32
#define ZCD_PIN 33

#define AC_CTRL_OFF digitalWrite(SCR_Pin, LOW)
#define AC_CTRL_ON digitalWrite(SCR_Pin, HIGH)

#define RELAY_OFF digitalWrite(RELAY_PIN, LOW)
#define RELAY_ON digitalWrite(RELAY_PIN, HIGH)

#define LED_OFF digitalWrite(LED_PIN, HIGH)
#define LED_ON digitalWrite(LED_PIN, LOW)

unsigned char dim = 0;

AsyncWebServer server(80); 

SemaphoreHandle_t dimSemaphore;
void createSemaphore(){
    dimSemaphore = xSemaphoreCreateMutex();
    xSemaphoreGive( ( dimSemaphore) );
}

// Lock the variable indefinietly. ( wait for it to be accessible )
void lockVariable(){
    xSemaphoreTake(dimSemaphore, dim);
}

// give back the semaphore.
void unlockVariable(){
    xSemaphoreGive(dimSemaphore);
}

const char mainPage[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP WiFi Dimmer</title>
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.3rem;}
    p {font-size: 1.9rem;}
    body {max-width: 400px; margin:0px auto; padding-bottom: 25px;}
    .slider { -webkit-appearance: none; margin: 14px; width: 360px; height: 25px; background: #FFD65C;
      outline: none; -webkit-transition: .2s; transition: opacity .2s;}
    .slider::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; background: #003249; cursor: pointer;}
    .slider::-moz-range-thumb { width: 35px; height: 35px; background: #003249; cursor: pointer; } 
  </style>
</head>
<body>
  <h2>ESP Web Server</h2>
  <p><span id="textSliderValue">%SLIDERVALUE%</span></p>
  <p><input type="range" onchange="updateSliderPWM(this)" id="pwmSlider" min="0" max="100" value="%SLIDERVALUE%" step="1" class="slider"></p>
<script>
function updateSliderPWM(element) {
  var sliderValue = document.getElementById("pwmSlider").value;
  document.getElementById("textSliderValue").innerHTML = sliderValue;
  console.log(sliderValue);
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/update?value="+sliderValue, true);
  xhr.send();
}
</script>
</body>
</html>
)rawliteral";

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(SCR_Pin, OUTPUT);
    pinMode(ZCD_PIN, INPUT);
    LED_OFF;
    RELAY_OFF;
    AC_CTRL_OFF;

    Serial.begin(115200); // initialize the serial communication:
    delay(1000);

    RELAY_ON;
    AC_CTRL_ON;
    delay(2000);
    RELAY_OFF;
    AC_CTRL_OFF;

    createSemaphore();

    server_setup();

    attachInterrupt(ZCD_PIN, zero_cross_int, RISING); // CHANGE FALLING RISING

    Serial.println("Test begin");
    set_power(5);
}

void loop()
{
    //dimmer_server();

    // for (i = 1; i < 10; i++)
    // {
    //     set_power(i);
    //     delay(500);
    // }
    // set_power(0);
    // delay(1000);
}

void zero_cross_int() // function to be fired at the zero crossing to dim the light
{
    if (dim < 5)
        return;
    if (dim > 95)
        return;

    int dimtime = (2 * 100 * dim);
    delayMicroseconds(dimtime); // Off cycle
    AC_CTRL_ON;                 // triac firing
    delayMicroseconds(500);     // triac On propagation delay
    AC_CTRL_OFF;                // triac Off
}

void set_power(int level)
{
    lockVariable();
    dim = map(level, 0, 100, 95, 5);
    if (level == 0)
    {
        RELAY_OFF;
    }
    else
        RELAY_ON;
    unlockVariable();
    delay(100); 
}

void server_setup()
{
    WiFi.disconnect();

    WiFi.begin(ssid, password);

    int connect_count = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
        vTaskDelay(500);
        Serial.print(".");
        connect_count++;
        if (connect_count > 20)
        {
            Serial.println("Wifi error");
            break;
        }
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
    }

    // Define a route to serve the HTML page 
    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) { 
      Serial.println("ESP32 Web Server: New request received:");  // for debugging 
      Serial.println("GET /");        // for debugging 
      request->send(200, "text/html", mainPage); 
    }); 
    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
 
    int paramsNr = request->params();
    Serial.println(paramsNr);
    
    AsyncWebParameter* p = request->getParam(0);
    Serial.print("Param name: ");
    Serial.println(p->name());
    Serial.print("Param value: ");
    Serial.println(p->value());
    Serial.println("------");

    set_power(atoi(p->value().c_str()));
 
    request->send(200, "text/plain", "message received");
  });

    server.begin();
}

void req_explain(String str)
{
    char temp[50];

    str.replace("&", " ");
    sprintf(temp, "%s", str.c_str());
    Serial.println(temp);

    int var_1 = 0;

    sscanf(temp, "?value=%d HTTP", &var_1);
    Serial.println(var_1);

    set_power(var_1);
}
