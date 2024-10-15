import numpy as np
import re
import struct
from sklearn.utils import shuffle
from sklearn.model_selection import train_test_split

class NeuralNetwork:
    def __init__(self, input_size, hidden_size1, hidden_size2, output_size):
        self.input_size = input_size
        self.hidden_size1 = hidden_size1
        self.hidden_size2 = hidden_size2
        self.output_size = output_size

        self.input_weights = np.random.randn(input_size, hidden_size1).astype(np.float32) * np.sqrt(2.0/input_size)
        self.hidden_weights1 = np.random.randn(hidden_size1, hidden_size2).astype(np.float32) * np.sqrt(2.0/hidden_size1)
        self.hidden_weights2 = np.random.randn(hidden_size2, output_size).astype(np.float32) * np.sqrt(2.0/hidden_size2)
        self.hidden_bias1 = np.zeros((1, hidden_size1), dtype=np.float32)
        self.hidden_bias2 = np.zeros((1, hidden_size2), dtype=np.float32)
        self.output_bias = np.zeros((1, output_size), dtype=np.float32)

    def relu(self, x):
        return np.maximum(0, x)

    def relu_derivative(self, x):
        return np.where(x > 0, 1, 0)

    def softmax(self, x):
        exp_x = np.exp(x - np.max(x, axis=1, keepdims=True))
        return exp_x / np.sum(exp_x, axis=1, keepdims=True)

    def forward(self, X):
        self.hidden1 = self.relu(np.dot(X, self.input_weights) + self.hidden_bias1)
        self.hidden2 = self.relu(np.dot(self.hidden1, self.hidden_weights1) + self.hidden_bias2)
        self.output = self.softmax(np.dot(self.hidden2, self.hidden_weights2) + self.output_bias)
        return self.output

    def backward(self, X, y, output, learning_rate):
        output_error = y - output
        hidden2_error = np.dot(output_error, self.hidden_weights2.T)
        hidden1_error = np.dot(hidden2_error, self.hidden_weights1.T)

        hidden2_delta = hidden2_error * self.relu_derivative(self.hidden2)
        hidden1_delta = hidden1_error * self.relu_derivative(self.hidden1)

        self.hidden_weights2 += learning_rate * np.dot(self.hidden2.T, output_error)
        self.hidden_weights1 += learning_rate * np.dot(self.hidden1.T, hidden2_delta)
        self.input_weights += learning_rate * np.dot(X.T, hidden1_delta)

        self.output_bias += learning_rate * np.sum(output_error, axis=0, keepdims=True)
        self.hidden_bias2 += learning_rate * np.sum(hidden2_delta, axis=0, keepdims=True)
        self.hidden_bias1 += learning_rate * np.sum(hidden1_delta, axis=0, keepdims=True)

    def train(self, X, y, X_val, y_val, epochs, learning_rate, batch_size=32, patience=50):
        best_val_loss = float('inf')
        patience_counter = 0
        for epoch in range(epochs):
            epoch_loss = 0
            for i in range(0, len(X), batch_size):
                batch_X = X[i:i+batch_size]
                batch_y = y[i:i+batch_size]
                output = self.forward(batch_X)
                self.backward(batch_X, batch_y, output, learning_rate)
                epoch_loss += np.mean(np.square(batch_y - output))

            val_output = self.forward(X_val)
            val_loss = np.mean(np.square(y_val - val_output))

            if epoch % 100 == 0:
                print(f"Epoch {epoch}, Loss: {epoch_loss/len(X)}, Val Loss: {val_loss}")

            if val_loss < best_val_loss:
                best_val_loss = val_loss
                patience_counter = 0
            else:
                patience_counter += 1
                if patience_counter >= patience:
                    print(f"Early stopping at epoch {epoch}")
                    break

