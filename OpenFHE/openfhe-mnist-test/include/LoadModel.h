/**
 * @file LoadModel.h
 *
 * @brief Includes functions used to get model parameters from 
 *
 * @author Linus Henke
 * Contact: linus.henke@mci.edu
 *
 */

#ifndef TEST_MNIST_LOADMODEL_H
#define TEST_MNIST_LOADMODEL_H

#include <fstream>
#include <sstream>
#include <vector>
#include <string>


/**
 * Function that loads a model matrix out of a csv file. Used for weights.
 * 
 * @param filename Path to model file
 */
std::vector<std::vector<double>> loadMatrix (std::string filename);


/**
 * Function that loads a model vector. Used for biases.
 * 
 * @param filename Path to model file
 */
std::vector<double> loadBias (std::string filename);


#endif //TEST_MNIST_LOADMODEL_H
