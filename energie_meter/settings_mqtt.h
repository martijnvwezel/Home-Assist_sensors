#include <strings.h>
#include <PubSubClient.h>

// *************************************************
// * Configurble settings, change if needed
// *************************************************
#define IO_INV_RX_PIN 5
#define IO_REQUEST_DATA 4
#define BAUDRATE 115200
#define MAXLINELENGTH 128

// * static ip adress
IPAddress static_ip(192, 168, 1, 8);
IPAddress gateway_ip(192, 168, 1, 1);
IPAddress subnet_mask(255, 255, 255, 0);

// * To be filled with EEPROM data
char MQTT_HOST[64] = "FILL_IN";
char MQTT_PORT[6]  = "FILL_IN";
char MQTT_USER[32] = "FILL_IN";
char MQTT_PASS[32] = "FILL_IN";

// * settings for le wifi
const char* ssid           = "FILL_IN";
const char* password       = "FILL_IN";
const bool  outputOnSerial = false;
const char* hostName       = "energie-meter";

// *************************************************
// ** Non configurble settings
// *************************************************

// * The hostname of our little creature
#define HOSTNAME "p1meter"

// * MQTT Last reconnection counter
long LAST_RECONNECT_ATTEMPT = 0;

// * MQTT network settings
#define MQTT_MAX_RECONNECT_TRIES 5

// * MQTT root topic
#define MQTT_ROOT_TOPIC "sensors/power/p1meter"

// * Set to store the data values read
long CONSUMPTION_LOW_TARIF;  // Meter reading Electrics - consumption low tariff
long CONSUMPTION_HIGH_TARIF; // Meter reading Electrics - consumption high tariff

long RETURNDELIVERY_LOW_TARIF;  // Meterstand Elektra - teruglevering laag tarief
long RETURNDELIVERY_HIGH_TARIF; // Meterstand Elektra - teruglevering hoog tarief

long ACTUAL_CONSUMPTION;    // Meter reading Electrics - Actual consumption
long ACTUAL_RETURNDELIVERY; // Meter reading Electrics - Actual return
long GAS_METER_M3;

long L1_INSTANT_POWER_USAGE;
long L2_INSTANT_POWER_USAGE;
long L3_INSTANT_POWER_USAGE;
long L1_INSTANT_POWER_CURRENT;
long L2_INSTANT_POWER_CURRENT;
long L3_INSTANT_POWER_CURRENT;
long L1_VOLTAGE;
long L2_VOLTAGE;
long L3_VOLTAGE;

// Set to store data counters read
long ACTUAL_TARIF;
long SHORT_POWER_OUTAGES;
long LONG_POWER_OUTAGES;
long SHORT_POWER_DROPS;
long SHORT_POWER_PEAKS;

#define P1_MAXLINELENGTH 64
char buffer[P1_MAXLINELENGTH]; // Buffer voor seriele data om \n te vinden.
char telegram[P1_MAXLINELENGTH];
int  bufpos = 0;

// * Set during CRC checking
unsigned int currentCRC = 0;

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
        if (mqtt_client.connect(HOSTNAME, MQTT_USER, MQTT_PASS)) {
            Serial.println(F("MQTT connected!"));

            // * Once connected, publish an announcement...
            char* message = new char[16 + strlen(HOSTNAME) + 1];
            strcpy(message, "p1 meter alive: ");
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

void send_metric(String name, long metric) {
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
    send_metric("consumption_low_tarif", CONSUMPTION_LOW_TARIF);
    send_metric("consumption_high_tarif", CONSUMPTION_HIGH_TARIF);
    send_metric("returndelivery_low_tarif", RETURNDELIVERY_LOW_TARIF);
    send_metric("returndelivery_high_tarif", RETURNDELIVERY_HIGH_TARIF);
    send_metric("actual_consumption", ACTUAL_CONSUMPTION);
    send_metric("actual_returndelivery", ACTUAL_RETURNDELIVERY);

    send_metric("l1_instant_power_usage", L1_INSTANT_POWER_USAGE);
    send_metric("l2_instant_power_usage", L2_INSTANT_POWER_USAGE);
    send_metric("l3_instant_power_usage", L3_INSTANT_POWER_USAGE);
    send_metric("l1_instant_power_current", L1_INSTANT_POWER_CURRENT);
    send_metric("l2_instant_power_current", L2_INSTANT_POWER_CURRENT);
    send_metric("l3_instant_power_current", L3_INSTANT_POWER_CURRENT);
    send_metric("l1_voltage", L1_VOLTAGE);
    send_metric("l2_voltage", L2_VOLTAGE);
    send_metric("l3_voltage", L3_VOLTAGE);

    if (GAS_METER_M3 > 0) {
        send_metric("gas_meter_m3", GAS_METER_M3);
    }
    send_metric("actual_tarif_group", ACTUAL_TARIF);
    send_metric("short_power_outages", SHORT_POWER_OUTAGES);
    send_metric("long_power_outages", LONG_POWER_OUTAGES);
    send_metric("short_power_drops", SHORT_POWER_DROPS);
    send_metric("short_power_peaks", SHORT_POWER_PEAKS);
}
