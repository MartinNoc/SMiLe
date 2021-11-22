#include "seal/seal.h"
#include <iostream>
#include "examples.h"
#include<stdlib.h>
#include <time.h>

using namespace std;
using namespace seal;

template <typename T>
void print_v(vector<T> vec)
{
	/*
	Save the formatting information for std::cout.
	*/
	ios old_fmt(nullptr);
	old_fmt.copyfmt(cout);

	size_t slot_count = vec.size();

	cout << fixed << setprecision(3);
	cout << endl;

	std::cout << "    [";
	for (std::size_t i = 0; i < slot_count; i++)
	{
		std::cout << " " << vec[i] << ((i != slot_count - 1) ? "," : " ]\n");
	}

	/*
	Restore the old std::cout formatting.
	*/
	cout.copyfmt(old_fmt);
}

int main2()
{
	print_example_banner("Example: CKKS Basics");

	srand(time(NULL));

	EncryptionParameters parms(scheme_type::ckks);
	size_t poly_modulus_degree = 8192;
	parms.set_poly_modulus_degree(poly_modulus_degree);
	parms.set_coeff_modulus(CoeffModulus::Create(poly_modulus_degree, { 60, 40, 40, 60 }));

	double scale = pow(2.0, 40);

	auto context = SEALContext::SEALContext(parms);
	print_parameters(context);
	cout << endl;
	
	cout << "Parameters are valid: " << boolalpha << context.key_context_data()->qualifiers().parameters_set() << endl;
	cout << "Maximal allowed coeff_modulus bit-count for this poly_modulus_degree: "
		<< CoeffModulus::MaxBitCount(poly_modulus_degree) << endl;
	cout << "Current coeff_modulus bit-count: " << context.key_context_data()->total_coeff_modulus_bit_count() << endl;
	

	KeyGenerator keygen(context);
	PublicKey public_key;
	keygen.create_public_key(public_key);

	auto secret_key = keygen.secret_key();
	auto relin_keys = keygen.create_relin_keys();
	
	Encryptor encryptor(context, public_key);
	Evaluator evaluator(context);
	Decryptor decryptor(context, secret_key);

	CKKSEncoder encoder(context);
	size_t slot_count = encoder.slot_count();
	cout << "Number of slots: " << slot_count << endl;

	vector<double> inputs;
	int num_inputs = 11;
	inputs.reserve(num_inputs);
	for (int i = 0; i < num_inputs; i++)
	{
		inputs.push_back(1.0 * i / (num_inputs - 1.0));
	}
	cout << "Input vector: " << endl;
	print_vector(inputs, 100);

	// Create a vector of plaintexts
	vector<Plaintext> pts;
	for (auto val : inputs) {
		Plaintext p;

		// Encode val as a plaintext vector: [ val, val, val, val, ..., val ]
		// (poly_modulus_degree/2 == 4096 repetitions)
		encoder.encode(val, scale, p);
		pts.emplace_back(move(p));
	}

	// Create a vector of ciphertexts
	vector<Ciphertext> cts;
	for (const auto &p : pts) {
		Ciphertext c;
		encryptor.encrypt(p, c);
		cts.emplace_back(move(c));
	}


	// SERVER SIDE

	vector<double> weights;
	weights.reserve(num_inputs);

	vector<Plaintext> weight_pts;
	for (int i = 0; i < num_inputs; i++)
	{
		Plaintext p;
		double weight = rand() % 2 + 1.0;
		weights.push_back(weight);
		encoder.encode(weight, scale, p);
		weight_pts.emplace_back(p);
	}
	print_v(weights);

	for (int i = 0; i < cts.size(); i++)
	{
		evaluator.multiply_plain_inplace(cts[i], weight_pts[i]);
		evaluator.rescale_to_next_inplace(cts[i]);
	}

	// Sum up ciphertexts
	Ciphertext ct_result;
	evaluator.add_many(cts, ct_result);


	// CLIENT SIDE (again)

	Plaintext pt_result;
	decryptor.decrypt(ct_result, pt_result);
	vector<double> vec_result;
	encoder.decode(pt_result, vec_result);
	cout << "Result: " << vec_result[0] << endl;
	cout << "True result: " << inner_product(inputs.cbegin(), inputs.cend(), weights.cbegin(), 0.0) << endl;

	return 0;
}