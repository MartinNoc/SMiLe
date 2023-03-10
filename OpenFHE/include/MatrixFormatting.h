/**
 * @file MatrixFormatting.h
 *
 * @brief Functions to format plaintext matrices, so that linear operations with ciphertexts can be carried out.
 * Function bodies are defined in src/MatrixFormatting.cpp.
 *
 * @author Linus Henke
 * Contact: linus.henke@mci.edu
 *
 */

#ifndef TEST_MNIST_MATRIXFORMATTING_H
#define TEST_MNIST_MATRIXFORMATTING_H

#include <vector>
#include <math.h>
#include <cstdint>


/**
 * Function that transposes a given plaintext matrix
 *
 * @param matrix Input matrix that should be transposed.
 */
std::vector<std::vector<double>> transpose(std::vector<std::vector<double>> matrix);


/**
 * Function that finds the next larger power of two according to a number n. This is needed, because an OpenFHE
 * Cryptocontext can only encode batchsizes that are a power of two.
 *
 * @param n Unsigned integer for which a 2^k will be found so that 2^(k-1) < n <= 2^k.
 */
unsigned int next_power2 (unsigned int n);


/**
 * Function that resizes a matrix so that it is of shape numCols x numRows. The resizing will be done by padding zeros.
 *
 * @param matrix The input matrix which should be resized.
 * @param numCols The number of columns which the new matrix should have
 * @param numRows The number of rows which the new matrix should have
 */
std::vector<std::vector<double>> resizeMatrix(std::vector<std::vector<double>> matrix, size_t numCols, size_t numRows);


/**
 * Function that finds an unsigned integer factor n1 for an unsigned integer n=n1 * n2, so that n1 + n2 is minimized.
 * The return value is the factor n1 according to the convention that n1 >= n2
 *
 * @param batchSize Equivalent to the integer n in the function's description.
 */
unsigned int find_n1(uint32_t batchSize);


/**
 * Function that returns a C++ vector of the diagonals of a plaintext matrix.
 *
 * @param matrix Input matrix, from which the diagonals should be extracted
 */
std::vector<std::vector<double>> diagonal_transformation(const std::vector<std::vector<double>>& matrix);


#endif //TEST_MNIST_MATRIXFORMATTING_H
