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

#include <Adafruit_MLX90640.h>
#include "settings.h"
#include "Arduino.h"
#include "fb_gfx.h"
#include "soc/soc.h"          //disable brownout problems#include "jpge_.h"
#include <soc/rtc_cntl_reg.h> //disable brownout problems
#include <esp_http_server.h>



#define SENS_A A0
#define SENS_B A1
#define SENS_C A2
#define LED D6
#define LIGHT_SEN_ENABLE D5

void setup() {

    Serial.begin(115200);
    Serial.print("sensor: ");
    // Serial.println(ESP.getHeapSize());
    // Serial.print("Free heap: ");
    // Serial.println(ESP.getFreeHeap());
    // Serial.print("Total PSRAM: ");
    // Serial.println(ESP.getPsramSize());
    // Serial.print("Free PSRAM: ");
    // Serial.println(ESP.getFreePsram());

    // // Wi-Fi connection
    // WiFi.begin(ssid, password);
    // while (WiFi.status() != WL_CONNECTED) {
    //     delay(500);
    //     Serial.print(".");
    // }
    // Serial.println("");
    // Serial.println("WiFi connected");

    // Serial.print("Camera Stream Ready! Go to: http://");
    // Serial.print(WiFi.localIP());

    pinMode(SENS_A, INPUT); // ADC 0
    pinMode(SENS_B, INPUT); // ADC 1
    pinMode(SENS_C, INPUT); // ADC 2
    pinMode(LED, OUTPUT);
    pinMode(LIGHT_SEN_ENABLE, OUTPUT);
    digitalWrite(LIGHT_SEN_ENABLE, HIGH);

}

void loop() {
    digitalWrite(LED, HIGH);
    delay(20);
    uint32_t sen_a = analogReadMilliVolts(SENS_A);
    uint32_t sen_b = analogReadMilliVolts(SENS_B);
    uint32_t sen_c = analogReadMilliVolts(SENS_C);
    digitalWrite(LED, LOW);
    // for (int i = 0; i < 16; i++) {
    //     Vbatt = Vbatt + analogReadMilliVolts(A0); // ADC with correction
    // }
    // float Vbattf = 2 * Vbatt / 16 / 1000.0; // attenuation ratio 1/2, mV --> V
    Serial.print(sen_a, 6);
    Serial.print("  ");
    Serial.print(sen_b, 6);
    Serial.print("  ");
    Serial.println(sen_c, 6);

    delay(60);
}

// 1132 044 3352

// * Following needed for platform io, remove when using Arduino IDE
int main() {
    setup();
    loop();
    return 1;
}
