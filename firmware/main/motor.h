#ifndef MOTOR_H
#define MOTOR_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/ledc.h"

#define NUM_MOTORS              2
#define MIN_SPEED_PERCENT       15  // Minimum speed percentage
#define MAX_DUTY                ((1 << LEDC_TIMER_10_BIT) - 1)  // Max duty cycle for 10-bit resolution
#define MOTOR_QUEUE_SIZE        10
#define SOFT_START_DELAY_MS     30  // Adjust this value to change the softness of the start

// Motor speed update structure
typedef struct {
    int motor_index;
    int speed_percent;
    bool direction;
} MotorUpdate;

void configure_motor_pwm(int gpio, ledc_channel_t channel);
void set_motor_speed(int motor_index, int speed_percent, bool direction);
void soft_start_motor(int motor_index, int target_speed, bool target_direction);
void motor_task(void *pvParameters);

extern QueueHandle_t motor_queue[2];
extern SemaphoreHandle_t motor_start_semaphore;

#endif // MOTOR_H