def parse_input(file_path):
    data = []
    labels = []
    with open(file_path, 'r') as file:
        for line in file:
            match = re.search(r'Red: (\d+), Green: (\d+), Blue: (\d+), Clear: (\d+), Color: (\w+)', line)
            if match:
                r, g, b, c, color = match.groups()
                data.append([int(r), int(g), int(b), int(c)])
                labels.append(color)
    return np.array(data), np.array(labels)

def normalize_data(data):
    return data / np.array([2048, 2048, 2048, 2048])

def one_hot_encode(labels):
    unique_labels = np.array(['Red', 'Black', 'Green', 'White'])
    label_dict = {label: i for i, label in enumerate(unique_labels)}
    encoded = np.zeros((len(labels), len(unique_labels)))
    for i, label in enumerate(labels):
        encoded[i, label_dict[label]] = 1
    return encoded, unique_labels

def float_to_hex(f):
    return hex(struct.unpack('<I', struct.pack('<f', f))[0])

# Prepare training data
input_data, labels = parse_input('color_data.txt')
X = normalize_data(input_data)
y, unique_labels = one_hot_encode(labels)

# Split data into train and validation sets
X_train, X_val, y_train, y_val = train_test_split(X, y, test_size=0.2, random_state=42)

# Create and train the neural network
nn = NeuralNetwork(input_size=4, hidden_size1=16, hidden_size2=8, output_size=4)
nn.train(X_train, y_train, X_val, y_val, epochs=10000, learning_rate=0.001, batch_size=32, patience=50)

# Print C-compatible array initializers
print("\nC-compatible array initializers:")
print("uint32_t input_weights[INPUT_SIZE][HIDDEN_SIZE1] = {")
for row in nn.input_weights:
    print("    {" + ", ".join(float_to_hex(x) for x in row) + "},")
print("};")

print("\nuint32_t hidden_weights1[HIDDEN_SIZE1][HIDDEN_SIZE2] = {")
for row in nn.hidden_weights1:
    print("    {" + ", ".join(float_to_hex(x) for x in row) + "},")
print("};")

print("\nuint32_t hidden_weights2[HIDDEN_SIZE2][OUTPUT_SIZE] = {")
for row in nn.hidden_weights2:
    print("    {" + ", ".join(float_to_hex(x) for x in row) + "},")
print("};")

print("\nuint32_t hidden_bias1[HIDDEN_SIZE1] = {" + ", ".join(float_to_hex(x) for x in nn.hidden_bias1.flatten()) + "};")
print("\nuint32_t hidden_bias2[HIDDEN_SIZE2] = {" + ", ".join(float_to_hex(x) for x in nn.hidden_bias2.flatten()) + "};")
print("\nuint32_t output_bias[OUTPUT_SIZE] = {" + ", ".join(float_to_hex(x) for x in nn.output_bias.flatten()) + "};")

# Debug information
print("\nSample predictions from Python:")
for i in range(min(5, len(X))):  # Print predictions for up to 5 samples
    sample_output = nn.forward(X[i:i+1])
    print(f"Input: {input_data[i]}, True label: {labels[i]}")
    print(f"Output: {sample_output[0]}")
    print(f"Predicted: {unique_labels[np.argmax(sample_output)]}\n")

print("Sample weight values:")
print("Input weights (first few):", nn.input_weights[:2, :2])
print("Hidden weights1 (first few):", nn.hidden_weights1[:2, :2])
print("Hidden weights2 (first few):", nn.hidden_weights2[:2, :2])
print("Hidden bias1 (first few):", nn.hidden_bias1[0, :2])
print("Hidden bias2 (first few):", nn.hidden_bias2[0, :2])
print("Output bias (all):", nn.output_bias[0])

# Test with a specific input
test_input = np.array([[1794, 2164, 1742, 1996]])  # Example for white
test_input_normalized = normalize_data(test_input)
test_output = nn.forward(test_input_normalized)
print("\nTest prediction:")
print("Input:", test_input[0])
print("Output:", test_output[0])
print("Predicted color:", unique_labels[np.argmax(test_output)])
