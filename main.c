#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"

// ---------------- Pin and ADC Setup ----------------
#define LED_PIN GPIO_NUM_2               // On-board LED pin
#define LDR_PIN GPIO_NUM_32              // LDR connected to GPIO32
#define LDR_ADC_CHANNEL ADC1_CHANNEL_4   // ADC1_CH4 for GPIO32

#define LUX_THRESHOLD 2000               // Light alert threshold
#define AVG_WINDOW 10                    // Moving average buffer size

// Gamma and RL10 for light sensor model
const float GAMMA = 0.7;
const float RL10 = 50.0;

// ---------------- LED Blink Task: Medium Priority (2) ----------------
void blink_task(void *pvParameters) {
    while (1) {
        gpio_set_level(LED_PIN, 1);                           // LED ON
        vTaskDelay(pdMS_TO_TICKS(500));                       // Delay 500ms
        gpio_set_level(LED_PIN, 0);                           // LED OFF
        vTaskDelay(pdMS_TO_TICKS(500));                       // Delay 500ms
    }
}

// ---------------- Console Print Task: Low Priority (1) ----------------
void print_task(void *pvParameters) {
    int count = 0;
    while (1) {
        printf("\U0001F4E1 Telemetry beacon alive, count = %d\n", count++);
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second delay
    }
}

// ---------------- Sensor Task: High Priority (3) ----------------
void sensor_task(void *pvParameters) {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(LDR_ADC_CHANNEL, ADC_ATTEN_DB_11);

    int raw;
    float Vmeasure = 0.0, Rmeasure = 0.0, lux = 0.0;
    float luxreadings[AVG_WINDOW] = {0};
    int idx = 0;
    float sum = 0;

    // Pre-fill moving average buffer
    for (int i = 0; i < AVG_WINDOW; i++) {
        raw = adc1_get_raw(LDR_ADC_CHANNEL);
        Vmeasure = (raw / 4095.0) * 3.3;
        Rmeasure = ((3.3 * 10000.0) / Vmeasure) - 10000.0;  // Voltage divider
        lux = pow((RL10 * 1000.0 * pow(10, GAMMA)) / Rmeasure, 1.0 / GAMMA);  // Lux formula
        luxreadings[i] = lux;
        sum += lux;
    }

    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t periodTicks = pdMS_TO_TICKS(500);  // 500ms loop period

    while (1) {
      // WARNING: Uncommenting this will starve lower-priority tasks
/*
while (1) {
    raw = adc1_get_raw(LDR_ADC_CHANNEL);
    Vmeasure = (raw / 4095.0) * 3.3;
    Rmeasure = ((3.3 * 10000.0) / Vmeasure) - 10000.0;
    lux = pow((RL10 * 1000.0 * pow(10, GAMMA)) / Rmeasure, 1.0 / GAMMA);

    sum -= luxreadings[idx];
    luxreadings[idx] = lux;
    sum += lux;
    idx = (idx + 1) % AVG_WINDOW;

    float avg_lux = sum / AVG_WINDOW;

    printf("[⚠️ STARVING SENSOR] LUX average = %.2f\n", avg_lux);

    // vTaskDelayUntil(&lastWakeTime, periodTicks); // ← Deliberately removed
}
*/

        raw = adc1_get_raw(LDR_ADC_CHANNEL);
        Vmeasure = (raw / 4095.0) * 3.3;
        Rmeasure = ((3.3 * 10000.0) / Vmeasure) - 10000.0;
        lux = pow((RL10 * 1000.0 * pow(10, GAMMA)) / Rmeasure, 1.0 / GAMMA);

        sum -= luxreadings[idx];
        luxreadings[idx] = lux;
        sum += lux;
        idx = (idx + 1) % AVG_WINDOW;

        float avg_lux = sum / AVG_WINDOW;

        if (avg_lux > LUX_THRESHOLD) {
            printf("[\u26A0\uFE0F Sensor ALERT] LUX average = %.2f > Threshold = %d\n", avg_lux, LUX_THRESHOLD);
        } else {
            printf("[\u2705 Sensor OK] LUX average = %.2f\n", avg_lux);
        }

        // Simulate heavy computation time that could lead to missed deadlines
        vTaskDelay(pdMS_TO_TICKS(300)); // Artificial CPU load

        vTaskDelayUntil(&lastWakeTime, periodTicks);
    }
}

// ---------------- Main Entry Point ----------------
void app_main(void) {
    // GPIO setup
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);

    gpio_reset_pin(LDR_PIN);
    gpio_set_direction(LDR_PIN, GPIO_MODE_INPUT);

    // ADC setup
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(LDR_ADC_CHANNEL, ADC_ATTEN_DB_11);

    // Task creation on Core 1
    xTaskCreatePinnedToCore(print_task,  "PrintTask",  2048, NULL, 1, NULL, 1); // Low
    xTaskCreatePinnedToCore(blink_task,  "BlinkTask",  2048, NULL, 2, NULL, 1); // Medium
    xTaskCreatePinnedToCore(sensor_task, "SensorTask", 4096, NULL, 3, NULL, 1); // High
}
