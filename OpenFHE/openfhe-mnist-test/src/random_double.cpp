#include "random_double.h"


double random_double(double min, double max) {

    double delta = max - min;
    double offset = ((double) rand() / (double) RAND_MAX);

    double res = min + delta * offset;

    return res;
}
