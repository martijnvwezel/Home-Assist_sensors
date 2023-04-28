#include "esphome.h"

#define SENS_A A0
#define SENS_B A1
#define SENS_C A2
#define LED D6
#define LIGHT_SEN_ENABLE D5

class MuinoWaterSensor : public Component,  public CustomMQTTDevice {
public:
    void setup() override {
        pinMode(SENS_A, INPUT); // ADC 0
        pinMode(SENS_B, INPUT); // ADC 1
        pinMode(SENS_C, INPUT); // ADC 2
        pinMode(LED, OUTPUT);
        pinMode(LIGHT_SEN_ENABLE, OUTPUT);
        digitalWrite(LIGHT_SEN_ENABLE, HIGH);

        subscribe("MuinoWaterSensor", &MuinoWaterSensor::on_message);

    }

    int magic_code_box(uint32_t sen_a, uint32_t sen_b, uint32_t sen_c) {
        // * Liter berekening
        // * asin^2(σ)+bsin^2(σ+π/3)+c*sin^3(σ-π/3)
        // * a⋅sin²(σ±ε)+b⋅sin²(σ±ε+π/3)+c⋅sin²(σ±ε-π/3)
        bool temp = this.under_value;
        // * Sent each, 500 mililiters
        if (sen_a < 500) {
            this.under_value = true;
        } else {
            this.under_value = false;
        }


      if (temp != this->under_value){
        publish("MuinoWaterSensor/pulse", 500); // * value in mililiters

        this.mili_liters_total = this.mili_liters_total + 500;

        // * Send total after calculation
        publish("MuinoWaterSensor/total_after_boot", this.mili_liters_total);
      }




        return 1;
    }
    void loop() override {
        digitalWrite(LED, HIGH);
        delay(20);
        uint32_t sen_a = analogReadMilliVolts(SENS_A);
        uint32_t sen_b = analogReadMilliVolts(SENS_B);
        uint32_t sen_c = analogReadMilliVolts(SENS_C);

        magic_code_box(sen_a, sen_b, sen_c);

        // * Log values
        ESP_LOGD("loop", "%d %d %d", sen_a, sen_b, sen_c);

        delay(60);
    }
private:
  uint32_t mili_liters_total = 0; // * since boot
  bool under_value = false;
};





