#define _USE_MATH_DEFINES

#include <iostream>
#include <seal/seal.h>

#include <fstream>
#include <utility>
#include <vector>
#include <sstream>
#include <functional> 
#include <math.h>

#include "helpers.h"

#include "matplotlibcpp.h"

namespace plt = matplotlibcpp;

using namespace std;
using namespace seal;


vector<vector<double>> read_csv(string filename, int nCols = 4, int ignoreRows = 2)
{
    vector<vector<double>> result;

    ifstream csvFile(filename);

    if (!csvFile.is_open()) throw runtime_error("Could not open csv File!");

    string line, colname;
    double val;

    if (csvFile.good())
    {
        // ignore first rows
        for (int i = 0; i < ignoreRows; ++i)
        {
            getline(csvFile, line);
        }
    }

    // initialize cols
    for (int i = 0; i < nCols; i++)
    {
        result.push_back(vector<double> {});
    }

    while (getline(csvFile, line))
    {
        stringstream ss(line);
        int colIndex = 0;

        while (ss >> val)
        {
            result[colIndex].push_back(val);

            if (ss.peek() == ',')
            {
                ss.ignore();
            }

            colIndex++;
        }
    }

    csvFile.close();

    return result;
}

void context_infos(SEALContext context) {
    if (context.parameters_set()) {
        cout << "Valid context!" << endl;
    }
    else {
        cout << "Invalid context!" << endl;
        cout << context.parameter_error_name() << endl;
        cout << context.parameter_error_message() << endl;
        return;
    }

    EncryptionParameterQualifiers quali = context.key_context_data()->qualifiers();
    cout << "Batching: " << quali.using_batching << endl;
    cout << "FFT: " << quali.using_fft << endl;
    cout << "NTT: " << quali.using_ntt << endl;
}

void realDotProduct(SEALContext context) {
    int N = 10;
    
    vector<double> inputs;
    for (int i = 0; i < N; i++) {
        inputs.push_back(i);
    }

    double scale = pow(2.0, 30);

    CKKSEncoder encoder(context);
    Plaintext pt;
    encoder.encode(inputs, scale, pt);

    // Set up keys
    KeyGenerator keygen(context);
    auto sk = keygen.secret_key();
    PublicKey pk;
    keygen.create_public_key(pk);
    RelinKeys relin_keys;
    keygen.create_relin_keys(relin_keys);
    GaloisKeys galk;
    keygen.create_galois_keys(galk);

    Encryptor encryptor(context, pk);
    Ciphertext ct;
    encryptor.encrypt(pt, ct);

    // Server
    Plaintext weight_pt;
    encoder.encode(inputs, scale, weight_pt);

    Evaluator evaluator(context);
    evaluator.multiply_plain_inplace(ct, weight_pt);
    evaluator.rescale_to_next_inplace(ct);

    for (size_t i = 1; i <= encoder.slot_count() / 2; i <<= 1) {
        Ciphertext temp_ct;
        evaluator.rotate_vector(ct, i, galk, temp_ct);
        evaluator.add_inplace(ct, temp_ct);
    }

    Decryptor decryptor(context, sk);
    Plaintext pt_result;
    decryptor.decrypt(ct, pt_result);

    vector<double> vec_result;
    encoder.decode(pt_result, vec_result);
    cout << "Result: " << vec_result[0] << endl;
}

