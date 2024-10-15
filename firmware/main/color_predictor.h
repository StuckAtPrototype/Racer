#ifndef COLOR_PREDICTOR_H
#define COLOR_PREDICTOR_H

#include <stdint.h>

#define INPUT_SIZE 4
#define HIDDEN_SIZE1 16
#define HIDDEN_SIZE2 8
#define OUTPUT_SIZE 4

typedef struct {
    float input_weights[INPUT_SIZE][HIDDEN_SIZE1];
    float hidden_weights1[HIDDEN_SIZE1][HIDDEN_SIZE2];
    float hidden_weights2[HIDDEN_SIZE2][OUTPUT_SIZE];
    float hidden_bias1[HIDDEN_SIZE1];
    float hidden_bias2[HIDDEN_SIZE2];
    float output_bias[OUTPUT_SIZE];
} NeuralNetwork;

uint32_t predict_color(NeuralNetwork* nn, float red, float green, float blue, float clear);
void initialize_neural_network(NeuralNetwork* nn);

#endif // COLOR_PREDICTOR_H
