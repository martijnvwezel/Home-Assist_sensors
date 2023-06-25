// docker run   -v "${PWD}":/config  -it ghcr.io/esphome/esphome run muinowatersensor.yaml

// #include <Arduino.h>
#include "esphome.h"

#define SMOOTHING_FACTOR 3 // 2 - 10
#define AC_STEPS 16        // given pi/3 coarse estimate of phase, calculate autocorrelation of signals within that pi/3 range
#define ALPHA_COR 0.1      // Value between 0-1

//* Some calculations help
#define PI_3 1.0471975512
#define PI2_3 2.09439510239
#define LOW 0
#define HIGH 1

#define SENS_A A0
#define SENS_B A1
#define SENS_C A2
#define LED D6
#define LIGHT_SEN_ENABLE D5

struct state_t {
    int8_t  phase  = 0;
    int8_t  fine   = 0;
    int32_t liters = 0;

    int a_min = 2500;
    int b_min = 2500;
    int c_min = 2500;

    int a_max = 0;
    int b_max = 0;
    int c_max = 0;
};

class MuinoWaterSensor : public PollingComponent,  public Sensor {
public:
    Sensor* watersensor_sensor = new Sensor();

    MuinoWaterSensor() : PollingComponent(15000) {
    }
    float get_setup_priority() const override { return esphome::setup_priority::HARDWARE; }

    void setup() override {
        pinMode(SENS_A, INPUT); // ADC 0
        pinMode(SENS_B, INPUT); // ADC 1
        pinMode(SENS_C, INPUT); // ADC 2
        pinMode(LED, OUTPUT);
        pinMode(LIGHT_SEN_ENABLE, OUTPUT);
        digitalWrite(LIGHT_SEN_ENABLE, HIGH);

        // subscribe("MuinoWaterSensor", &MuinoWaterSensor::on_message);
    }

    int magic_code_box(int sen_a, int sen_b, int sen_c) {
        // * Liter berekening
        // * asin^2(σ)+bsin^2(σ+π/3)+c*sin^3(σ-π/3)
        // * a⋅sin²(σ±ε)+b⋅sin²(σ±ε+π/3)+c⋅sin²(σ±ε-π/3)

        static bool not_inited = true; // if start of pulse detected ignore liters overcommunicating

        if (not_inited) {
            this->state.phase  = 0;
            this->state.fine   = 0;
            this->state.liters = 0;

            this->state.a_min = sen_a;
            this->state.b_min = sen_b;
            this->state.c_min = sen_c;

            this->state.a_max = 0;
            this->state.b_max = 0;
            this->state.c_max = 0;
            not_inited  = false;
        }

        float alpha_cor = 0.01;
        if (this->mili_liters_total < 2) {
            alpha_cor = 0.1; // when 2 liter not found correct harder
            if (this->state.liters < 0) {
                this->state.liters = 0;
            }
        }

        // * Calculate minimum value
        this->state.a_min = mini_average(this->state.a_min, sen_a, alpha_cor);
        this->state.b_min = mini_average(this->state.b_min, sen_b, alpha_cor);
        this->state.c_min = mini_average(this->state.c_min, sen_c, alpha_cor);

        this->state.a_max = max_average(this->state.a_max, sen_a, alpha_cor);
        this->state.b_max = max_average(this->state.b_max, sen_b, alpha_cor);
        this->state.c_max = max_average(this->state.c_max, sen_c, alpha_cor);

        int a_zc = (this->state.a_min + this->state.a_max) >> 1;
        int b_zc = (this->state.b_min + this->state.b_max) >> 1;
        int c_zc = (this->state.c_min + this->state.c_max) >> 1;

        int sa = sen_a - a_zc;
        int sb = sen_b - b_zc;
        int sc = sen_c - c_zc;

        phase_coarse_iter(sa, sb, sc);
        phase_fine_iter(sa, sb, sc);
        magnitude_offset_iter(sen_a, sen_b, sen_c);

        float liters_float_coarse = (float)this->state.liters + ((float)this->state.liters / 6);
        float liters_float_fine   = (float)this->state.liters + ((float)this->state.phase / 6) + ((float)this->state.fine / (16 * 6));

        uint32_t mililiters    = (uint32_t)(liters_float_fine * 1000);
        this->mili_liters_total = mililiters;

        return 1;
    }
    void loop() override {
        digitalWrite(LED, HIGH);
        delay(5);
        int32_t sen_a = analogReadMilliVolts(SENS_A);
        int32_t sen_b = analogReadMilliVolts(SENS_B);
        int32_t sen_c = analogReadMilliVolts(SENS_C);

        digitalWrite(LED, LOW);
        delay(5);

        int32_t sen_a_zero = analogReadMilliVolts(SENS_A);
        int32_t sen_b_zero = analogReadMilliVolts(SENS_B);
        int32_t sen_c_zero = analogReadMilliVolts(SENS_C);

        sen_a = sen_a - sen_a_zero;
        sen_b = sen_b - sen_b_zero;
        sen_c = sen_c - sen_c_zero;

        bool send = magic_code_box(sen_a, sen_b, sen_c);

        // if (send && sender++ > 3000) {
        //     send_data_to_broker();
        ESP_LOGD(this->mili_liters_total);
        //     sender = 0;
        // }

        // // * Log values
        // ESP_LOGD("loop", "%d %d %d", sen_a, sen_b, sen_c);

        delay(5);
    }

