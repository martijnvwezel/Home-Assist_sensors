#ifdef ESP32
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

// #include "conversions/img_converter_blieb.h"
// #include <img_converter.h>

Adafruit_MLX90640 mlx;
camera_fb_t       fb;
float             frame[32 * 24]; // buffer for full frame of temperatures

// low range of the sensor (this will be blue on the screen)
#define MINTEMP (float)20

// high range of the sensor (this will be red on the screen)
#define MAXTEMP (float)35

#define PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* _STREAM_BOUNDARY     = "\r\n--" PART_BOUNDARY "\r\n";
static const char* _STREAM_PART         = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

httpd_handle_t stream_httpd = NULL;
static uint    sent         = 0;
bool           fmt_2_jpg(uint8_t* src, size_t src_len, uint16_t width, uint16_t height, pixformat_t format, uint8_t quality, uint8_t** out, size_t* out_len) {
    int      jpg_buf_len = 64 * 1024;
    uint8_t* jpg_buf     = (uint8_t*)malloc(jpg_buf_len);
    if (jpg_buf == NULL) {
        return false;
    }

    Serial.println("Succesfull malloc..");
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
    if (res != ESP_OK) {
        Serial.print("1Res esp: ");
        Serial.println(res);
        return res;
    }
    Serial.println("Start While loop");
    while (true) {
        Serial.print("foto: ");
        Serial.println(sent++);
        if (mlx.getFrame(frame) != 0) {
            Serial.println("Failed");
            continue;
        } else {
            Serial.println("Got a frame..");
        }

        Serial.println("Foto:");
        int colorTemp;
        for (uint8_t h = 0; h < 24; h++) {
            for (uint8_t w = 0; w < 32; w++) {
                float t = frame[h * 32 + w];
                Serial.print(t, 1);
                Serial.print(", ");

                t = min(t, MAXTEMP);
                t = max(t, MINTEMP);

                uint8_t colorIndex = map(t, MINTEMP, MAXTEMP, 0, 255);
                colorIndex         = constrain(colorIndex, 0, 255);

                fb.buf[h * 32 + w] = (uint8_t)frame[h * 32 + w]; // camColors[colorIndex];
            }
        }
        Serial.println("");

        bool jpeg_converted = frame_2_jpg(&fb, 80, &_jpg_buf, &_jpg_buf_len);

        if (!jpeg_converted) {
            Serial.println("JPEG compression failed");
            res = ESP_FAIL;
        } else {
            Serial.println("JPEG compression success");
        }

        if (res == ESP_OK) {
            size_t hlen = snprintf((char*)part_buf, 64, _STREAM_PART, _jpg_buf_len);
            res         = httpd_resp_send_chunk(req, (const char*)part_buf, hlen);
            Serial.print("2Res esp: ");
            Serial.println(res);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, (const char*)_jpg_buf, _jpg_buf_len);
            Serial.print("3Res esp: ");
            Serial.println(res);
        }
        if (res == ESP_OK) {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
            Serial.print("4Res esp: ");
            Serial.println(res);
        }

        _jpg_buf = NULL;

        if (_jpg_buf) {
            free(_jpg_buf);
            _jpg_buf = NULL;
        }
        if (res != ESP_OK) {
            break;
        }
        Serial.printf("MJPG: %uB\n", (uint32_t)(_jpg_buf_len));
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
    fb.buf    = buffer;              /*!< Pointer to the pixel data */
    fb.len    = 24 * 32;             /*!< Length of the buffer in bytes */
    fb.width  = 32;                  /*!< Width of the buffer in pixels */
    fb.height = 24;                  /*!< Height of the buffer in pixels */
    fb.format = PIXFORMAT_GRAYSCALE; /*!< Format of the pixel data */
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
}