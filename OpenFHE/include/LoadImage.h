/**
 * @file LoadImage.h
 *
 * @brief Includes a function to read mnist images from a .csv file
 *
 * @author Linus Henke
 * Contact: linus.henke@mci.edu
 *
 */

#ifndef TEST_MNIST_LOADIMAGE_H
#define TEST_MNIST_LOADIMAGE_H

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <sstream>


/**
 * Function that returns a vector of an mnist image read from a .csv file
 *
 * @param pathToFile path to csv file containing the mnist image
 */
std::vector<double> load_image(std::string pathToFile);

#endif //TEST_MNIST_LOADIMAGE_H
