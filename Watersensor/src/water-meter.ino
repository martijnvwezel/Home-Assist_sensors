#ifdef ESP32
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#endif

#include <ArduinoOTA.h>
#include "settings.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"          //disable brownout problems#include "jpge_.h"
#include <soc/rtc_cntl_reg.h> //disable brownout problems
#include <esp_http_server.h>

#include <math.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define PI_3 1.0471975512

float phase_iter(float a, float b, float c){
    static float mina = 442, minb = 1140, minc = 312;
    static float phase = 1;
    mina = MIN(mina,a);
    minb = MIN(minb,b);
    minc = MIN(minc,c);

    const float epsilon = 0.1;
    float less = (a-mina)*pow(sin(phase-epsilon),2)+(b-minb)*pow(sin(phase-epsilon+PI_3),2)+(c-minc)*pow(sin(phase-epsilon-PI_3),2);
    float same = (a-mina)*pow(sin(phase),2)+(b-minb)*pow(sin(phase+PI_3),2)+(c-minc)*pow(sin(phase-PI_3),2);
    float more = (a-mina)*pow(sin(phase+epsilon),2)+(b-minb)*pow(sin(phase+epsilon+PI_3),2)+(c-minc)*pow(sin(phase+epsilon-PI_3),2);
    if(more>same && more>less)
        phase += 0.1;
    if(less>same && less>more)
        phase -= 0.1;
    return phase;
}

int magic_code_box(uint32_t sen_a, uint32_t sen_b, uint32_t sen_c) {
    // * Liter berekening
    // * asin^2(σ)+bsin^2(σ+π/3)+c*sin^3(σ-π/3)
    // * a⋅sin²(σ±ε)+b⋅sin²(σ±ε+π/3)+c⋅sin²(σ±ε-π/3)
    bool temp = under_value;


    float phase = phase_iter((float)sen_a,(float)sen_b,(float)sen_c);

    uint32_t mili_liters = (uint32_t)((phase*1000)/PI_3);
    old_sensor_value = mili_liters_total;
    mili_liters_total = mili_liters_total + mili_liters;

    if ((old_sensor_value+1000) > mili_liters_total){
        return 0;
    }

    return 1;
}


void do_water_measurment(){

    digitalWrite(LED, HIGH);
    delay(20);
    uint32_t sen_a = analogReadMilliVolts(SENS_A);
    uint32_t sen_b = analogReadMilliVolts(SENS_B);
    uint32_t sen_c = analogReadMilliVolts(SENS_C);
    digitalWrite(LED, LOW);


    Serial.print(sen_a, 6);
    Serial.print("  ");
    Serial.print(sen_b, 6);
    Serial.print("  ");
    Serial.println(sen_c, 6);

    if (magic_code_box(sen_a, sen_b, sen_c))
        send_data_to_broker();


    delay(80);
}

void setup() {

    Serial.begin(115200);
    Serial.print("Water sensor V1.0.0!");


    // Wi-Fi connection
    Serial.println("Connecting to " + String(ssid) + "..");
    WiFi.config(static_ip, gateway_ip, subnet_mask);
    WiFi.hostname(HOSTNAME);
    WiFi.mode(WIFI_STA);

    WiFi.begin(ssid, password);
   int deadCounter = 20;
    while (WiFi.status() != WL_CONNECTED && deadCounter-- > 0) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.waitForConnectResult() != WL_CONNECTED || WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi Connect Failed! Rebooting...");
        delay(1000);
        ESP.restart(); // solves, bug in the ESP
    } else {
        Serial.println("");
        Serial.print("Connected to ");
        Serial.println(ssid);
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    }


   // Port defaults to 8266
    // ArduinoOTA.setPort(8266);

    // Hostname defaults to esp8266-[ChipID]
    ArduinoOTA.setHostname(HOSTNAME);

    // No authentication by default
    // ArduinoOTA.setPassword((const char *)"123");

    ArduinoOTA.onStart([]() { Serial.println("Start"); });
    ArduinoOTA.onEnd([]() { Serial.println("\nEnd"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) { Serial.printf("Progress: %u%%\r", (progress / (total / 100))); });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("Error[%u]: ", error);
        if (error == OTA_AUTH_ERROR)
            Serial.println("Auth Failed");
        else if (error == OTA_BEGIN_ERROR)
            Serial.println("Begin Failed");
        else if (error == OTA_CONNECT_ERROR)
            Serial.println("Connect Failed");
        else if (error == OTA_RECEIVE_ERROR)
            Serial.println("Receive Failed");
        else if (error == OTA_END_ERROR)
            Serial.println("End Failed");
    });
    ArduinoOTA.begin();

    pinMode(SENS_A, INPUT); // ADC 0
    pinMode(SENS_B, INPUT); // ADC 1
    pinMode(SENS_C, INPUT); // ADC 2
    pinMode(LED, OUTPUT);
    pinMode(LIGHT_SEN_ENABLE, OUTPUT);
    digitalWrite(LIGHT_SEN_ENABLE, HIGH);

    // * Setup MQTT
    Serial.printf("MQTT connecting to: %s:%d\n", mqtt_server, mqtt_port);
    mqtt_client.setServer(mqtt_server, mqtt_port);



}

void loop() {

    ArduinoOTA.handle();






    do_water_measurment();


}

// 1132 044 3352

// * Following needed for platform io, remove when using Arduino IDE
int main() {
    setup();
    loop();
    return 1;
}
