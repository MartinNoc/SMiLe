# [OpenFHE](https://github.com/openfheorg/openfhe-development) mnist
This repository is dedicated to recreating the cryptonet neural network in OpenFHE in order to introduce fully homorphic
encryption. The project requires the following packages

## Installation
This is a cmake project, which is thus installed with
```
mkdir build && cd build
cmake ..
make 
make install
```
The binaries can then be executed from the `bin` folder in the source directory.

## Programs

### Keygen
Keygen can be used to generate the keys required for encryption/decryption and the operations required for carrying out
the neural network. The program's arguments are
```
<multiplication depth (should be set to 9 for the neural network to work)> <mod size> <first mod size> <security level>
```

### Encrypt
Encrypt can be used after the keygen program, in order to encrypt a $32\times 32$ mnist image stored in a .csv file into
an OpenFHE ciphertext. Example images can be found in the `source/img` folder. The path to the image file should be 
given as an argument to the program.

### Test Mnist
Do a test run for the neural network. The file containing the ciphertext on which inference should be made should be 
given as an argument.

### Eval Mnist
This is a C++ mainfile, which carries out inference on multiple ciphertexts in order to log execution time and 
precision of the scheme. The arguments of the program are
```
<input file> <output json file>
```
The input file should be containing the filenames of the ciphertext files which should be used for evalutaion.

### Test Matmul
This is a program dedicated to testing the matrix multiplication implemented in OpenFHE with \
$A.v = \sum_{k=0}^{n_2-1} \mathrm{Rot} \left( \sum_{j=0}^{n_1-1} \mathrm{Rot}\left(diag(A, kn_1 + j), -kn_1\right) \cdot \mathrm{Rot} (v,j), kn_1 \right)$