void complexDotProduct(SEALContext context) {
    int N = 10;

    vector<complex<double>> inputs;
    vector<double> weights;
    for (int i = 0; i < N; i++) {
        inputs.push_back(exp(1i * (M_PI * 2.0 / N)));
        weights.push_back((i & 1) ? 1.0 : 0.0);
    }

    double scale = pow(2.0, 30);

    CKKSEncoder encoder(context);
    Plaintext pt;
    encoder.encode(inputs, scale, pt);

    // Set up keys
    KeyGenerator keygen(context);
    auto sk = keygen.secret_key();
    PublicKey pk;
    keygen.create_public_key(pk);
    RelinKeys relin_keys;
    keygen.create_relin_keys(relin_keys);
    GaloisKeys galk;
    keygen.create_galois_keys(galk);

    Encryptor encryptor(context, pk);
    Ciphertext ct;
    encryptor.encrypt(pt, ct);

    // Server
    Plaintext weight_pt;
    encoder.encode(weights, scale, weight_pt);

    Evaluator evaluator(context);
    evaluator.multiply_plain_inplace(ct, weight_pt);
    evaluator.rescale_to_next_inplace(ct);

    Decryptor decryptor(context, sk);

    Plaintext intRes;
    decryptor.decrypt(ct, intRes);
    vector<complex<double>> res;
    encoder.decode(intRes, res);
    for (int i = 0; i < N; i++) {
        cout << i << ": " << res[i] << endl;
    }

    for (size_t i = 1; i <= encoder.slot_count() / 2; i <<= 1) {
        Ciphertext temp_ct;
        evaluator.rotate_vector(ct, i, galk, temp_ct);
        evaluator.add_inplace(ct, temp_ct);
    }
    
    Plaintext pt_result;
    decryptor.decrypt(ct, pt_result);

    vector<complex<double>> vec_result;
    encoder.decode(pt_result, vec_result);
    cout << "Result: " << vec_result[0] << endl;
}

vector<double> secure_dft(SEALContext context, vector<double> data) {
    chrono::high_resolution_clock::time_point time_start, time_end;
    chrono::microseconds time_diff;

    int N = data.size();
    cout << "datapoints: " << N << endl;

    int no_dfs = 50;
    cout << "no_dfs: " << no_dfs << endl;

    double scale = pow(2.0, 30);
    CKKSEncoder encoder(context);

    Plaintext pt;
    encoder.encode(data, scale, pt);

    // Set up keys
    KeyGenerator keygen(context);
    auto sk = keygen.secret_key();
    PublicKey pk;
    keygen.create_public_key(pk);
    RelinKeys relin_keys;
    keygen.create_relin_keys(relin_keys);
    GaloisKeys galk;
    keygen.create_galois_keys(galk);

    Encryptor encryptor(context, pk);
    Ciphertext ct;
    encryptor.encrypt(pt, ct);

    Evaluator evaluator(context);
    Decryptor decryptor(context, sk);

    vector<double> millis;
    Plaintext div1e3;
    for (int i = 0; i < N; i++) {
        millis.push_back(0.001);
    }
    encoder.encode(millis, scale, div1e3);

    vector<double> frequencies;

    for (int f = 0; f < no_dfs; f++) {
        vector<complex<double>> w;
        for (int j = 0; j < N; j++) {
            w.push_back(exp(1i * (M_PI * j * f * 2.0 / N)));
        }

        Plaintext w_pt;
        encoder.encode(w, scale, w_pt);
        
        // multiply and rescale
        Ciphertext result;
        evaluator.multiply_plain(ct, w_pt, result);
        evaluator.relinearize_inplace(ct, relin_keys);
        evaluator.rescale_to_next_inplace(result);
        
        // rotate and add for vector sum
        for (int i = 1; i <= encoder.slot_count() / 2; i <<= 1) {
            Ciphertext temp_ct;
            evaluator.rotate_vector(result, i, galk, temp_ct);
            evaluator.add_inplace(result, temp_ct);
        }

        /*
        // scale down
        cout << "scale result: " << log2(result.scale()) << endl;
        cout << "scale factor: " << log2(div1e3.scale()) << endl;
        evaluator.multiply_plain_inplace(result, div1e3);
        evaluator.rescale_to_next_inplace(result);
        */

        // multiply with complex conjugate
        Ciphertext result_conj;
        evaluator.complex_conjugate(result, galk, result_conj);
        evaluator.multiply_inplace(result, result_conj);
        evaluator.rescale_to_next_inplace(result);
        
        // decrypt
        Plaintext pt_result;
        decryptor.decrypt(result, pt_result);

        // decode
        vector<double> vec_result;
        encoder.decode(pt_result, vec_result);

        // std::printf("%2d: %10.3f\n", f, vec_result[0]);

        // put result into return vector
        frequencies.push_back(vec_result[0]);
    }

    return frequencies;
}

