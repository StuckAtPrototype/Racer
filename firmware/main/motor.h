/**
 * @file motor.h
 * @brief Motor control system header
 * 
 * This header file defines the interface for the motor control system
 * in the Racer3 device. It provides PWM-based speed and direction control
 * for two motors with soft start capabilities and queue-based command processing.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#ifndef MOTOR_H
#define MOTOR_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/ledc.h"

// Motor control configuration
#define NUM_MOTORS              2    // Number of motors in the system
#define MIN_SPEED_PERCENT       15   // Minimum speed percentage (below this motors don't move)
#define MAX_DUTY                ((1 << LEDC_TIMER_10_BIT) - 1)  // Max duty cycle for 10-bit resolution
#define MOTOR_QUEUE_SIZE        10   // Size of motor command queue
#define SOFT_START_DELAY_MS     30   // Delay between speed increments for soft start

/**
 * @brief Motor update structure
 * 
 * Defines the structure for motor speed and direction updates.
 * Used for queuing motor commands for processing by the motor task.
 */
typedef struct {
    int motor_index;        // Motor index (0 or 1)
    int speed_percent;      // Speed percentage (0-100)
    bool direction;         // Direction (false=forward, true=backward)
} MotorUpdate;

// Motor control function prototypes
/**
 * @brief Configure PWM for a motor
 * 
 * This function sets up the LEDC channel for PWM control of a motor.
 * It configures the timer, channel, and GPIO pin for motor control.
 * 
 * @param gpio GPIO pin number for the motor
 * @param channel LEDC channel to use for PWM
 */
void configure_motor_pwm(int gpio, ledc_channel_t channel);

/**
 * @brief Set motor speed and direction
 * 
 * This function sets the speed and direction for a specific motor.
 * It maps the speed percentage to PWM duty cycle and handles
 * direction control through GPIO pins.
 * 
 * @param motor_index Motor index (0 or 1)
 * @param speed_percent Speed percentage (0-100)
 * @param direction Direction (false=forward, true=backward)
 */
void set_motor_speed(int motor_index, int speed_percent, bool direction);

/**
 * @brief Soft start motor with gradual speed increase
 * 
 * This function provides smooth motor control by gradually increasing
 * the speed from 0 to the target speed. It includes direction change
 * handling and prevents sudden motor movements.
 * 
 * @param motor_index Motor index (0 or 1)
 * @param target_speed Target speed percentage (0-100)
 * @param target_direction Target direction (false=forward, true=backward)
 */
void soft_start_motor(int motor_index, int target_speed, bool target_direction);

/**
 * @brief Motor control task
 * 
 * This function runs as a FreeRTOS task to process motor commands
 * from the queue. It handles motor updates and soft start operations.
 * 
 * @param pvParameters Task parameters (unused)
 */
void motor_task(void *pvParameters);

// Global motor control resources
extern QueueHandle_t motor_queue[2];           // Motor command queues
extern SemaphoreHandle_t motor_start_semaphore; // Semaphore for motor start coordination

#endif // MOTOR_H
