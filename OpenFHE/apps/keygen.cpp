#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <math.h>

#include "KeyGen.h"

#include "openfhe.h"

#include "cryptocontext-ser.h"
#include "key/key-ser.h"
#include "scheme/ckksrns/ckksrns-ser.h"

using namespace lbcrypto;

const std::string FOLDER = "../keys/";

int main (int argc, char** argv) {

    if (argc != 5) {
        std::cout << "args: <mult depth> <mod size> <first mod size> <security level>" << std::endl;
        return 0;
    }

    uint32_t multDepth = std::stoi(argv[1]);
    uint32_t scalSize = std::stoi(argv[2]);
    uint32_t firstModSize = std::stoi(argv[3]);
    uint32_t securityLevel = std::stoi(argv[4]);
    uint32_t batchSize = 1024;

    CCParams<CryptoContextCKKSRNS> parameters;
    parameters.SetMultiplicativeDepth(multDepth);
    parameters.SetScalingModSize(scalSize);
    parameters.SetFirstModSize(firstModSize);
    parameters.SetRingDim(8192);

    switch (securityLevel) {
        case 128:
            parameters.SetSecurityLevel(HEStd_128_classic);
            break;

        case 256:
            parameters.SetSecurityLevel(HEStd_256_classic);
            break;

        case 192:
            parameters.SetSecurityLevel(HEStd_192_classic);
            break;

        default:
            parameters.SetSecurityLevel(HEStd_NotSet);
    }

    parameters.SetBatchSize(batchSize);

    CryptoContext<DCRTPoly> context = GenCryptoContext(parameters);

    context->Enable(PKE);
    context->Enable(KEYSWITCH);
    context->Enable(LEVELEDSHE);
    context->Enable(ADVANCEDSHE);

    std::cout << "The Cryptocontext has been generated..." << std::endl;

    KeyPair<DCRTPoly> keys = context->KeyGen();
    std::cout << "Keypair has been generated..." << std::endl;

    context->EvalMultKeyGen(keys.secretKey);
    std::cout << "Eval. mult. keys generated" << std::endl;

    std::vector<int> rotations = genRotations(batchSize);

    std::cout << "Generating rotation keys..." << std::endl;
    context->EvalRotateKeyGen(keys.secretKey, rotations);
    std::cout << "Done!" << std::endl;

    if (!Serial::SerializeToFile(FOLDER + "context.txt", context, SerType::BINARY)) {
        std::cerr << "Error serializing context." << std::endl;
        std::exit(1);
    }
    std::cout << "Cryptocontext serialized!" << std::endl;

    if (!Serial::SerializeToFile(FOLDER + "publicKey.txt", keys.publicKey, SerType::BINARY)) {
        std::cerr << "Error serializing public key." << std::endl;
        std::exit(1);
    }
    std::cout << "Public key serialized!" << std::endl;

    if (!Serial::SerializeToFile(FOLDER + "secretKey.txt", keys.secretKey, SerType::BINARY)) {
        std::cerr << "Error serializing public key." << std::endl;
        std::exit(1);
    }
    std::cout << "Secret key serialized!" << std::endl;

    std::ofstream multKeyFile(FOLDER + "multKey.txt", std::ios::out | std::ios::binary);
    if (multKeyFile.is_open()) {
        if (!context->SerializeEvalMultKey(multKeyFile, SerType::BINARY)) {
            std::cerr << "Error serializing multiplication key." << std::endl;
            std::exit(1);
        }
        std::cout << "Multiplication key serialized!" << std::endl;
        multKeyFile.close();

    } else {
        std::cerr << "Error opening Mult Key file..." << std::endl;
    }

    std::ofstream rotKeyFile(FOLDER + "rotKey.txt", std::ios::out | std::ios::binary);
    if (rotKeyFile.is_open()) {
        if (!context->SerializeEvalAutomorphismKey(rotKeyFile, SerType::BINARY)) {
            std::cerr << "Error serializing rotation key." << std::endl;
            std::exit(1);
        }
        std::cout << "Rotation key serialized!" << std::endl;
        rotKeyFile.close();
    } else {
        std::cerr << "Error opening Mult Key file..." << std::endl;
    }

    return 0;
}
