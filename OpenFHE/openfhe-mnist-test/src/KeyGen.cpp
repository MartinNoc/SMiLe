#include "KeyGen.h"


std::vector<int> genRotations (
        uint32_t batchSize
) {
    std::set<int> resultSet;

    unsigned int n1 = find_n1(batchSize);
    unsigned int n2 = batchSize / n1;

    for (unsigned int k=0; k<n2; k++) {
        resultSet.insert(k*n1);
        for (unsigned int j=0; j<n1; j++)
            resultSet.insert((k*n1 + j));
    }

    std::vector<int> result(resultSet.size());
    std::copy(resultSet.begin(), resultSet.end(), result.begin());

    return result;
}
