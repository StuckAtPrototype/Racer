/**
 * @file gpio_interrupt.h
 * @brief GPIO interrupt handling header
 * 
 * This header file defines the interface for GPIO interrupt handling
 * in the Racer3 device. It provides configuration and management of
 * GPIO interrupts for triggering color sensor readings with debouncing
 * and event queue management.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#ifndef GPIO_INTERRUPT_H
#define GPIO_INTERRUPT_H

#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

// GPIO interrupt configuration
#define INTERRUPT_PIN 10    // GPIO pin number for interrupt input

// GPIO interrupt function prototypes
/**
 * @brief Get the GPIO interrupt event queue handle
 * 
 * This function returns the handle to the GPIO interrupt event queue.
 * The queue is used to pass interrupt events from the ISR to the
 * interrupt processing task.
 * 
 * @return QueueHandle_t Handle to the GPIO event queue
 */
QueueHandle_t gpio_interrupt_get_evt_queue(void);

/**
 * @brief Configure GPIO interrupt system
 * 
 * This function initializes the GPIO interrupt system by creating
 * the event queue, configuring the interrupt pin, and setting up
 * the interrupt service routine with debouncing.
 */
void configure_gpio_interrupt(void);

/**
 * @brief Start the GPIO interrupt processing task
 * 
 * This function creates and starts the FreeRTOS task that processes
 * GPIO interrupt events from the queue. The task handles color sensor
 * readings triggered by interrupts.
 */
void start_gpio_interrupt_task(void);

// Global GPIO interrupt resources
extern QueueHandle_t gpio_evt_queue;    // GPIO interrupt event queue

#endif // GPIO_INTERRUPT_H
