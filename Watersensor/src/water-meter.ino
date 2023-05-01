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
// #define MIN_AVERAGE(X, Y) (((X+10) <= (Y) && (Y) > 10 ) ? (X) : ((Y+X)/2))
struct data_buffer raw_data_buffer;

#define ALPHA_COR 0.1 // Value between 0-1
float mini_average(float x, float y) {
    if ((x + 10) <= y && y > 10) {
        return x;
    } else {
        return (1 - ALPHA_COR) * x + ALPHA_COR * y;
    }
}

// float phase_iter(float a, float b, float c, float mina, float minb, float minc, float epsilon, float amplitude) {
//     static float phase = 0;
//     float        less  = (a + amplitude - mina) * sin(phase - epsilon) + (b + amplitude - minb) * sin(phase - epsilon + PI2_3) + (c + amplitude - minc) * sin(phase - epsilon - PI2_3);
//     float        same  = (a + amplitude - mina) * sin(phase) + (b + amplitude - minb) * sin(phase + PI2_3) + (c + amplitude - minc) * sin(phase - PI2_3);
//     float        more  = (a + amplitude - mina) * sin(phase + epsilon) + (b + amplitude - minb) * sin(phase + epsilon + PI2_3) + (c + amplitude - minc) * sin(phase + epsilon - PI2_3);
//     if (more > same && more > less)
//         phase += epsilon;
//     if (less > same && less > more)
//         phase -= epsilon;
//     return phase;
// }

float phase_iter(float phase, float a, float b, float c) {
    // * Liter berekening
    // * Amplitude voor evenwichtspunt
    // * asin^2(σ)+bsin^2(σ+π/3)+c*sin^3(σ-π/3)
    // * a⋅sin²(σ±ε)+b⋅sin²(σ±ε+π/3)+c⋅sin²(σ±ε-π/3)

    float sin1 = sin(phase);
    float sin2 = sin(phase + PI2_3);
    float sin3 = -sin2; // sin(phase + PI4_3);
    float less = a * sin3 + b * sin1 + c * sin2;
    float same = a * sin1 + b * sin2 + c * sin3;
    float more = a * sin2 + b * sin3 + c * sin1;
    if (more > same && more >= less)
        return phase + PI_3;
    else if (less > same)
        return phase - PI_3;
    else
        return phase;
}

int magic_code_box(float sen_a, float sen_b, float sen_c) {
    static float prev_a = 0, prev_b = 0, prev_c = 0;
    static float phase = 0;

    // * Calculate minimum value
    mina = mini_average(mina, sen_a);
    minb = mini_average(minb, sen_b);
    minc = mini_average(minc, sen_c);

    // * Add minima + amplitude to signal to get zerocrossing
    float amplitude = 50.0;
    float a = sen_a - mina + amplitude;
    float b = sen_b - minb + amplitude;
    float c = sen_c - minc + amplitude;





    // * Get amount of liters
    phase = phase_iter(phase, a, b, c);
    // float    phase       = phase_iter(sen_a, sen_b, sen_c,  mina, minb, minc, 0.5, 50.0);
    uint32_t mili_liters = ((uint32_t)((phase * 100) / PI2)) * 10;
    mili_liters_total    = mili_liters;

    prev_a = sen_a;
    prev_b = sen_b;
    prev_c = sen_c;

    // if ((old_sensor_value+1000) > mili_liters_total) {
    //     old_sensor_value = mili_liters_total;
    //     return 1;
    // }

    return 1;
}

void do_water_measurment() {

    digitalWrite(LED, HIGH);
    delay(5);
    uint32_t sen_a = analogReadMilliVolts(SENS_A);
    uint32_t sen_b = analogReadMilliVolts(SENS_B);
    uint32_t sen_c = analogReadMilliVolts(SENS_C);
    digitalWrite(LED, LOW);

    // Serial.print(sen_a);
    // Serial.print("  ");
    // Serial.print(sen_b);
    // Serial.print("  ");
    // Serial.print(sen_c);
    // Serial.print("  ");
    // Serial.print(mina);
    // Serial.print("  ");
    // Serial.print(minb);
    // Serial.print("  ");
    // Serial.print(minc);
    // Serial.print("  ");
    // Serial.println(mili_liters_total);

    bool send = magic_code_box((float)sen_a, (float)sen_b, (float)sen_c);
    if (send)
        send_data_to_broker();

    delay(80);
}

void setup() {

    Serial.begin(115200);
    Serial.print("Water sensor V1.0.3!");

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

    // ArduinoOTA.handle();
    do_water_measurment();
}

// 1132 044 3352

// * Following needed for platform io, remove when using Arduino IDE
int main() {
    setup();
    loop();
    return 1;
}
