//
// Created by lchenke on 01.03.23.
//
#include "LoadModel.h"


std::vector<std::vector<double>> loadMatrix (std::string filename) {
    std::fstream fin(filename);

    std::vector<std::vector<double>> matrix;
    std::string line, entry;

    while (getline(fin, line)) {
        std::vector<double> row;

        std::stringstream s(line);

        while (getline(s, entry, ',')) {
            row.push_back(std::stod(entry));
        }

        row.erase(row.begin());

        matrix.push_back(row);
    }

    return matrix;
}


std::vector<double> loadBias (std::string filename) {
    std::fstream fin(filename);

    std::vector<double> biases;
    std::string line, entry;

    while (getline(fin, line)) {
        std::vector<double> row;

        std::stringstream s(line);

        while (getline(s, entry, ',')) {
            row.push_back(std::stod(entry));
        }

        biases.push_back(row[1]);
    }

    return biases;
}
