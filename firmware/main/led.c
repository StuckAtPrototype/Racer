// led.c

#include "led.h"
#include "driver/gpio.h"
#include <stdio.h>

// Shared configuration structure
led_config_t led_config = {
        .mode = LED_CONST,
        .flash_period = pdMS_TO_TICKS(500), // Default 500ms
        .led_state = {false, false, false, false}
};

static const int led_gpios[NUM_LEDS] = {LED_1_GPIO, LED_2_GPIO, LED_3_GPIO, LED_4_GPIO};

static int get_gpio_num(int led_index)
{
    if (led_index >= 0 && led_index < NUM_LEDS) {
        return led_gpios[led_index];
    }
    return -1;
}

static void led_task(void *pvParameters)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    bool toggle = false;

    while (1) {
        switch (led_config.mode) {
            case LED_CONST:
                // Apply current LED states
                for (int i = 0; i < NUM_LEDS; i++) {
                    gpio_set_level(led_gpios[i], led_config.led_state[i] ? 1 : 0);
                }
                vTaskDelay(led_config.flash_period);
                break;
            case LED_FLASH_ALL:
                for (int i = 0; i < NUM_LEDS; i++) {
                    gpio_set_level(led_gpios[i], toggle ? 1 : 0);
                }
                toggle = !toggle;
                vTaskDelayUntil(&xLastWakeTime, led_config.flash_period);
                break;
            case LED_FLASH_BACK:
                gpio_set_level(led_gpios[0], toggle ? 1 : 0);
                gpio_set_level(led_gpios[1], toggle ? 1 : 0);
                toggle = !toggle;
                vTaskDelayUntil(&xLastWakeTime, led_config.flash_period);
                break;
            case LED_FLASH_FRONT:
                gpio_set_level(led_gpios[2], toggle ? 1 : 0);
                gpio_set_level(led_gpios[3], toggle ? 1 : 0);
                toggle = !toggle;
                vTaskDelayUntil(&xLastWakeTime, led_config.flash_period);
                break;
            case LED_FLASH_FRONT_ALTERNATE:
                gpio_set_level(led_gpios[2], toggle ? 1 : 0);
                gpio_set_level(led_gpios[3], toggle ? 0 : 1);
                toggle = !toggle;
                vTaskDelayUntil(&xLastWakeTime, led_config.flash_period);
                break;
        }
    }
}

static void configure_led(int gpio)
{
    gpio_config_t io_conf = {
            .intr_type = GPIO_INTR_DISABLE,
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = (1ULL << gpio),
            .pull_down_en = 0,
            .pull_up_en = 0
    };
    gpio_config(&io_conf);
}

void led_init(void)
{
    for (int i = 0; i < NUM_LEDS; i++) {
        configure_led(led_gpios[i]);
    }

    xTaskCreate(led_task, "led_task", 2048, NULL, 10, NULL);
}

void set_led(int led_index, bool state)
{
    if (led_index < 0 || led_index >= NUM_LEDS) {
        printf("Invalid LED index\n");
        return;
    }

    led_config.led_state[led_index] = state;
    if (led_config.mode == LED_CONST) {
        gpio_set_level(led_gpios[led_index], state ? 1 : 0);
    }
    printf("LED %d (GPIO %d) set to %s\n", led_index, led_gpios[led_index], state ? "ON" : "OFF");
}

void led_set_flash_mode(led_flash mode)
{
    led_config.mode = mode;
    printf("LED flash mode set to %d\n", mode);
}

void led_set_flash_period(TickType_t period)
{
    led_config.flash_period = period;
    printf("LED flash period set to %u ticks\n", (unsigned int)period);
}

void led_all_on(void)
{
    for (int i = 0; i < NUM_LEDS; i++) {
        set_led(i, true);
    }
    led_set_flash_mode(LED_CONST);
}

void led_all_off(void)
{
    for (int i = 0; i < NUM_LEDS; i++) {
        set_led(i, false);
    }
    led_set_flash_mode(LED_CONST);
}

void led_front_on(void)
{
    set_led(0, true);
    set_led(1, true);
    led_set_flash_mode(LED_CONST);
}

void led_back_on(void)
{
    set_led(2, true);
    set_led(3, true);
    led_set_flash_mode(LED_CONST);
}