vector<double> secure_dft_timing(SEALContext context, vector<double> data) {

    chrono::high_resolution_clock::time_point time_start, time_end;
    chrono::microseconds time_multiply_plain_sum(0);
    chrono::microseconds time_relinearize_sum(0);
    chrono::microseconds time_rescale_sum(0);
    chrono::microseconds time_rescale_2_sum(0);
    chrono::microseconds time_complex_conjugate_sum(0);
    chrono::microseconds time_multiply_sum(0);
    chrono::microseconds time_decrypt_sum(0);
    chrono::microseconds time_decode_sum(0);
    chrono::microseconds time_encode_sum(0);
    chrono::microseconds time_rotate_sum(0);
    chrono::microseconds time_add_sum(0);

    int N = data.size();
    cout << "datapoints: " << N << endl;

    int no_dfs = 50;
    cout << "averaging: " << no_dfs << endl;

    double scale = pow(2.0, 30);
    CKKSEncoder encoder(context);
    Plaintext pt;
    encoder.encode(data, scale, pt);

    // Set up keys
    KeyGenerator keygen(context);
    auto sk = keygen.secret_key();
    PublicKey pk;
    keygen.create_public_key(pk);
    RelinKeys relin_keys;
    keygen.create_relin_keys(relin_keys);
    GaloisKeys galk;
    keygen.create_galois_keys(galk);

    Encryptor encryptor(context, pk);
    Ciphertext ct;
    encryptor.encrypt(pt, ct);

    Evaluator evaluator(context);
    Decryptor decryptor(context, sk);

    vector<double> millis;
    Plaintext div1e3;
    for (int i = 0; i < N; i++) {
        millis.push_back(0.001);
    }
    encoder.encode(millis, scale, div1e3);

    complex<double> Ninv = 1 / N;

    vector<double> frequencies;

    for (int f = 0; f < no_dfs; f++) {
        vector<complex<double>> w;
        for (int j = 0; j < N; j++) {
            w.push_back(exp(1i * (M_PI * j * f * 2.0 / N)));
        }

        Plaintext w_pt;
        time_start = chrono::high_resolution_clock::now();
        encoder.encode(w, scale, w_pt);
        time_end = chrono::high_resolution_clock::now();
        time_encode_sum += chrono::duration_cast<chrono::microseconds>(time_end - time_start);
        
        // multiply and rescale
        Ciphertext result;
        time_start = chrono::high_resolution_clock::now();
        evaluator.multiply_plain(ct, w_pt, result);
        time_end = chrono::high_resolution_clock::now();
        time_multiply_plain_sum += chrono::duration_cast<chrono::microseconds>(time_end - time_start);

        time_start = chrono::high_resolution_clock::now();
        evaluator.relinearize_inplace(ct, relin_keys);
        time_end = chrono::high_resolution_clock::now();
        time_relinearize_sum += chrono::duration_cast<chrono::microseconds>(time_end - time_start);

        time_start = chrono::high_resolution_clock::now();
        evaluator.rescale_to_next_inplace(result);
        time_end = chrono::high_resolution_clock::now();
        time_rescale_sum += chrono::duration_cast<chrono::microseconds>(time_end - time_start);

        // rotate and add for vector sum
        for (int i = 1; i <= encoder.slot_count() / 2; i <<= 1) {
            Ciphertext temp_ct;
            time_start = chrono::high_resolution_clock::now();
            evaluator.rotate_vector(result, i, galk, temp_ct);
            time_end = chrono::high_resolution_clock::now();
            time_rotate_sum += chrono::duration_cast<chrono::microseconds>(time_end - time_start);

            time_start = chrono::high_resolution_clock::now();
            evaluator.add_inplace(result, temp_ct);
            time_end = chrono::high_resolution_clock::now();
            time_add_sum += chrono::duration_cast<chrono::microseconds>(time_end - time_start);
        }

        /*
        // scale down
        cout << "scale result: " << log2(result.scale()) << endl;
        cout << "scale factor: " << log2(div1e3.scale()) << endl;
        evaluator.multiply_plain_inplace(result, div1e3);
        evaluator.rescale_to_next_inplace(result);
        */

        // multiply with complex conjugate
        Ciphertext result_conj;

        time_start = chrono::high_resolution_clock::now();
        evaluator.complex_conjugate(result, galk, result_conj);
        time_end = chrono::high_resolution_clock::now();
        time_complex_conjugate_sum += chrono::duration_cast<chrono::microseconds>(time_end - time_start);

        time_start = chrono::high_resolution_clock::now();
        evaluator.multiply_inplace(result, result_conj);
        time_end = chrono::high_resolution_clock::now();
        time_multiply_sum += chrono::duration_cast<chrono::microseconds>(time_end - time_start);
        
        time_start = chrono::high_resolution_clock::now();
        evaluator.rescale_to_next_inplace(result);
        time_end = chrono::high_resolution_clock::now();
        time_rescale_2_sum += chrono::duration_cast<chrono::microseconds>(time_end - time_start);

        // decrypt
        Plaintext pt_result;
        time_start = chrono::high_resolution_clock::now();
        decryptor.decrypt(result, pt_result);
        time_end = chrono::high_resolution_clock::now();
        time_decrypt_sum += chrono::duration_cast<chrono::microseconds>(time_end - time_start);

        // decode
        vector<double> vec_result;
        time_start = chrono::high_resolution_clock::now();
        encoder.decode(pt_result, vec_result);
        time_end = chrono::high_resolution_clock::now();
        time_decode_sum += chrono::duration_cast<chrono::microseconds>(time_end - time_start);

        // std::printf("%2d: %10.3f\n", f, vec_result[0]);

        // put result into return vector
        frequencies.push_back(vec_result[0]);
    }

    auto avg_encode = time_encode_sum.count() / no_dfs;
    auto avg_multiply_plain = time_multiply_plain_sum.count() / no_dfs;
    auto avg_relinearize = time_relinearize_sum.count() / no_dfs;
    auto avg_rescale = time_rescale_sum.count() / no_dfs;
    auto avg_rotate = time_rotate_sum.count() / (no_dfs * log2(encoder.slot_count() / 2) + 1);
    auto avg_add = time_add_sum.count() / (no_dfs * log2(encoder.slot_count() / 2) + 1);
    auto avg_conjugate_complex = time_complex_conjugate_sum.count() / no_dfs;
    auto avg_multiply = time_multiply_sum.count() / no_dfs;
    auto avg_rescale_2 = time_rescale_2_sum.count() / no_dfs;
    auto avg_decrypt = time_decrypt_sum.count() / no_dfs;
    auto avg_decode = time_decode_sum.count() / no_dfs;

    cout << "Average encode (weights vector): " << avg_encode << " microseconds" << endl;
    cout << "Average multiply plain: " << avg_multiply_plain << " microseconds" << endl;
    cout << "Average relinearize: " << avg_relinearize << " microseconds" << endl;
    cout << "Average rescale: " << avg_rescale << " microseconds" << endl;
    cout << "Average rotate: " << avg_rotate << " microseconds" << endl;
    cout << "Average add: " << avg_add << " microseconds" << endl;
    cout << "Average conjugate complex: " << avg_conjugate_complex << " microseconds" << endl;
    cout << "Average multiply: " << avg_multiply << " microseconds" << endl;
    cout << "Average rescale (2): " << avg_rescale_2 << " microseconds" << endl;
    cout << "Average decrypt: " << avg_decrypt << " microseconds" << endl;
    cout << "Average decode: " << avg_decode << " microseconds" << endl;

    return frequencies;
}

