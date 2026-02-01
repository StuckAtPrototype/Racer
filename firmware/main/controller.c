/**
 * @file controller.c
 * @brief Motor controller and game state management
 * 
 * This file implements the motor controller system that manages motor commands
 * and game state based on color detection. It handles different game modes
 * including speedup, slowdown, and spinout effects.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include "controller.h"
#include "motor.h"
#include <stdio.h>
#include "esp_log.h"
#include "led.h"

// Motor control state variables
static MotorCommand current_command;           // Current motor command being processed
static SemaphoreHandle_t command_mutex;        // Mutex for thread-safe command access
static TaskHandle_t controller_task_handle = NULL;  // Handle for controller task
static TimerHandle_t command_timer;            // Timer for motor command duration
static TimerHandle_t command_game_timer;       // Timer for game state duration

// Game state management
game_status state;                             // Current game state (speedup, slowdown, spinout, off)

// Cooldown mechanism to prevent rapid state changes
// Provides immunity period after hitting an obstacle (yellow detection)
static TickType_t cooldown_end = 0;

/**
 * @brief Set game status based on detected color
 * 
 * This function processes color detection results and sets the appropriate game state.
 * Different colors trigger different game effects:
 * - Yellow: Spinout effect (1 second duration with cooldown)
 * - Red/Green: Speed boost effect (10 seconds duration)
 * - Blue: Speed reduction effect (10 seconds duration)
 * 
 * @param detected_color The color detected by the color sensor
 */
void command_set_game_status(color_class_t detected_color){
    uint32_t period = 0;
    TickType_t current_time = xTaskGetTickCount();

    // Process detected color and set appropriate game state
    switch (detected_color) {
        case COLOR_YELLOW: // Spinout effect (obstacle hit)
            // Check cooldown to prevent rapid spinout triggers
            if(current_time < cooldown_end){
                ESP_LOGI("controller", "Yellow cooldown active. Ignoring spinout trigger.");
                return; // Exit early if still in cooldown period
            }
            
            // Stop any active game timer
            if (xTimerIsTimerActive(command_game_timer) == pdTRUE) {
                xTimerStop(command_game_timer, 0);
            }
            
            // Set spinout state and visual feedback
            state = GAME_SPINOUT;
            ESP_LOGI("controller", "YELLOW detected - SPINOUT mode activated");
            led_set_indicator_color(LED_COLOR_YELLOW);
            led_set_flash_mode(LED_FLASH_ALL);  // Flash all LEDs for spinout
            period = 1000; // 1 second spinout duration
            cooldown_end = current_time + pdMS_TO_TICKS(1000); // 1 second cooldown
            break;
            
        case COLOR_RED: // Speed boost effect
        case COLOR_GREEN: // Speed boost effect
            // Stop any active game timer
            if (xTimerIsTimerActive(command_game_timer) == pdTRUE) {
                xTimerStop(command_game_timer, 0);
            }
            
            // Set speed boost state and visual feedback
            state = GAME_SPEEDUP;
            led_set_indicator_color(LED_COLOR_RED);
            period = 10000; // 10 seconds speed boost duration
            ESP_LOGI("controller", "RED/GREEN detected - SPEEDUP mode activated");
            led_set_flash_mode(LED_FLASH_FRONT_ALTERNATE);  // Alternate front LEDs
            break;
            
        case COLOR_BLUE: // Speed reduction effect
            // Stop any active game timer
            if (xTimerIsTimerActive(command_game_timer) == pdTRUE) {
                xTimerStop(command_game_timer, 0);
            }
            
            // Set speed reduction state and visual feedback
            state = GAME_SLOWDOWN;
            led_set_indicator_color(LED_COLOR_BLUE);
            period = 10000; // 10 seconds speed reduction duration
            ESP_LOGI("controller", "BLUE detected - SLOWDOWN mode activated");
            led_set_flash_mode(LED_FLASH_BACK);  // Flash rear LEDs
            break;
            
        default:
            // Unknown color - no game state change
            period = 0;
            break;
    }

    // Start the game timer if a valid game state was set
    if (period != 0) {
        ESP_LOGI("controller", "Starting game timer for %d ms", period);
        xTimerChangePeriod(command_game_timer, pdMS_TO_TICKS(period), 0);
        xTimerStart(command_game_timer, 0);
    }
}

/**
 * @brief Callback function for motor command timer expiration
 * 
 * This function is called when the motor command timer expires, indicating
 * that the current motor command should be stopped. It sends stop commands
 * to both motors.
 * 
 * @param xTimer Timer handle (unused)
 */
void command_timer_callback(TimerHandle_t xTimer)
{
    ESP_LOGI("controller", "Motor command timer expired, stopping motors");

    // Create stop commands for both motors
    MotorUpdate update_a = {0, 0, 0};  // Motor A: speed=0, direction=0
    MotorUpdate update_b = {1, 0, 0};  // Motor B: speed=0, direction=0

    // Send stop commands to motor queues
    BaseType_t xStatus;
    xStatus = xQueueSend(motor_queue[0], &update_a, 0);
    if (xStatus != pdPASS) {
        ESP_LOGE("controller","Failed to send stop command for Motor A");
    }
    xStatus = xQueueSend(motor_queue[1], &update_b, 0);
    if (xStatus != pdPASS) {
        ESP_LOGE("controller","Failed to send stop command for Motor B");
    }
}

/**
 * @brief Callback function for game state timer expiration
 * 
 * This function is called when the game state timer expires, indicating
 * that the current game effect should end. It resets the game state to OFF
 * and restores normal LED behavior.
 * 
 * @param xTimer Timer handle (unused)
 */
