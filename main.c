#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"

// ---------------- Pin and ADC Setup ----------------
#define LED_PIN GPIO_NUM_2
#define LDR_PIN GPIO_NUM_32
#define LDR_ADC_CHANNEL ADC1_CHANNEL_4

#define ADC_MAX 4095.0
#define V_SOURCE 3.3
#define R_FIXED 10000.0
#define GAMMA 0.7
#define RL10 50.0

#define LUX_MIN_THRESHOLD 500
#define LUX_MAX_THRESHOLD 2000
#define AVG_WINDOW 10

void blink_task(void *pvParameters) {
    while (1) {
        gpio_set_level(LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void print_task(void *pvParameters) {
    int count = 0;
    while (1) {
        printf("📡 Telemetry beacon alive, count = %d\n", count++);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void sensor_task(void *pvParameters) {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(LDR_ADC_CHANNEL, ADC_ATTEN_DB_12);  // Use DB_12 to avoid deprecation

    int raw;
    float Vmeasure = 0.0, Rmeasure = 0.0, lux = 0.0;
    float luxreadings[AVG_WINDOW] = {0};
    int idx = 0;
    float sum = 0;

    for (int i = 0; i < AVG_WINDOW; i++) {
        raw = adc1_get_raw(LDR_ADC_CHANNEL);
        Vmeasure = (raw / ADC_MAX) * V_SOURCE;
        Rmeasure = (Vmeasure / (V_SOURCE - Vmeasure)) * R_FIXED;
        lux = pow((RL10 * 1000.0 * pow(10.0 / Rmeasure, GAMMA)), 1.0 / GAMMA);
        luxreadings[i] = lux;
        sum += lux;
    }

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t periodTicks = pdMS_TO_TICKS(500);

    while (1) {
        raw = adc1_get_raw(LDR_ADC_CHANNEL);
        Vmeasure = (raw / ADC_MAX) * V_SOURCE;

        if (Vmeasure < V_SOURCE) {
            Rmeasure = (Vmeasure / (V_SOURCE - Vmeasure)) * R_FIXED;
        } else {
            Rmeasure = 1e6;
        }

        lux = pow((RL10 * 1000.0 * pow(10.0 / Rmeasure, GAMMA)), 1.0 / GAMMA);

        sum -= luxreadings[idx];
        luxreadings[idx] = lux;
        sum += lux;
        idx = (idx + 1) % AVG_WINDOW;

        float avg_lux = sum / AVG_WINDOW;

        if (avg_lux < LUX_MIN_THRESHOLD || avg_lux > LUX_MAX_THRESHOLD) {
            printf("[⚠️ Sensor ALERT] LUX average = %.2f\n", avg_lux);
        } else {
            printf("[✅ Sensor OK] LUX average = %.2f\n", avg_lux);
        }

        vTaskDelayUntil(&lastWakeTime, periodTicks);
    }
}

void app_main(void) {
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(LDR_PIN);
    gpio_set_direction(LDR_PIN, GPIO_MODE_INPUT);

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(LDR_ADC_CHANNEL, ADC_ATTEN_DB_12);  // Use DB_12 to avoid warning

    xTaskCreatePinnedToCore(print_task,  "PrintTask",  2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(blink_task,  "BlinkTask",  2048, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(sensor_task, "SensorTask", 4096, NULL, 3, NULL, 1);
}
