#include <iostream>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

typedef std::chrono::high_resolution_clock Clock;

#include "Deserialize.h"
#include "openfhe.h"
#include "LinTools.h"
#include "LoadModel.h"
#include "LoadImage.h"

using namespace lbcrypto;


//  Function that evaluates relu with deg=3
Ciphertext<DCRTPoly> evalRelu (Ciphertext<DCRTPoly> cipher, double xMin, double xMax);


//  String search and replace function
void myReplace(std::string& str,
               const std::string& oldStr,
               const std::string& newStr);


double time_in_seconds (std::chrono::time_point<std::chrono::system_clock> start,
                        std::chrono::time_point<std::chrono::system_clock> end);





int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "args: <textfile containing ciphertext file locations> <output json location>" << std::endl;
        return 0;
    }

    std::string outputFile = argv[2];
    std::string inputFile = argv[1];

    std::vector<std::string> files;

    std::fstream inFile(inputFile);

    std::string line;
    while (std::getline(inFile, line)) {
        files.push_back(line);
    }

    inFile.close();

    const std::string INPUT_DIR = "../ciphertexts/";
    const std::string IMG_DIR = "../img/";

    CryptoContext<DCRTPoly> context;
    KeyPair<DCRTPoly> keys;
    DeserializeContext(context, keys);


    //  Loading the convolutional layer weights and biases
    auto convWeights = loadMatrix("../model/_Conv_0_weights.csv");
    convWeights = transpose(convWeights);
    std::vector<double> convBiasesVec = loadBias("../model/_Conv_0_bias.csv");
    Plaintext convBiases = context->MakeCKKSPackedPlaintext(convBiasesVec);

    //  Loading the gemm3 weights and biases
    auto gemm1Weights = loadMatrix("../model/_Gemm_3_w.csv");
    gemm1Weights = transpose(gemm1Weights);
    std::vector<double> gemm1BiasesVec = loadBias("../model/_Gemm_3_bias.csv");
    Plaintext gemm1Biases = context->MakeCKKSPackedPlaintext(gemm1BiasesVec);

    // Loading the gemm5 weights and biases
    auto gemm2Weights = loadMatrix("../model/_Gemm_5_w.csv");
    gemm2Weights = transpose(gemm2Weights);
    std::vector<double> gemm2BiasesVec = loadBias("../model/_Gemm_5_bias.csv");
    Plaintext gemm2Biases = context->MakeCKKSPackedPlaintext(gemm2BiasesVec);


    std::vector<double> tConv;
    std::vector<double> tRelu1;
    std::vector<double> tGemm1;
    std::vector<double> tRelu2;
    std::vector<double> tGemm2;

    std::vector<int> correct;

    std::vector<std::vector<double>> plainRes;
    std::vector<std::vector<double>> cipherRes;

    for (auto file : files) {
        std::cout << "Making measurement for " << file << std::endl;

        Ciphertext<DCRTPoly> cipher;
        DeserializeCiphertext(cipher, INPUT_DIR + file);

        //  Applying convolutional layer
        auto startOper = Clock::now();
        cipher = matrix_multiplication_diagonals(convWeights, cipher);
        cipher = context->EvalAdd(cipher, convBiases);
        auto endOper = Clock::now();
        tConv.push_back(time_in_seconds(startOper, endOper));


        // Applying first instance of relu
        double xMin = -6.5318193435668945;
        double xMax = 8.548895835876465;

        startOper = Clock::now();
        cipher = evalRelu(cipher, xMin, xMax);
        endOper = Clock::now();
        tRelu1.push_back(time_in_seconds(startOper, endOper));


        // Applying first fully connected layer
        startOper = Clock::now();
        cipher = matrix_multiplication_diagonals(gemm1Weights, cipher);
        cipher = context->EvalAdd(cipher, gemm1Biases);
        endOper = Clock::now();
        tGemm1.push_back(time_in_seconds(startOper, endOper));


        // Applying second instance of relu
        xMin = -14.685586750507355;
        xMax = 12.968225657939911;

        startOper = Clock::now();
        cipher = evalRelu(cipher, xMin, xMax);
        endOper = Clock::now();
        tRelu2.push_back(time_in_seconds(startOper, endOper));


        // Applying second fully connected layer
        startOper = Clock::now();
        cipher = matrix_multiplication_diagonals(gemm2Weights, cipher);
        cipher = context->EvalAdd(cipher, gemm2Biases);
        endOper = Clock::now();
        tGemm2.push_back(time_in_seconds(startOper, endOper));

        Plaintext result;
        context->Decrypt(keys.secretKey, cipher, &result);
        result->SetLength(10);
        auto resultVec = result->GetRealPackedValue();

        auto it = std::minmax_element(resultVec.begin(), resultVec.end());
        int res = std::distance(resultVec.begin(), it.second);

        correct.push_back(std::to_string(res)[0] == file[0] ? 1 : 0);

        cipherRes.push_back(resultVec);

        myReplace(file, ".txt", ".csv");
        std::vector<double> plainVec = load_image(IMG_DIR + file);

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

        plainRes.push_back(plainVec);
    }

    json j;

    j["tConv"] = tConv;
    j["tRelu1"] = tRelu1;
    j["tGemm1"] = tGemm1;
    j["tRelu2"] = tRelu2;
    j["tGemm2"] = tGemm2;

    j["correct"] = correct;

    j["plainRes"] = plainRes;
    j["cipherRes"] = cipherRes;

    std::ofstream outFile(outputFile);

    if(!outFile.is_open()) {
        std::cout << "Could not open output file!" << std::endl;
        return -1;
    }

    outFile << j;

    outFile.close();

    return 0;
}







Ciphertext<DCRTPoly> evalRelu (Ciphertext<DCRTPoly> cipher, double xMin, double xMax) {
    auto context = cipher->GetCryptoContext();

    return context->EvalChebyshevFunction([](double x) -> double {return x < .0 ? .0 : x;}, cipher, xMin, xMax, 3);
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


double time_in_seconds (std::chrono::time_point<std::chrono::system_clock> start,
                        std::chrono::time_point<std::chrono::system_clock> end) {
    return (double) std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() * 1E-9;
}
