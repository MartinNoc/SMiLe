#include "Deserialize.h"

const std::string FOLDER = "../keys/";
const std::string CIPHER_FOLDER = "../ciphertexts/";

void DeserializeContext(CryptoContext<DCRTPoly>& context, KeyPair<DCRTPoly>& keyPair, bool evalKeys) {
    context->ClearEvalMultKeys();
    context->ClearEvalAutomorphismKeys();
    lbcrypto::CryptoContextFactory<lbcrypto::DCRTPoly>::ReleaseAllContexts();

    if (!Serial::DeserializeFromFile(FOLDER + "context.txt", context, SerType::BINARY)) {
        std::cerr << "Error deserializing context!" << std::endl;
        std::exit(1);
    }
    std::cout << "Context deserialized!" << std::endl;

    if (!Serial::DeserializeFromFile(FOLDER + "publicKey.txt", keyPair.publicKey, SerType::BINARY)) {
        std::cerr << "Error deserializing public key!" << std::endl;
        std::exit(1);
    }
    std::cout << "Public key deserialized!" << std::endl;

    if (!Serial::DeserializeFromFile(FOLDER + "secretKey.txt", keyPair.secretKey, SerType::BINARY)) {
        std::cerr << "Error reading serializing secret key!" << std::endl;
        std::exit(1);
    }
    std::cout << "Private key deserialized!" << std::endl;

    if (evalKeys) {
        std::ifstream multKeyIStream(FOLDER + "multKey.txt", std::ios::in | std::ios::binary);
        if (!multKeyIStream.is_open()) {
            std::cerr << "Error opening mult. key file." << std::endl;
            std::exit(1);
        }
        if (!context->DeserializeEvalMultKey(multKeyIStream, SerType::BINARY)) {
            std::cerr << "Error deserializing mult. key." << std::endl;
            std::exit(1);
        }
        std::cout << "Deserialized mult. key" << std::endl;
        multKeyIStream.close();

        std::ifstream rotKeyIStream(FOLDER + "rotKey.txt", std::ios::in | std::ios::binary);
        if (!rotKeyIStream.is_open()) {
            std::cerr << "Error opening rot. key file." << std::endl;
            std::exit(1);
        }
        if (!context->DeserializeEvalAutomorphismKey(rotKeyIStream, SerType::BINARY)) {
            std::cerr << "Error deserializing rot. key." << std::endl;
            std::exit(1);
        }
        std::cout << "Deserialized rot. key" << std::endl;
        rotKeyIStream.close();
    }
}


void DeserializeCiphertext(Ciphertext<DCRTPoly>& cipher, std::string filename) {
    if (!Serial::DeserializeFromFile(CIPHER_FOLDER + filename, cipher, SerType::BINARY)) {
        std::cerr << "Could not deserialize " + filename + " ciphertext" << std::endl;
        exit(1);
    }
    std::cout << "Ciphertext " + filename << " deserialized." << std::endl;
}
