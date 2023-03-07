#include "LinTools.h"


std::vector<double> rotate_plain(const std::vector<double>& vector, int index) {
    std::vector<double> result(vector.size());


    if(index == 0)
        return vector;
    else {
        if (index < 0)
            index = vector.size() + index % vector.size();

        for (unsigned int i = 0; i < vector.size(); i++) {
            unsigned int newIndex = (i + index) % vector.size();
            result[i] = vector[newIndex];
        }
    }

    return result;
}


Ciphertext<DCRTPoly> matrix_multiplication_diagonals (std::vector<std::vector<double>> matrix, const Ciphertext<DCRTPoly>& vector) {
    auto context = vector->GetCryptoContext();
    uint32_t batchSize = vector->GetEncodingParameters()->GetBatchSize();

    matrix = resizeMatrix(matrix, batchSize, batchSize);
    auto diagonals = diagonal_transformation(matrix);

    unsigned int n1 = find_n1(batchSize);
    unsigned int n2 = batchSize / n1;

    Plaintext pl = context->MakeCKKSPackedPlaintext(diagonals[0]);
    Ciphertext<DCRTPoly> subResult = context->EvalMult(pl, vector);

    auto cipherPrecompute = context->EvalFastRotationPrecompute(vector);
    uint32_t M = 2 * context->GetRingDimension();

    std::vector<Ciphertext<DCRTPoly>> rotCache;
    for (unsigned int j=1; j<n1; j++) {
        pl = context->MakeCKKSPackedPlaintext(diagonals[j]);
        Ciphertext<DCRTPoly> rotation = context->EvalFastRotation(vector, j, M, cipherPrecompute);
        rotCache.push_back(rotation);
        subResult += context->EvalMult(pl, rotation);
    }

    Ciphertext<DCRTPoly> result = subResult;

    for (unsigned int k=1; k<n2; k++) {
        pl = context->MakeCKKSPackedPlaintext(rotate_plain(diagonals[k*n1], -k*n1));
        subResult = context->EvalMult(pl, vector);

        for (unsigned int j=1; j<n1; j++) {
            pl = context->MakeCKKSPackedPlaintext(rotate_plain(diagonals[k*n1 + j], -k*n1));
            subResult += context->EvalMult(pl, rotCache[j-1]);
        }

        result += context->EvalRotate(subResult, k*n1);
    }

    return result;
}


std::vector<double> plain_matrix_multiplication(const std::vector<std::vector<double>>& matrix, const std::vector<double>& vector) {
    std::vector<double> result;

    for (int i=0; i<(int) matrix.size(); i++) {
        double entry = 0;

        for (int j=0; j<(int) matrix[0].size(); j++) {
            entry += matrix[i][j] * vector[j];
        }

        result.push_back(entry);
    }

    return result;
}


std::vector<double> plain_addition(std::vector<double> a, const std::vector<double>& b) {
    for (int j=0; j<(int) a.size(); j++) {
        a[j] += b[j];
    }

    return a;
}
