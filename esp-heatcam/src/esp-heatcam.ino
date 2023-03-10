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
#include "soc/soc.h"          //disable brownout problems
#include <soc/rtc_cntl_reg.h> //disable brownout problems
#include <esp_http_server.h>

Adafruit_MLX90640 mlx;
camera_fb_t       fb;

// low range of the sensor (this will be blue on the screen)
static int MINTEMP = 15;
// high range of the sensor (this will be red on the screen)
static int MAXTEMP = 30;

// after 5 frames the frame is generating a new value
static int no_frames_handled = 5;
static int average_frames    = 25; // average of the frame temp
static int average_frame_ind = 0;  // idex

float frame[32 * 24]; // buffer for full frame of temperatures

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY     = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART         = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
static uint    sent         = 0;

bool fmt_2_jpg(uint8_t* src, size_t src_len, uint16_t width, uint16_t height, pixformat_t format, uint8_t quality, uint8_t** out, size_t* out_len) {
    int      jpg_buf_len = 8 * 1024;
    uint8_t* jpg_buf     = (uint8_t*)malloc(jpg_buf_len);
    if (jpg_buf == NULL) {
        return false;
    }

    memory_stream dst_stream(jpg_buf, jpg_buf_len);

    if (!convert_image(src, width, height, format, quality, &dst_stream)) {
        free(jpg_buf);
        return false;
    }

    *out     = jpg_buf;
    *out_len = dst_stream.get_size();
    return true;
}

bool frame_2_jpg(camera_fb_t* fb, uint8_t quality, uint8_t** out, size_t* out_len) {
    return fmt_2_jpg(fb->buf, fb->len, fb->width, fb->height, fb->format, quality, out, out_len);
}

static esp_err_t stream_handler(httpd_req_t* req) {
    // camera_fb_t *fb = NULL;
    esp_err_t res          = ESP_OK;
    size_t    _jpg_buf_len = 0;
    uint8_t*  _jpg_buf     = NULL;
    char*     part_buf[64];

    res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
    if (res != ESP_OK)
        return res;

    Serial.println("Start While loop");
    while (true) {
        uint32_t timestamp = millis();
        // Serial.print("foto: ");
        // Serial.println(sent++);
        if (mlx.getFrame(frame) != 0) {
            Serial.println("Failed");

            Serial.println("Adafruit MLX90640 Camera");
            if (!mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
                Serial.println("Not Found Adafruit MLX90640");
            } else {
                Serial.println("Found Adafruit MLX90640");
            }
            Serial.print("Serial number: ");
            Serial.print(mlx.serialNumber[0], HEX);
            Serial.print(mlx.serialNumber[1], HEX);
            Serial.println(mlx.serialNumber[2], HEX);

            mlx.setMode(MLX90640_CHESS);
            mlx.setResolution(MLX90640_ADC_18BIT);
            mlx.setRefreshRate(MLX90640_8_HZ);
            Wire.setClock(1000000); // max 1 MHz
        }

        // // after 5 frames the frame is generating a new value
        // static int no_frames_handled = 5;
        // static int average_frames
        // static int average_frame_ind = 0; // idex

        int16_t temp_value = 0;
        for (uint16_t i = 0; i < 24 * 32; i++) {
            temp_value = temp_value + (int16_t)frame[i];
        }

        average_frames = average_frames + temp_value / (24 * 32);

        if (++no_frames_handled >= 5) {
            no_frames_handled = 0;
            average_frames    = average_frames / 5;

            MAXTEMP = average_frames + 7;
            MINTEMP = average_frames - 7;

            MAXTEMP = min(MAXTEMP, 80);
            MINTEMP = max(MINTEMP, -25);

            Serial.print(MAXTEMP);
            Serial.print(", ");
            Serial.print(MINTEMP);
            Serial.println("");

            average_frames = 0;
        }

        int colorTemp;
        for (uint8_t h = 0; h < 24; h++) {
            for (uint8_t w = 0; w < 32; w++) {
                float t = frame[h * 32 + w];
                // Serial.print(t, 1); Serial.print(", ");

                t = min((int)t, MAXTEMP);
                t = max((int)t, MINTEMP);

                uint8_t colorIndex = map(t, MINTEMP, MAXTEMP, 0, 255);
                colorIndex         = constrain(colorIndex, 0, 255);

                fb.buf[h * 32 + w] = (uint8_t)(camColors[colorIndex] & 0xFF);
            }
        }

        bool jpeg_converted = frame_2_jpg(&fb, 80, &_jpg_buf, &_jpg_buf_len);

        if (!jpeg_converted) {
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
        }

        if (res == ESP_OK) {
            size_t hlen = snprintf((char*)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res         = httpd_resp_send_chunk(req, (const char*)part_buf, hlen);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char*)_jpg_buf, _jpg_buf_len);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
        }

        if (_jpg_buf) {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if (res != ESP_OK) {
            return res;
        }
        // Serial.printf("MJPG: %uB\n", (uint32_t)(_jpg_buf_len));
        // Serial.print((millis() - timestamp) / 2);
        // Serial.println(" ms per frame (2 frames per display)");
    }
    return res;
}

void startCameraServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port    = 80;

    httpd_uri_t index_uri = {.uri = "/", .method = HTTP_GET, .handler = stream_handler, .user_ctx = NULL};

    // Serial.printf("Starting web server on port: '%d'\n", config.server_port);
    if (httpd_start(&stream_httpd, &config) == ESP_OK) {
        httpd_register_uri_handler(stream_httpd, &index_uri);
    }
}

void setup() {

    Serial.begin(115200);
    Serial.print("Total heap: ");
    Serial.println(ESP.getHeapSize());
    Serial.print("Free heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("Total PSRAM: ");
    Serial.println(ESP.getPsramSize());
    Serial.print("Free PSRAM: ");
    Serial.println(ESP.getFreePsram());
    // uint8_t * jpg_buf = (uint8_t *)malloc(32*24);
    // Serial.println(jpg_buf);

    uint8_t buffer[32 * 24];
    fb.buf    = buffer;           /*!< Pointer to the pixel data */
    fb.len    = 24 * 32;          /*!< Length of the buffer in bytes */
    fb.width  = 32;               /*!< Width of the buffer in pixels */
    fb.height = 24;               /*!< Height of the buffer in pixels */
    fb.format = PIXFORMAT_RGB565; /*!< Format of the pixel data */
    // fb.timeval           = 0;                   /*!< Timestamp since boot of the first DMA buffer of the frame */

    delay(100);

    Serial.println("Adafruit MLX90640 Camera");
    if (!mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
        Serial.println("Not Found Adafruit MLX90640");
    } else {
        Serial.println("Found Adafruit MLX90640");
    }
    Serial.print("Serial number: ");
    Serial.print(mlx.serialNumber[0], HEX);
    Serial.print(mlx.serialNumber[1], HEX);
    Serial.println(mlx.serialNumber[2], HEX);

    mlx.setMode(MLX90640_CHESS);
    mlx.setResolution(MLX90640_ADC_18BIT);
    mlx.setRefreshRate(MLX90640_8_HZ);
    Wire.setClock(1000000); // max 1 MHz

    // Wi-Fi connection
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected");

    Serial.print("Camera Stream Ready! Go to: http://");
    Serial.print(WiFi.localIP());

    // Start streaming web server
    startCameraServer();
}

void loop() {

    delay(1);
    if (mlx.getFrame(frame) != 0) {
        Serial.println("Failed");
    } else {
        Serial.println("Got a frame..");
    }
}

// * Following needed for platform io, remove when using Arduino IDE
int main() {
    setup();
    loop();
    return 1;
}
