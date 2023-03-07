#include <iostream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

#include "LinTools.h"
#include "LoadImage.h"
#include "LoadModel.h"

//  String search and replace function
void myReplace(std::string& str,
               const std::string& oldStr,
               const std::string& newStr);

int main(int argc, char* argv[]) {
    const std::string INPUT_DIR = "../img/";

    std::vector<std::string> files;

    std::string inputFile = argv[1];
    std::string outputFile = argv[2];

    std::ifstream inFile(inputFile);

    if (!inFile.is_open()) {
        std::cout << "Could not open input file." << std::endl;
        return -1;
    }

    std::string line;

    while (std::getline(inFile, line)) {
        myReplace(line, ".txt", ".csv");
        files.push_back(line);
    }

    inFile.close();

    std::vector<std::vector<double>> plainRes;

    //  Loading the convolutional layer weights and biases
    auto convWeights = loadMatrix("../model/_Conv_0_weights.csv");
    convWeights = transpose(convWeights);
    std::vector<double> convBiasesVec = loadBias("../model/_Conv_0_bias.csv");

    //  Loading the gemm3 weights and biases
    auto gemm1Weights = loadMatrix("../model/_Gemm_3_w.csv");
    gemm1Weights = transpose(gemm1Weights);
    std::vector<double> gemm1BiasesVec = loadBias("../model/_Gemm_3_bias.csv");

    // Loading the gemm5 weights and biases
    auto gemm2Weights = loadMatrix("../model/_Gemm_5_w.csv");
    gemm2Weights = transpose(gemm2Weights);
    std::vector<double> gemm2BiasesVec = loadBias("../model/_Gemm_5_bias.csv");

    for (auto file : files) {
        std::vector<double> plainVec = load_image(INPUT_DIR + file);

        //  Doing plain Convolution
        plainVec = plain_matrix_multiplication(convWeights, plainVec);
        plainVec = plain_addition(plainVec, convBiasesVec);

        // Doing relu1
        for (auto& elem : plainVec)
            elem = elem < .0 ? .0 : elem;

        //  Doing gemm3
        plainVec = plain_matrix_multiplication(gemm1Weights, plainVec);
        plainVec = plain_addition(plainVec, gemm1BiasesVec);

        // Doing relu2
        for (auto& elem : plainVec)
            elem = elem < .0 ? .0 : elem;

        // Doing gemm5
        plainVec = plain_matrix_multiplication(gemm2Weights, plainVec);
        plainVec = plain_addition(plainVec, gemm2BiasesVec);

        for (auto elem : plainVec)
            std::cout << elem << "\t";

        std::cout << std::endl;

        plainRes.push_back(plainVec);
    }

    json j;

    std::ifstream outFile(outputFile);

    if (!outFile.is_open()) {
        std::cout << "Could not open outputfile." << std::endl;
        return -1;
    }

    j = json::parse(outFile);

    outFile.close();

    j["plainRes"] = plainRes;

    std::ofstream writerBuffer(outputFile);

    writerBuffer << j;

    writerBuffer.close();

    return 0;
}


void myReplace(std::string& str,
               const std::string& oldStr,
               const std::string& newStr)
{
    std::string::size_type pos = 0u;
    while((pos = str.find(oldStr, pos)) != std::string::npos){
        str.replace(pos, oldStr.length(), newStr);
        pos += newStr.length();
    }
}
