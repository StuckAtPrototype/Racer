/**
 * @file gpio_interrupt.c
 * @brief GPIO interrupt handling implementation
 * 
 * This file implements GPIO interrupt handling for the Racer3 device, specifically
 * for triggering color sensor readings. It provides interrupt configuration,
 * debouncing, and event queue management for reliable GPIO event processing.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include "gpio_interrupt.h"
#include <stdio.h>
#include "freertos/task.h"
#include "driver/gpio.h"

// GPIO interrupt configuration
#define GPIO_QUEUE_LENGTH 10                    // Maximum number of queued events
#define GPIO_QUEUE_ITEM_SIZE sizeof(uint32_t)   // Size of each queue item
#define DEBOUNCE_TIME_MS 200                    // Debounce time in milliseconds

// Global queue for GPIO events
QueueHandle_t gpio_evt_queue = NULL;

/**
 * @brief Get the GPIO event queue handle
 * 
 * This function returns the handle to the GPIO event queue, allowing other
 * tasks to receive GPIO interrupt events.
 * 
 * @return Handle to the GPIO event queue, or NULL if not initialized
 */
QueueHandle_t gpio_interrupt_get_evt_queue(void){
    return gpio_evt_queue;
}

/**
 * @brief GPIO interrupt service routine
 * 
 * This ISR handles GPIO interrupts with debouncing to prevent spurious
 * triggers. It only processes falling edge interrupts and implements
 * a time-based debounce mechanism.
 * 
 * @param arg GPIO number passed as argument
 */
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    static uint32_t last_interrupt_time = 0;
    uint32_t interrupt_time = xTaskGetTickCountFromISR();
    uint32_t gpio_num = (uint32_t) arg;

    // Check if it's a falling edge (GPIO level is low)
    if (gpio_get_level(gpio_num) == 0) {
        // Debounce mechanism: only process if enough time has passed
        if (interrupt_time - last_interrupt_time > pdMS_TO_TICKS(DEBOUNCE_TIME_MS)) {
            xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
            last_interrupt_time = interrupt_time;
        }
    }
}

/**
 * @brief Configure GPIO interrupt for color sensor trigger
 * 
 * This function sets up the GPIO interrupt system for triggering color sensor
 * readings. It creates an event queue, configures the interrupt pin, and
 * installs the interrupt service routine with debouncing.
 */
void configure_gpio_interrupt(void)
{
    // Create a queue to handle GPIO events from ISR
    gpio_evt_queue = xQueueCreate(GPIO_QUEUE_LENGTH, GPIO_QUEUE_ITEM_SIZE);

    // Configure the GPIO pin for interrupt
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_NEGEDGE;              // Interrupt on falling edge only
    io_conf.pin_bit_mask = (1ULL << INTERRUPT_PIN);     // Bit mask of the pin
    io_conf.mode = GPIO_MODE_INPUT;                     // Set as input
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;            // Enable pull-up resistor
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;       // Disable pull-down resistor
    gpio_config(&io_conf);

    // Install GPIO ISR service
    gpio_install_isr_service(0);

    // Attach the interrupt service routine to the pin
    gpio_isr_handler_add(INTERRUPT_PIN, gpio_isr_handler, (void*) INTERRUPT_PIN);

    printf("GPIO %d configured for interrupt on falling edge only with pull-up enabled and %d ms debounce\n", INTERRUPT_PIN, DEBOUNCE_TIME_MS);
}