vector<double> dct(SEALContext context, vector<double> data) {
    int N = data.size();
    cout << "N = " << N << endl;
    
    int no_dfs = 200;
    cout << "no_dfs = " << no_dfs << endl;

    vector<double> frequencies;

    for (int f = 0; f < no_dfs; f++) {
        double yk = 0;
        for (int j = 0; j < N; j++) {
            yk += data[j] * cos(M_PI * f * (2. * j + 1) / (2. * N));
        }

        frequencies.push_back(2*abs(yk));
    }

    return frequencies;
}

vector<double> secure_dct(SEALContext context, vector<double> data) {
    int N = data.size();
    cout << "datapoints: " << N << endl;

    int no_dfs = 200;
    cout << "no_dfs: " << no_dfs << endl;

    double scale = pow(2.0, 30);
    CKKSEncoder encoder(context);
    Plaintext pt;
    encoder.encode(data, scale, pt);

    // Set up keys
    KeyGenerator keygen(context);
    auto sk = keygen.secret_key();
    PublicKey pk;
    keygen.create_public_key(pk);
    RelinKeys relin_keys;
    keygen.create_relin_keys(relin_keys);
    GaloisKeys galk;
    keygen.create_galois_keys(galk);

    Encryptor encryptor(context, pk);
    Ciphertext ct;
    encryptor.encrypt(pt, ct);

    Evaluator evaluator(context);
    Decryptor decryptor(context, sk);

    vector<double> frequencies;

    for (int f = 0; f < no_dfs; f++) {

        vector<double> c;
        for (int j = 0; j < N; j++) {
            c.push_back(cos(M_PI * f * (2. * j + 1) / (2. * N)));
        }

        Plaintext c_pt;
        encoder.encode(c, scale, c_pt);

        // multiply and rescale
        Ciphertext result;
        evaluator.multiply_plain(ct, c_pt, result);
        evaluator.relinearize_inplace(ct, relin_keys);
        evaluator.rescale_to_next_inplace(result);

        // rotate and add for vector sum
        for (int i = 1; i <= encoder.slot_count() / 2; i <<= 1) {
            Ciphertext temp_ct;
            evaluator.rotate_vector(result, i, galk, temp_ct);
            evaluator.add_inplace(result, temp_ct);
        }

        // decrypt
        Plaintext pt_result;
        decryptor.decrypt(result, pt_result);

        // decode
        vector<double> vec_result;
        encoder.decode(pt_result, vec_result);

        std::printf("%2d: %10.3f\n", f, abs(vec_result[0]));

        // put result into return vector
        frequencies.push_back(abs(vec_result[0]));
    }

    return frequencies;
}

int main()
{
    EncryptionParameters parms(scheme_type::ckks);
    size_t poly_modulus_degree = 8192;

    parms.set_poly_modulus_degree(poly_modulus_degree);
    parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, { 45,30,30,30,45 }));
    double scale = pow(2.0, 25);

    SEALContext context(parms);

    print_parameters(context);

    vector<vector<double>> hydr = read_csv("../../sensitive_data/data.csv");
    vector<double> yraw = hydr[1];

    yraw.resize(200);

    double mean = accumulate(yraw.begin(), yraw.end(), 0.0) / yraw.size();
    double min = *min_element(yraw.begin(), yraw.end());
    double max = *max_element(yraw.begin(), yraw.end());

    /*
    // subtract mean to eliminate 0th frequency peak in frequency domain
    for (double& y : yraw) {
        y = (y - mean);
    }
    */

    // scale signal to [-1,+1]
    for (double& y : yraw) {
        y -= min;
        y /= (max - min);
        y = y * 2 - 1;
    }

    vector<double> freqs = secure_dft(context, yraw);
    
    // freqs[0] = 0;
    /*
    plt::plot(freqs);
    plt::show();
    */

    return 0;
}