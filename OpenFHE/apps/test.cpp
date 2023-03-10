#include <iostream>
#include <iomanip>
#include <chrono>
typedef std::chrono::high_resolution_clock Clock;

#include "openfhe.h"
#include "LinTools.h"
#include "random_double.h"
#include "KeyGen.h"

using namespace lbcrypto;

int main(int argc, char** argv) {
    int seed = time(NULL);
    srand((unsigned) seed);

    std::cout << std::fixed;
    std::cout << std::setprecision(2);

    uint32_t numCols = std::stoi(argv[1]);
    uint32_t batchSize = next_power2(numCols);
    uint32_t numRows = std::stoi(argv[2]);

    CCParams<CryptoContextCKKSRNS> parameters;
    parameters.SetBatchSize(batchSize);
    parameters.SetMultiplicativeDepth(1);
    parameters.SetScalingModSize(40);

    auto context = GenCryptoContext(parameters);
    context->Enable(KEYSWITCH);
    context->Enable(PKE);
    context->Enable(LEVELEDSHE);
    context->Enable(ADVANCEDSHE);

    KeyPair<DCRTPoly> keys = context->KeyGen();
    std::cout << "Keypair generated." << std::endl;

    std::cout << "Generating evaluation keys...";
    context->EvalMultKeyGen(keys.secretKey);

    std::vector<std::vector<double>> matrix;
    for(int i=0; i < (int) numRows; i++) {
        std::vector<double> row;
        for (int j=0; j<(int) numCols; j++)
            row.push_back((double) (random_double(0., 1.)));

        matrix.push_back(row);
    }

    std::vector<int> rotations = genRotations(batchSize);

    context->EvalRotateKeyGen(keys.secretKey, rotations);

    std::cout << "Done!\n\n";

    std::vector<double> plainText;
    for (uint32_t i=0; i<numCols; i++)
        plainText.push_back((double) (i + 1));

    Plaintext pl = context->MakeCKKSPackedPlaintext(plainText);
    Ciphertext<DCRTPoly> ct = context->Encrypt(pl, keys.publicKey);

    std::cout << "matrix:" << std::endl;

    for (auto row: matrix) {
        std::cout << "[\t";
        for (auto number: row) {
            std::cout << number << "\t";
        }
        std::cout << "]" << std::endl;
    }
    std::cout << std::endl;

    std::cout << "diagonals: " << std::endl;
    auto diags = diagonal_transformation(matrix);
    for (auto row: diags) {
        std::cout << "[\t";
        for (auto number: row) {
            std::cout << number << "\t";
        }
        std::cout << "]" << std::endl;
    }
    std::cout << std::endl;

    std::vector<double> plainResult;
    for (int i=0; i<(int) matrix.size(); i++) {
        double entry = .0;
        for (int j=0; j<(int) matrix[0].size(); j++) {
            entry += matrix[i][j] * plainText[j];
        }
        plainResult.push_back(entry);
    }
    std::cout << "Expected result: [\t";
    for (auto entry: plainResult) {
        std::cout << entry << "\t";
    }
    std::cout << "]" << std::endl << std::endl;

    std::cout << "Carrying out matrix multiplication...";
    auto start = Clock::now();
    Ciphertext<DCRTPoly> cipherResult = matrix_multiplication_diagonals(matrix, ct);
    auto end = Clock::now();
    double execTime = (double) std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() * 1E-9;
    std:: cout << "Done!\n\n";

    std::cout << "Result on encrypted data: ";
    Plaintext result;
    context->Decrypt(cipherResult, keys.secretKey, &result);
    result->SetLength(matrix.size());
    std::cout << result;
    std::cout << "Took " << execTime << "s" << std::endl;

    return 0;
}
