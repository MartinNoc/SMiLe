/**
 * @file LinTools.h
 *
 * @brief Includes functions used to deserialize ciphertexts and encryption keys
 *
 * @author Linus Henke
 * Contact: linus.henke@mci.edu
 *
 */

#ifndef TEST_MNIST_DESERIALIZE_H
#define TEST_MNIST_DESERIALIZE_H

#include <iostream>

#include "openfhe.h"

#include "key/key-ser.h"
#include "cryptocontext-ser.h"
#include "scheme/ckksrns/ckksrns-ser.h"

using namespace lbcrypto;

/**
 * Function that deserializes a Cryptocontext and a Keypair file to an according OpenFHE object. Assumes the keys to be
 * in ../keys.
 *
 * @param context Cryptocontext object, to which the context should be deserialized
 * @param keypair Object to which the keypair should be serialized
 * @param evalKeys Specifies whether the evaluation keys should be deserialized. Default is set to true.
 */
void DeserializeContext(CryptoContext<DCRTPoly>& context, KeyPair<DCRTPoly>& keypair, bool evalKeys = true);

/**
 * Function that deserializes a ciphertext to a ciphertext object.
 *
 * @param cipher Ciphertext object to which the information should be deserialized
 * @param filename Path to the file containing the desired ciphertext
 */
void DeserializeCiphertext(Ciphertext<DCRTPoly>& cipher, std::string filename);

#endif //TEST_MNIST_DESERIALIZE_H
