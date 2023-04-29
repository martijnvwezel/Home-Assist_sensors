#include <PubSubClient.h>

// *************************************************
// * Configurble settings, change if needed
// *************************************************

const char* ssid     = "cc";
const char* password = "ccc";

// * static ip adress
IPAddress static_ip(192, 168, 1, 10);
IPAddress gateway_ip(192, 168, 1, 1);
IPAddress subnet_mask(255, 255, 255, 0);


// Your MQTT broker address and credentials
const char* mqtt_server   = "192.168.1.5";
const char* mqtt_user     = "ccc";
const char* mqtt_password = "ccc";
const int   mqtt_port     = 1883;


#define SENS_A A0
#define SENS_B A1
#define SENS_C A2
#define LED D6
#define LIGHT_SEN_ENABLE D5


// * The hostname of our little creature
#define HOSTNAME "MuinoWaterMeter"

// * MQTT Last reconnection counter
long LAST_RECONNECT_ATTEMPT = 0;

// * MQTT network settings
#define MQTT_MAX_RECONNECT_TRIES 5

// * MQTT root topic
#define MQTT_ROOT_TOPIC "MuinoWaterMeter"

uint32_t mili_liters_total = 0; // * since boot
uint32_t old_sensor_value = 0;
bool under_value = false;


// * Initiate WIFI client
WiFiClient espClient;

// * Initiate MQTT client
PubSubClient mqtt_client(espClient);

// * Reconnect to MQTT server and subscribe to in and out topics
bool mqtt_reconnect() {
    // * Loop until we're reconnected
    int MQTT_RECONNECT_RETRIES = 0;

    while (!mqtt_client.connected() && MQTT_RECONNECT_RETRIES < MQTT_MAX_RECONNECT_TRIES) {
        MQTT_RECONNECT_RETRIES++;
        Serial.printf("MQTT connection attempt %d / %d ...\n", MQTT_RECONNECT_RETRIES, MQTT_MAX_RECONNECT_TRIES);

        // * Attempt to connect
        if (mqtt_client.connect(HOSTNAME, mqtt_user, mqtt_password)) {
            Serial.println(F("MQTT connected!"));

            // * Once connected, publish an announcement...
            char* message = new char[16 + strlen(HOSTNAME) + 1];
            strcpy(message, "Water sensor alive: ");
            strcat(message, HOSTNAME);
            mqtt_client.publish("hass/status", message);

            Serial.printf("MQTT root topic: %s\n", MQTT_ROOT_TOPIC);
        } else {
            Serial.print(F("MQTT Connection failed: rc="));
            Serial.println(mqtt_client.state());
            Serial.println(F(" Retrying in 5 seconds"));
            Serial.println("");

            // * Wait 5 seconds before retrying
            delay(5000);
        }
    }

    if (MQTT_RECONNECT_RETRIES >= MQTT_MAX_RECONNECT_TRIES) {
        Serial.printf("*** MQTT connection failed, giving up after %d tries ...\n", MQTT_RECONNECT_RETRIES);
        return false;
    }

    return true;
}


// * Send a message to a broker topic
void send_mqtt_message(const char* topic, char* payload) {
    Serial.printf("MQTT Outgoing on %s: ", topic);
    Serial.println(payload);

    bool result = mqtt_client.publish(topic, payload, false);

    if (!result) {
        Serial.printf("MQTT publish to topic %s failed\n", topic);
    }
}









void send_metric(String name, uint32_t metric) {
    Serial.print(F("Sending metric to broker: "));
    Serial.print(name);
    Serial.print(F("="));
    Serial.println(metric);

    char output[10];
    ltoa(metric, output, sizeof(output));

    String topic = String(MQTT_ROOT_TOPIC) + "/" + name;
    send_mqtt_message(topic.c_str(), output);
}

void send_data_to_broker() {
    if (!mqtt_client.connected()) {
        long now = millis();

        if (now - LAST_RECONNECT_ATTEMPT > 5000) {
            LAST_RECONNECT_ATTEMPT = now;

            if (mqtt_reconnect()) {
                LAST_RECONNECT_ATTEMPT = 0;
            }
        }
    } else {
        mqtt_client.loop();
    }

    // send_metric("pulse", 500);
    send_metric("total_after_boot", mili_liters_total);
}