void command_game_timer_callback(TimerHandle_t xTimer)
{
    ESP_LOGI("controller", "Game state timer expired, returning to normal mode");
    state = GAME_OFF;
    led_set_indicator_color(LED_COLOR_OFF);
    led_set_flash_mode(LED_CONST);  // Return to constant LED mode
}

// Game effect speed modifiers
#define SPEEDUP_INCREMENT 10      // Speed increase amount for speedup effect
#define SLOWDOWN_DECREMENT 30     // Speed decrease amount for slowdown effect
#define SPINOUT_SPEED 80          // Fixed speed for spinout effect

/**
 * @brief Set motor command with game state modifications
 * 
 * This function processes motor commands and applies game state modifications.
 * It also manages the motor command timer and notifies the controller task.
 * 
 * @param command The motor command to process and execute
 */
void set_motor_command(MotorCommand command)
{
    // Apply game state modifications to the motor command
    switch (state) {
        case GAME_SPINOUT: // Spinout effect - motors spin in opposite directions
            command.MotorASpeed = SPINOUT_SPEED;
            command.MotorADirection = 1;  // Motor A backward
            command.MotorBSpeed = SPINOUT_SPEED;
            command.MotorBDirection = 0;  // Motor B forward
            command.seconds = 10;  // Override duration for spinout
            break;
            
        case GAME_SPEEDUP: // Speed boost effect - increase motor speeds
            if(command.MotorASpeed < (100 - SPEEDUP_INCREMENT) && command.MotorBSpeed < (100 - SPEEDUP_INCREMENT)){
                command.MotorASpeed += SPEEDUP_INCREMENT;
                command.MotorBSpeed += SPEEDUP_INCREMENT;
            }
            break;
            
        case GAME_SLOWDOWN: // Speed reduction effect - decrease motor speeds
            if(command.MotorASpeed > SLOWDOWN_DECREMENT && command.MotorBSpeed > SLOWDOWN_DECREMENT){
                command.MotorASpeed -= SLOWDOWN_DECREMENT;
                command.MotorBSpeed -= SLOWDOWN_DECREMENT;
            }
            break;
            
        default:
            // No game state modification - use original command
            break;
    }

    // Manage motor command timer
    if (xTimerIsTimerActive(command_timer) == pdFALSE) {
        ESP_LOGD("controller", "Starting motor command timer");
        xTimerChangePeriod(command_timer, pdMS_TO_TICKS(command.seconds * 100), 0);
        xTimerStart(command_timer, 0);
    } else {
        ESP_LOGD("controller", "Resetting motor command timer");
        xTimerReset(command_timer, 0);
        xTimerStart(command_timer, 0);
    }

    // Store the current command
    current_command = command;

    // Notify the controller task to process the new command
    if (controller_task_handle != NULL) {
        xTaskNotifyGive(controller_task_handle);
    }

    ESP_LOGD("controller","Motor command set: Timer duration %lu seconds", command.seconds);
}



/**
 * @brief Controller task for processing motor commands
 * 
 * This task waits for notifications from the motor command system and processes
 * motor commands by sending them to the motor control queues. It runs in a
 * separate task to ensure responsive motor control.
 * 
 * @param pvParameters Task parameters (unused)
 */
void controller_task(void *pvParameters)
{
    while (1) {
        // Wait for notification to process a new motor command
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Get the current motor command
        MotorCommand command = current_command;

        // Create motor update structures for both motors
        MotorUpdate update_a = {0, command.MotorASpeed, command.MotorADirection};
        MotorUpdate update_b = {1, command.MotorBSpeed, command.MotorBDirection};

        // Send motor updates to the motor control queues
        BaseType_t xStatus;
        xStatus = xQueueSend(motor_queue[0], &update_a, 0);
        if (xStatus != pdPASS) {
            ESP_LOGE("controller", "Failed to send motor update for Motor A");
        }
        xStatus = xQueueSend(motor_queue[1], &update_b, 0);
        if (xStatus != pdPASS) {
            ESP_LOGE("controller","Failed to send motor update for Motor B");
        }

        // Log motor command details
        ESP_LOGD("controller","Motor A: speed %d%%, direction %s", command.MotorASpeed,
               command.MotorADirection == 0 ? "FORWARD" : "BACKWARD");
        ESP_LOGD("controller","Motor B: speed %d%%, direction %s", command.MotorBSpeed,
               command.MotorBDirection == 0 ? "FORWARD" : "BACKWARD");

        ESP_LOGD("controller","Motor commands processed and sent to motor queues");
    }
}

/**
 * @brief Initialize the motor controller system
 * 
 * This function initializes all components of the motor controller system including:
 * - Command mutex for thread safety
 * - Motor command timer for command duration management
 * - Game state timer for game effect duration management
 * - Controller task for processing motor commands
 * - Initial game state
 */
void controller_init()
{
    // Create mutex for thread-safe command access
    command_mutex = xSemaphoreCreateMutex();
    if (command_mutex == NULL) {
        ESP_LOGE("controller","Failed to create command mutex");
        return;
    }

    // Create timer for motor command duration management
    command_timer = xTimerCreate("CommandTimer", pdMS_TO_TICKS(1000), pdFALSE, (void *)0, command_timer_callback);
    if (command_timer == NULL) {
        ESP_LOGE("controller","Failed to create motor command timer");
        return;
    }

    // Create timer for game state duration management
    command_game_timer = xTimerCreate("CommandGameTimer", pdMS_TO_TICKS(1000),
                                      pdFALSE, (void *)0, command_game_timer_callback);
    if (command_game_timer == NULL) {
        ESP_LOGE("controller","Failed to create game state timer");
        return;
    }

    // Create controller task for processing motor commands
    xTaskCreate(controller_task, "controller_task", 2048, NULL, 5, &controller_task_handle);

    // Initialize game state to OFF (normal operation)
    state = GAME_OFF;
}
