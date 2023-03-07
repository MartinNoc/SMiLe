/**
 * @file KeyGen.h
 *
 * @brief Includes a function used to generate rotation keys
 *
 * @author Linus Henke
 * Contact: linus.henke@mci.edu
 *
 */

#ifndef TEST_MNIST_KEYGEN_H
#define TEST_MNIST_KEYGEN_H

#include <vector>
#include <set>
#include <math.h>
#include <iostream>
#include <bitset>

#include "MatrixFormatting.h"

/**
 * Function that returns an integer vector containing all required rotation indices
 * 
 * @param batchSize Batchsize of the crypto context
*/
std::vector<int> genRotations (
        uint32_t batchSize
);


#endif //TEST_MNIST_KEYGEN_H
