/**
 * @file neural_net_predictor.h
 * @brief Neural network-based color prediction system header (DEPRECATED)
 * 
 * This header file defines the interface for a neural network-based color
 * prediction system in the Racer3 device. This system is currently DEPRECATED
 * and not in active use. The current system uses tolerance-based color
 * classification instead of neural networks.
 * 
 * The neural network implements a multi-layer perceptron with:
 * - Input layer: 4 neurons (RGB + Clear)
 * - Hidden layer 1: 16 neurons with ReLU activation
 * - Hidden layer 2: 8 neurons with ReLU activation  
 * - Output layer: 4 neurons with softmax activation
 * 
 * @author StuckAtPrototype, LLC
 * @version 1.0
 * @deprecated This neural network system is no longer used, but is kept for reference and for exploration.
 */

#ifndef RACER_NEURAL_NET_PREDICTOR_H
#define RACER_NEURAL_NET_PREDICTOR_H

#include <stdint.h>

// Neural network architecture constants
#define INPUT_SIZE 4      // Input neurons (Red, Green, Blue, Clear)
#define HIDDEN_SIZE1 16   // First hidden layer neurons
#define HIDDEN_SIZE2 8    // Second hidden layer neurons
#define OUTPUT_SIZE 4     // Output neurons (color classes)

/**
 * @brief Neural network structure
 * 
 * Defines the complete neural network with weights and biases for all layers.
 * This structure contains pre-trained weights and biases for color classification.
 */
typedef struct {
    float input_weights[INPUT_SIZE][HIDDEN_SIZE1];    // Input to hidden layer 1 weights
    float hidden_weights1[HIDDEN_SIZE1][HIDDEN_SIZE2]; // Hidden layer 1 to 2 weights
    float hidden_weights2[HIDDEN_SIZE2][OUTPUT_SIZE];  // Hidden layer 2 to output weights
    float hidden_bias1[HIDDEN_SIZE1];                 // Hidden layer 1 biases
    float hidden_bias2[HIDDEN_SIZE2];                 // Hidden layer 2 biases
    float output_bias[OUTPUT_SIZE];                   // Output layer biases
} NeuralNetwork;

// Neural network function prototypes
/**
 * @brief Predict color using neural network
 * 
 * This function performs a forward pass through the neural network to predict
 * the color class based on RGB and clear channel inputs. The network uses
 * ReLU activation for hidden layers and softmax for the output layer.
 * 
 * @param nn Pointer to the neural network structure
 * @param red Normalized red value (0.0-1.0)
 * @param green Normalized green value (0.0-1.0)
 * @param blue Normalized blue value (0.0-1.0)
 * @param clear Normalized clear value (0.0-1.0)
 * @return Predicted color class index (0-3)
 */
uint32_t predict_color(NeuralNetwork* nn, float red, float green, float blue, float clear);

/**
 * @brief Initialize neural network with pre-trained weights
 * 
 * This function loads pre-trained weights and biases into the neural network
 * structure. The weights are hardcoded values that were trained offline
 * for color classification tasks.
 * 
 * @param nn Pointer to the neural network structure to initialize
 */
void initialize_neural_network(NeuralNetwork* nn);

#endif //RACER_NEURAL_NET_PREDICTOR_H
