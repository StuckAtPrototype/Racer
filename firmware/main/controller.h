// controller.h

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "led.h"

// Define the MotorCommand structure
typedef struct {
    int MotorASpeed;
    int MotorADirection;
    int MotorBSpeed;
    int MotorBDirection;
    uint32_t seconds;
} MotorCommand;

// game states
typedef enum {
    GAME_RED = 0,
    GAME_BLACK,
    GAME_GREEN,
    GAME_WHITE,
    GAME_YELLOW,
    GAME_OFF
} game_status;

void command_set_game_status(uint32_t status);
void set_motor_command(MotorCommand command);
void controller_init(void);

#endif // CONTROLLER_H
