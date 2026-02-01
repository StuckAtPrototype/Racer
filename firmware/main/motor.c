/**
 * @file motor.c
 * @brief Motor control system implementation
 * 
 * This file implements the motor control system for the Racer3 device.
 * It provides PWM-based motor control with soft start functionality,
 * speed and direction control, and motor task management.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include "motor.h"
#include <stdio.h>
#include "esp_log.h"

// Motor control system resources
QueueHandle_t motor_queue[2];        // Queues for motor control commands (one per motor)
SemaphoreHandle_t motor_start_semaphore;  // Semaphore for motor start synchronization

/**
 * @brief Configure PWM channel for motor control
 * 
 * This function configures a LEDC (LED Controller) channel for PWM-based motor control.
 * Each motor direction (forward/backward) requires a separate PWM channel.
 * 
 * @param gpio GPIO pin number for the motor control signal
 * @param channel LEDC channel to configure
 */
void configure_motor_pwm(int gpio, ledc_channel_t channel)
{
    ledc_channel_config_t ledc_channel = {
            .speed_mode     = LEDC_LOW_SPEED_MODE,  // Use low-speed mode for motor control
            .channel        = channel,              // LEDC channel number
            .timer_sel      = LEDC_TIMER_0,         // Use timer 0 (configured in main.c)
            .intr_type      = LEDC_INTR_DISABLE,    // Disable interrupts for motor control
            .gpio_num       = gpio,                 // GPIO pin for PWM output
            .duty           = 0,                    // Start with 0% duty cycle (motor off)
            .hpoint         = 0                     // No phase shift
    };
    esp_err_t err = ledc_channel_config(&ledc_channel);
    if (err != ESP_OK) {
        ESP_LOGE("motor","LEDC Channel Config failed for GPIO %d: %s", gpio, esp_err_to_name(err));
    }
}

/**
 * @brief Set motor speed and direction
 * 
 * This function sets the speed and direction of a motor using PWM control.
 * It handles speed mapping from percentage to duty cycle and ensures
 * only one direction channel is active at a time.
 * 
 * @param motor_index Motor index (0 or 1)
 * @param speed_percent Speed percentage (0-100, 0 = stop)
 * @param direction Motor direction (true = forward, false = backward)
 */
void set_motor_speed(int motor_index, int speed_percent, bool direction)
{
    // Validate motor index
    if (motor_index < 0 || motor_index >= NUM_MOTORS) {
        ESP_LOGE("motor","Invalid motor index: %d", motor_index);
        return;
    }

    // Clamp speed percentage to valid range (0-100%)
    speed_percent = (speed_percent < MIN_SPEED_PERCENT) ? 0 : (speed_percent > 100) ? 100 : speed_percent;

    // Map speed percentage to PWM duty cycle
    int duty;
    if (speed_percent == 0) {
        duty = 0;  // Motor off
    } else {
        // Map MIN_SPEED_PERCENT-100% to MIN_DUTY-MAX_DUTY range
        int min_duty = (MIN_SPEED_PERCENT * MAX_DUTY) / 100;
        duty = min_duty + ((speed_percent - MIN_SPEED_PERCENT) * (MAX_DUTY - min_duty)) / (100 - MIN_SPEED_PERCENT);
    }

    // Calculate LEDC channel numbers for forward and backward directions
    ledc_channel_t fwd_channel = (ledc_channel_t)(motor_index * 2);      // Forward channel
    ledc_channel_t bwd_channel = (ledc_channel_t)(motor_index * 2 + 1);  // Backward channel

    // Set PWM duty for both channels (only one will be active)
    ledc_set_duty(LEDC_LOW_SPEED_MODE, fwd_channel, direction ? duty : 0);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, bwd_channel, direction ? 0 : duty);
    
    // Update the PWM outputs
    ledc_update_duty(LEDC_LOW_SPEED_MODE, fwd_channel);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, bwd_channel);

    ESP_LOGD("motor","Motor %d: speed %d%% (duty %d), direction %s",
           motor_index, speed_percent, duty, direction ? "FORWARD" : "BACKWARD");
}

