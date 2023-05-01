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

uint32_t mili_liters_total = 0; // * since boot
uint32_t old_sensor_value = 0;
bool under_value = false;


static float mina = 3000, minb = 3000, minc = 3000;

#define BUFFER_SIZE 256
struct data_buffer {
    uint32_t sens_a[BUFFER_SIZE];
    uint32_t sen_a_pos = 0;

    uint32_t sens_b[BUFFER_SIZE];
    uint32_t sen_b_pos = 0;

    uint32_t sens_c[BUFFER_SIZE];
    uint32_t sen_c_pos = 0;
};



#define PI_3 1.0471975512
#define PI2_3 2.09439510239
#define PI2 6.28318530718
#define PI  3.14159265358



// * The hostname of our little creature
#define HOSTNAME "MuinoWaterMeter"

// * MQTT Last reconnection counter
long LAST_RECONNECT_ATTEMPT = 0;

// * MQTT network settings
#define MQTT_MAX_RECONNECT_TRIES 5

// * MQTT root topic
#define MQTT_ROOT_TOPIC "MuinoWaterMeter"


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
    // Serial.printf("MQTT Outgoing on %s: ", topic);
    // Serial.println(payload);

    bool result = mqtt_client.publish(topic, payload, false);

    if (!result) {
        Serial.printf("MQTT publish to topic %s failed\n", topic);
    }
}


void swap(float* xp, float* yp) {
    float temp = *xp;
    *xp        = *yp;
    *yp        = temp;
}

// A function to implement bubble sort
void bubbleSort(float arr[], int n) {
    for (int i = 0; i < n - 1; i++)

        // Last i elements are already in place
        for (int j = 0; j < n - i - 1; j++)
            if (arr[j] > arr[j + 1])
                swap(&arr[j], &arr[j + 1]);
}

float median(float array[], size_t length) {

    bubbleSort(array, length);

    if (length == 0) {
        printf("no median for empty data\n");
    }
    if (length % 2 == 1) {
        return array[length / 2];
    } else {
        int i = length / 2;
        return (array[i - 1] + array[i]) / 2;
    }
}







void send_metric(String name, uint32_t metric) {
    // Serial.print(F("Sending metric to broker: "));
    // Serial.print(name);
    // Serial.print(F("="));
    // Serial.println(metric);

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


// void update_minima(uint32_t sen_a, uint32_t sen_b, uint32_t sen_c) {
//     // Find minima and average
//     static uint32_t average_a = 0 , average_b = 0 , average_c = 0 ;
//     raw_data_buffer.sens_a[raw_data_buffer.sen_a_pos++] = sen_a;
//     raw_data_buffer.sens_b[raw_data_buffer.sen_b_pos++] = sen_b;
//     raw_data_buffer.sens_c[raw_data_buffer.sen_c_pos++] = sen_c;

//     if (raw_data_buffer.sen_a_pos >= BUFFER_SIZE) {
//         uint32_t cnt_a = 0, cnt_b = 0, cnt_c = 0;
//         for (size_t i = 0; i < BUFFER_SIZE; i++) {
//             if (raw_data_buffer.sens_a[i] > 10) {
//                 mina = MIN(mina, raw_data_buffer.sens_a[i]); // TODO if new minima is found check how many times that happend
//             }
//             if (raw_data_buffer.sens_b[i] > 10) {
//                 minb = MIN(minb, raw_data_buffer.sens_b[i]);
//             }
//             if (raw_data_buffer.sens_c[i] > 10) {
//                 minc = MIN(minc, raw_data_buffer.sens_c[i]);
//             }

//             cnt_a = cnt_a + raw_data_buffer.sens_a[i];
//             cnt_b = cnt_b + raw_data_buffer.sens_b[i];
//             cnt_c = cnt_c + raw_data_buffer.sens_c[i];
//         }

//         average_a = cnt_a = cnt_a / (BUFFER_SIZE);
//         average_b = cnt_b = cnt_b / (BUFFER_SIZE);
//         average_c = cnt_c = cnt_c / (BUFFER_SIZE);

//         raw_data_buffer.sen_a_pos = 0;
//         raw_data_buffer.sen_b_pos = 0;
//         raw_data_buffer.sen_c_pos = 0;
//     }
// }

// size_t minima_pos = 0;
// bool full_minim_arr = false;
// float minima_arr [32];
// float minimb_arr [32];
// float minimc_arr [32];

// float minima_value(float *minim_arr, float new_value){
//     minim_arr[minima_pos++] = new_value;
//     if (minima_pos >= 32) {
//         full_minim_arr = true;
//         minima_pos     = 0;
//     }
//     float minimc_arr[32];
//     memcpy(minimc_arr, minim_arr, minima_pos);
// }
