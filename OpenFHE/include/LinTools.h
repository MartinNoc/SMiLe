/**
 * @file LinTools.h
 *
 * @brief Includes functions that are necessary for carrying out linear operations on plaintexts in the form C++ vectors
 * and ciphertexts in the for of CKKS OpenFHE Ciphertext objects. Function bodies are defined in src/LinTools.cpp.
 *
 * @author Linus Henke
 * Contact: linus.henke@mci.edu
 *
 */

#ifndef TEST_MNIST_LINTOOLS_H
#define TEST_MNIST_LINTOOLS_H

#include <vector>

#include "MatrixFormatting.h"
#include "openfhe.h"

using namespace lbcrypto;


/**
 * Function that carries out a matrix multiplication between a plain matrix and an encrypted CKKS vector using the
 * babystep-giantstep diagonal method. The matrix will be resized to be a quadratic matrix according to the contexts
 * batchsize.
 *
 * @param matrix Plaintext matrix, which should be multiplied with the vector
 */
Ciphertext<DCRTPoly> matrix_multiplication_diagonals(std::vector<std::vector<double>> matrix, const Ciphertext<DCRTPoly>& vector);


/**
 * Function that carries out a plaintext vector-matrix multiplication. Mostly used for accuracy studies of the
 * ciphertext operations.
 *
 * @param matrix Plaintext matrix which should be multiplied with the plain vector
 * @param vector Plaintext vector which should be multiplied with the plain matrix
 */
std::vector<double> plain_matrix_multiplication(const std::vector<std::vector<double>>& matrix, const std::vector<double>& vector);


/**
 * Function that carries out addition between to plain vectors. Mostly used for accuracy studies of the ciphertext
 * operations
 *
 * @param a First vector of addition. Not passed as reference, because function needs an editable copy of one of summands
 * @param b Second vector of addition.
 */
std::vector<double> plain_addition(std::vector<double> a, const std::vector<double>& b);

#endif //TEST_MNIST_LINTOOLS_H
