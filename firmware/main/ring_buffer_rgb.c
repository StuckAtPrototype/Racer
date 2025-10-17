/**
 * @file ring_buffer_rgb.c
 * @brief Ring buffer implementation for RGB color data
 * 
 * This file implements a circular ring buffer specifically designed for storing
 * RGB color values. It provides efficient storage and retrieval of color data
 * with averaging capabilities for noise reduction in color sensor readings.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#include "ring_buffer_rgb.h"
#include "esp_log.h"

/**
 * @brief Initialize a ring buffer structure
 * 
 * This function initializes a ring buffer by setting the head and tail pointers
 * to 0 and marking the buffer as not full.
 * 
 * @param rb Pointer to the ring buffer structure to initialize
 */
void ring_buffer_init(ring_buffer_t *rb) {
    rb->head = 0;      // Start at beginning of buffer
    rb->tail = 0;      // Start at beginning of buffer
    rb->full = false;  // Buffer is initially empty
}

/**
 * @brief Advance the head pointer of the ring buffer
 * 
 * This internal function advances the head pointer and manages the full state.
 * When the buffer is full, it also advances the tail pointer to maintain
 * the circular buffer behavior.
 * 
 * @param rb Pointer to the ring buffer structure
 */
static void advance_pointer(ring_buffer_t *rb) {
    if (rb->full) {
        rb->tail = (rb->tail + 1) % RING_BUFFER_SIZE;  // Advance tail when full
    }
    rb->head = (rb->head + 1) % RING_BUFFER_SIZE;      // Advance head
    rb->full = (rb->head == rb->tail);                 // Update full state
}

/**
 * @brief Check if the ring buffer is empty
 * 
 * @param rb Pointer to the ring buffer structure
 * @return true if buffer is empty, false otherwise
 */
bool ring_buffer_is_empty(const ring_buffer_t *rb) {
    return (!rb->full && (rb->head == rb->tail));
}

/**
 * @brief Add RGB color data to the ring buffer
 * 
 * This function stores RGB color values at the current head position
 * and advances the head pointer.
 * 
 * @param rb Pointer to the ring buffer structure
 * @param red Red component value
 * @param green Green component value
 * @param blue Blue component value
 */
void ring_buffer_put(ring_buffer_t *rb, uint32_t red, uint32_t green, uint32_t blue) {
    rb->buffer_red[rb->head] = red;      // Store red value
    rb->buffer_green[rb->head] = green;  // Store green value
    rb->buffer_blue[rb->head] = blue;    // Store blue value
    advance_pointer(rb);                 // Advance head pointer
}

/**
 * @brief Retrieve RGB color data from the ring buffer
 * 
 * This function retrieves the oldest RGB color values from the buffer
 * and advances the tail pointer.
 * 
 * @param rb Pointer to the ring buffer structure
 * @param red Pointer to store red component value
 * @param green Pointer to store green component value
 * @param blue Pointer to store blue component value
 * @return true if data was retrieved, false if buffer is empty
 */
bool ring_buffer_get(ring_buffer_t *rb, uint32_t *red, uint32_t *green, uint32_t *blue) {
    if (ring_buffer_is_empty(rb)) {
        return false; // Buffer empty
    }
    *red = rb->buffer_red[rb->tail];      // Retrieve red value
    *green = rb->buffer_green[rb->tail];  // Retrieve green value
    *blue = rb->buffer_blue[rb->tail];    // Retrieve blue value
    rb->full = false;                     // Mark as not full
    rb->tail = (rb->tail + 1) % RING_BUFFER_SIZE;  // Advance tail pointer
    return true;
}

/**
 * @brief Calculate and retrieve average RGB values from the ring buffer
 * 
 * This function calculates the average of all RGB values currently stored
 * in the ring buffer. This is useful for noise reduction in color sensor
 * readings by averaging multiple samples.
 * 
 * @param rb Pointer to the ring buffer structure
 * @param red Pointer to store average red component value
 * @param green Pointer to store average green component value
 * @param blue Pointer to store average blue component value
 * @return true if average was calculated, false if buffer is empty
 */
bool ring_buffer_get_avg(ring_buffer_t *rb, uint32_t *red, uint32_t *green, uint32_t *blue) {
    if (ring_buffer_is_empty(rb)) {
        return false; // Buffer empty
    }

    uint32_t avg_red = 0;
    uint32_t avg_green = 0;
    uint32_t avg_blue = 0;

    // Sum all values in the buffer
    for(int i = 0; i < RING_BUFFER_SIZE; i++){
        // Debug: Uncomment to log individual buffer values
        // ESP_LOGI("ring_buffer", "buffer[%i]: R:%lu, G:%lu, B:%lu",
        //          i, rb->buffer_red[i], rb->buffer_green[i], rb->buffer_blue[i]);

        avg_red += rb->buffer_red[i];
        avg_green += rb->buffer_green[i];
        avg_blue += rb->buffer_blue[i];
    }

    // Calculate averages
    *red = avg_red / RING_BUFFER_SIZE;
    *green = avg_green / RING_BUFFER_SIZE;
    *blue = avg_blue / RING_BUFFER_SIZE;

    return true;
}