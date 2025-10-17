/**
 * @file ring_buffer_rgb.h
 * @brief Ring buffer for RGB color data header
 * 
 * This header file defines the interface for a circular ring buffer specifically
 * designed for storing RGB color values. It provides efficient storage and retrieval
 * of color data with averaging capabilities for noise reduction in color sensor readings.
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 */

#ifndef RING_BUFFER_RGB_H
#define RING_BUFFER_RGB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Ring buffer size for color data storage
#define RING_BUFFER_SIZE 3

/**
 * @brief Ring buffer structure for RGB color data
 * 
 * This structure implements a circular buffer for storing RGB color values.
 * It maintains separate buffers for red, green, and blue components with
 * head/tail pointers for efficient circular access.
 */
typedef struct {
    uint32_t buffer_red[RING_BUFFER_SIZE];    // Red component buffer
    uint32_t buffer_green[RING_BUFFER_SIZE];  // Green component buffer
    uint32_t buffer_blue[RING_BUFFER_SIZE];   // Blue component buffer
    size_t head;                              // Write index (next position to write)
    size_t tail;                              // Read index (next position to read)
    bool full;                                // Full flag (true when buffer is full)
} ring_buffer_t;

// Ring buffer management functions
/**
 * @brief Initialize a ring buffer structure
 * 
 * Initializes the ring buffer by setting head and tail pointers to 0
 * and marking the buffer as not full.
 * 
 * @param rb Pointer to the ring buffer structure to initialize
 */
void ring_buffer_init(ring_buffer_t *rb);

/**
 * @brief Check if the ring buffer is empty
 * 
 * @param rb Pointer to the ring buffer structure
 * @return true if buffer is empty, false otherwise
 */
bool ring_buffer_is_empty(const ring_buffer_t *rb);

/**
 * @brief Add RGB color data to the ring buffer
 * 
 * Stores RGB color values at the current head position and advances
 * the head pointer. When the buffer is full, it overwrites the oldest data.
 * 
 * @param rb Pointer to the ring buffer structure
 * @param red Red component value
 * @param green Green component value
 * @param blue Blue component value
 */
void ring_buffer_put(ring_buffer_t *rb, uint32_t red, uint32_t green, uint32_t blue);

/**
 * @brief Retrieve RGB color data from the ring buffer
 * 
 * Retrieves the oldest RGB color values from the buffer and advances
 * the tail pointer.
 * 
 * @param rb Pointer to the ring buffer structure
 * @param red Pointer to store red component value
 * @param green Pointer to store green component value
 * @param blue Pointer to store blue component value
 * @return true if data was retrieved, false if buffer is empty
 */
bool ring_buffer_get(ring_buffer_t *rb, uint32_t *red, uint32_t *green, uint32_t *blue);

/**
 * @brief Calculate and retrieve average RGB values from the ring buffer
 * 
 * Calculates the average of all RGB values currently stored in the buffer.
 * This is useful for noise reduction in color sensor readings.
 * 
 * @param rb Pointer to the ring buffer structure
 * @param red Pointer to store average red component value
 * @param green Pointer to store average green component value
 * @param blue Pointer to store average blue component value
 * @return true if average was calculated, false if buffer is empty
 */
bool ring_buffer_get_avg(ring_buffer_t *rb, uint32_t *red, uint32_t *green, uint32_t *blue);

#endif // RING_BUFFER_RGB_H
