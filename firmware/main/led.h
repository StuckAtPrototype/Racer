// led.h

#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define NUM_LEDS 4

#define LED_1_GPIO 27
#define LED_2_GPIO 26
#define LED_3_GPIO 25
#define LED_4_GPIO 22

// LED states
typedef enum {
    LED_CONST = 0,
    LED_FLASH_ALL,
    LED_FLASH_BACK,
    LED_FLASH_FRONT,
    LED_FLASH_FRONT_ALTERNATE
} led_flash;

typedef struct {
    led_flash mode;
    TickType_t flash_period;
    bool led_state[NUM_LEDS];
} led_config_t;

// Extern declaration of the shared configuration
extern led_config_t led_config;

// Original function prototypes
void led_init(void);
void set_led(int led_index, bool state);

// New function prototypes
void led_set_flash_mode(led_flash mode);
void led_set_flash_period(TickType_t period);
void led_all_on(void);
void led_all_off(void);
void led_front_on(void);
void led_back_on(void);

#endif // LED_H
