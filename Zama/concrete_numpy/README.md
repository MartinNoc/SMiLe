# concrete numpy

Documentation and Installation guide --> [click here](https://docs.zama.ai/concrete-numpy/stable/index.html)

## Neural Network for Linear Regression

[Community discussion](https://community.zama.ai/t/neural-network-with-concrete-numpy-precision-quantization-issue/109)

Translating a torch NN into concrete.

Network architecture:  
- Input layer size: 4
- 2 fully connected hidden layers of size 3, ReLU6 activation function
- Output layer: fully connected, size: 1 (k alone), 2 (k, d)
