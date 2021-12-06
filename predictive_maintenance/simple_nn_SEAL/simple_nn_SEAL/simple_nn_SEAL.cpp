#include <iostream>
#include <vector>
#include <math.h>
#include <seal/seal.h>
#include <regex>
#include <string>
#include <iostream>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

using namespace std;
using namespace seal;

vector<double> read_csv(string filename, int nCols = 4, int ignoreRows = 2)
{
    vector<double> result;

    ifstream csvFile(filename);

    if (!csvFile.is_open()) throw runtime_error("Could not open csv File!");

    string line, colname;
    double val;
    string colval;

    if (csvFile.good())
    {
        // ignore first rows
        for (int i = 0; i < ignoreRows; ++i)
        {
            getline(csvFile, line);
        }
    }

    while (getline(csvFile, line))
    {
        stringstream ss(line);
        getline(ss, colval, ',');
        getline(ss, colval, ',');
        result.push_back(stod(colval));
    }

    csvFile.close();

    return result;
}

bool sortBySuffixId(string first, string second) {
    smatch m1, m2;
    regex_search(first, m1, regex("_([0-9]+)\.csv"));
    regex_search(second, m2, regex("_([0-9]+)\.csv"));

    return stoi(m1[1]) < stoi(m2[1]);
}

int main()
{
    EncryptionParameters parms(scheme_type::ckks);
    size_t poly_modulus_degree = 4096;

    int bits_scale = 25;

    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, { 39, bits_scale, 39 }));
    double scale = pow(2.0, bits_scale);

    SEALContext context(parms);

    // Data files
    vector<string> filenames;
    string path = "../../sensitive_data/machine_data/Erstdaten";
    for (const auto& entry : fs::directory_iterator(path)) {
        filenames.push_back(entry.path().string());
    }
    sort(filenames.begin(), filenames.end(), sortBySuffixId);

    vector<vector<double>> data;

    for (auto file : filenames) {
        data.push_back(read_csv(file, 1, 1));
    }

    // Key Generator
    KeyGenerator keygen(context);
    auto sk = keygen.secret_key();
    PublicKey pk;
    keygen.create_public_key(pk);
    RelinKeys relin_keys;
    keygen.create_relin_keys(relin_keys);
    GaloisKeys galk;
    keygen.create_galois_keys(galk);

    // Encode and Encrypt
    CKKSEncoder encoder(context);
    Encryptor encryptor(context, pk);

    vector<Ciphertext> data_ct;

    for (auto col : data) {
        Plaintext pt;
        encoder.encode(col, scale, pt);
        Ciphertext ct;
        encryptor.encrypt(pt, ct);

        data_ct.push_back(ct);
    }

    // Evaluate
    Evaluator evaluator(context);
    for (int i = 0; i < data_ct.size() - 1; i++) {
        evaluator.sub_inplace(data_ct[i], data_ct[i + 1]);


        // rotate and add for vector sum
        for (int j = 1; j <= encoder.slot_count() / 2; j <<= 1) {
            Ciphertext temp_ct;
            evaluator.rotate_vector(data_ct[i], j, galk, temp_ct);
            evaluator.add_inplace(data_ct[i], temp_ct);
        }
    }

    // Decrypt
    Decryptor decryptor(context, sk);
    vector<double> result_vec;

    for (auto ct : data_ct) {
        Plaintext pt_result;
        decryptor.decrypt(ct, pt_result);
        vector<double> result;
        encoder.decode(pt_result, result);

        result_vec.push_back(result[0]);
    }

    return 0;
}