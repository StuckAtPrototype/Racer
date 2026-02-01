/**
 * @file controller.h
 * @brief Motor controller and game state management header
 * 
 * This header file defines the interface for the motor controller system
 * in the Racer3 device. It manages motor commands, game state based on
 * color detection, and implements various game effects like speedup,
 * slowdown, and spinout.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "led.h"
#include "color_predictor.h"

/**
 * @brief Motor command structure
 * 
 * Defines the command structure for controlling both motors with
 * speed, direction, and duration parameters.
 */
typedef struct {
    int MotorASpeed;        // Speed for motor A (0-100)
    int MotorADirection;    // Direction for motor A (0=forward, 1=backward)
    int MotorBSpeed;        // Speed for motor B (0-100)
    int MotorBDirection;    // Direction for motor B (0=forward, 1=backward)
    uint32_t seconds;       // Duration in seconds (0=continuous)
} MotorCommand;

/**
 * @brief Game state enumeration
 * 
 * Defines the different game states that affect motor behavior
 * based on detected colors on the track.
 */
typedef enum {
    GAME_SLOWDOWN = 0,  // Slow down effect (Red/Green detection)
    GAME_SPINOUT,       // Spinout effect (Blue detection)
    GAME_SPEEDUP,       // Speed up effect (Yellow detection)
    GAME_OFF            // Normal operation (no special effects)
} game_status;

// Controller function prototypes
/**
 * @brief Set game state based on detected color
 * 
 * This function analyzes the detected color and sets the appropriate
 * game state that will modify motor behavior. Different colors trigger
 * different effects:
 * - Yellow: Speed up effect
 * - Red/Green: Slow down effect  
 * - Blue: Spinout effect
 * 
 * @param detected_color The color detected by the color sensor
 */
void command_set_game_status(color_class_t detected_color);

/**
 * @brief Set motor command with game state modifications
 * 
 * This function applies the current game state modifications to the
 * motor command and manages the command timer for automatic stopping.
 * Game effects modify the motor speeds and directions as appropriate.
 * 
 * @param command The base motor command to apply
 */
void set_motor_command(MotorCommand command);

/**
 * @brief Initialize the controller system
 * 
 * This function initializes the controller task, creates necessary
 * mutexes, timers, and sets up the command processing system.
 */
void controller_init(void);

#endif // CONTROLLER_H