    void update() override {
        // * Send total after calculation
        this->watersensor_sensor->publish_state(this->mili_liters_total);
    }

private:
    uint32_t mili_liters_total = 0; // * since boot

    int sender = 0;                 // * Send only 1 update each 30 seconds

    bool           not_inited = true;
    static state_t state;

    float mini_average(float x, float y, float alpha_cor) {

        if ((x + 5) <= y && y > 10) {
            return x;
        } else {
            return (1 - alpha_cor) * x + alpha_cor * y;
        }
    }

    float max_average(float x, float y, float alpha_cor) {
        if ((x - 5) >= y && y < 2500) {
            return x;
        } else {
            return (1 - alpha_cor) * x + alpha_cor * y;
        }
    }

    void magnitude_offset_iter(int a, int b, int c) {
        int8_t phase = this->state.phase;
        if (this->state.fine > 8) {
            phase = (phase + 7) % 6;
        }
        int* a_min     = &this->state.a_min;
        int* b_min     = &this->state.b_min;
        int* c_min     = &this->state.c_min;
        int* a_max     = &this->state.a_max;
        int* b_max     = &this->state.b_max;
        int* c_max     = &this->state.c_max;
        int* u[6]      = {a_max, b_min, c_max, a_min, b_max, c_min};
        int* signal[6] = {&a, &b, &c, &a, &b, &c};
        if (this->state.liters > 2) {
            if ((this->state.fine > 14) || this->state.fine < 3)
                *u[phase] = (((((*u[phase]) << SMOOTHING_FACTOR) - (*u[phase]) + *signal[phase]) >> SMOOTHING_FACTOR) + 1 - (phase & 1));
        }
    }

    // check with three 2pi/3 steps peak autocorrelation and adjust towards max by pi/3 steps
    // 2cos(0)=2 2cos(pi/3)=1 2cos(2pi/3)=-1 2cos(pi)=-2 2cos(4pi/3)=-1 2cos(5pi/3)=1
    void phase_coarse_iter(int a, int b, int c) {
        int8_t*  phase  = &this->state.phase;
        int32_t* liters = &this->state.liters;
        short    pn[5];
        if (*phase & 1)
            pn[0] = a + a - b - c, pn[1] = b + b - a - c,
            pn[2] = c + c - a - b;     // same
        else
            pn[0]     = b + c - a - a, // less
                pn[1] = a + c - b - b, // more
                pn[2] = a + b - c - c; // same
        pn[3] = pn[0], pn[4] = pn[1];
        short i = *phase > 2 ? *phase - 3 : *phase;
        if (pn[i + 2] < pn[i + 1] && pn[i + 2] < pn[i])
            if (pn[i + 1] > pn[i])
                (*phase)++;
            else
                (*phase)--;
        if (*phase == 6)
            (*liters)++, *phase = 0;
        else if (*phase == -1)
            (*liters)--, *phase = 5;
    }
    int phase_fine_iter(int a, int b, int c) {
        const float step = (M_PI) / (3 * AC_STEPS);
        float       array[AC_STEPS * 3 + 1];
        int         largest_index = -AC_STEPS;
        float       largest       = 0;

        for (int i = -AC_STEPS; i <= (AC_STEPS * 2); i++) {
            float x             = (this->state.phase * M_PI / 3) + (step * i);
            float cora          = a * cos(x);
            float corb          = b * cos(x + (PI2_3));
            float corc          = c * cos(x - (PI2_3));
            float cor           = cora + corb + corc;
            array[i + AC_STEPS] = cor;
            if (cor > largest) {
                largest       = cor;
                largest_index = i;
                this->state.fine   = largest_index;
            }
        }
        return largest_index;
    }
};
