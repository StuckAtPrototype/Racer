#include "gpio_interrupt.h"
#include <stdio.h>
#include "freertos/task.h"
#include "driver/gpio.h"

#define GPIO_QUEUE_LENGTH 10
#define GPIO_QUEUE_ITEM_SIZE sizeof(uint32_t)
#define DEBOUNCE_TIME_MS 200

QueueHandle_t gpio_evt_queue = NULL;

QueueHandle_t gpio_interrupt_get_evt_queue(void){
    return gpio_evt_queue;
}

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    static uint32_t last_interrupt_time = 0;
    uint32_t interrupt_time = xTaskGetTickCountFromISR();
    uint32_t gpio_num = (uint32_t) arg;

    // Check if it's a falling edge
    if (gpio_get_level(gpio_num) == 0) {
        // Debounce mechanism
        if (interrupt_time - last_interrupt_time > pdMS_TO_TICKS(DEBOUNCE_TIME_MS)) {
            xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
            last_interrupt_time = interrupt_time;
        }
    }
}

void configure_gpio_interrupt(void)
{
    // Create a queue to handle GPIO event from ISR
    gpio_evt_queue = xQueueCreate(GPIO_QUEUE_LENGTH, GPIO_QUEUE_ITEM_SIZE);

    // Configure the GPIO pin
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;  // Interrupt on falling edge only
    io_conf.pin_bit_mask = (1ULL << INTERRUPT_PIN);  // Bit mask of the pin
    io_conf.mode = GPIO_MODE_INPUT;  // Set as input
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;  // Enable pull-up resistor
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;  // Disable pull-down resistor
    gpio_config(&io_conf);

    // Install GPIO ISR service
    gpio_install_isr_service(0);

    // Attach the interrupt service routine
    gpio_isr_handler_add(INTERRUPT_PIN, gpio_isr_handler, (void*) INTERRUPT_PIN);

    printf("GPIO %d configured for interrupt on falling edge only with pull-up enabled and %d ms debounce\n", INTERRUPT_PIN, DEBOUNCE_TIME_MS);
}