/**
 * @brief Soft start motor with gradual speed changes
 * 
 * This function provides smooth motor control by gradually changing speed
 * and handling direction changes safely. It prevents sudden motor movements
 * that could damage the hardware or cause instability.
 * 
 * @param motor_index Motor index (0 or 1)
 * @param target_speed Target speed percentage (0-100)
 * @param target_direction Target direction (true = forward, false = backward)
 */
void soft_start_motor(int motor_index, int target_speed, bool target_direction)
{
    // Static variables to track current motor states
    static int current_speed[NUM_MOTORS] = {0};
    static bool current_direction[NUM_MOTORS] = {true, true};

    int start_speed = current_speed[motor_index];
    bool start_direction = current_direction[motor_index];

    // Handle direction changes by first stopping the motor
    if (start_direction != target_direction && start_speed > 0) {
        ESP_LOGD("motor","Soft stopping motor %d for direction change", motor_index);
        for (int speed = start_speed; speed >= MIN_SPEED_PERCENT; speed--) {
            set_motor_speed(motor_index, speed, start_direction);
            vTaskDelay(SOFT_START_DELAY_MS / portTICK_PERIOD_MS);
        }
        set_motor_speed(motor_index, 0, start_direction);
        start_speed = 0;
    }

    // Ramp up to target speed in the correct direction
    if (target_speed > 0) {
        ESP_LOGD("motor","Soft starting motor %d to %d%%", motor_index, target_speed);
        
        // Handle initial start from stopped state
        if (start_speed == 0) {
            ESP_LOGD("motor","Starting motor %d from stopped state", motor_index);
            set_motor_speed(motor_index, MIN_SPEED_PERCENT, target_direction);
            vTaskDelay(SOFT_START_DELAY_MS * 2 / portTICK_PERIOD_MS);  // Longer delay for initial start
            start_speed = MIN_SPEED_PERCENT;
        }

        // Gradually change speed to target
        int step = (target_speed > start_speed) ? 1 : -1;
        for (int speed = start_speed; speed != target_speed; speed += step) {
            set_motor_speed(motor_index, speed, target_direction);
            vTaskDelay(SOFT_START_DELAY_MS / portTICK_PERIOD_MS);
        }
    } else {
        ESP_LOGD("motor","Stopping motor %d", motor_index);
        set_motor_speed(motor_index, 0, target_direction);
    }

    // Ensure we reach exactly the target speed
    set_motor_speed(motor_index, target_speed, target_direction);

    // Update current motor state
    current_speed[motor_index] = target_speed;
    current_direction[motor_index] = target_direction;
    ESP_LOGD("motor","Motor %d soft start completed", motor_index);
}

/**
 * @brief Motor control task
 * 
 * This task processes motor control commands from the motor queues.
 * It runs continuously, checking both motor queues for new commands
 * and applying them to the appropriate motors.
 * 
 * @param pvParameters Task parameters (unused - motor index is determined by queue)
 */
void motor_task(void *pvParameters)
{
    int motor_index = (int)pvParameters;
    MotorUpdate update;
    
    while (1) {
        // Check motor queue 0 (Motor A) for commands
        if (xQueueReceive(motor_queue[0], &update, pdMS_TO_TICKS(10)) == pdTRUE) {
            ESP_LOGD("Motor", "Motor A command: index %d, speed %d%%, direction %s", 
                     update.motor_index, update.speed_percent,
                     update.direction ? "FORWARD" : "BACKWARD");
            set_motor_speed(update.motor_index, update.speed_percent, update.direction);
        }
        
        // Check motor queue 1 (Motor B) for commands
        if (xQueueReceive(motor_queue[1], &update, pdMS_TO_TICKS(10)) == pdTRUE) {
            ESP_LOGD("Motor", "Motor B command: index %d, speed %d%%, direction %s", 
                     update.motor_index, update.speed_percent,
                     update.direction ? "FORWARD" : "BACKWARD");
            set_motor_speed(update.motor_index, update.speed_percent, update.direction);
        }
    }
}