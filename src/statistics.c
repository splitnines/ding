#include <math.h>
#include <stddef.h>


double sum_total(double *samples, size_t count) {
    double sum = samples[0];
    for (size_t i = 1; i < count; i++)
        sum += samples[i];
    return sum;
}

double min(double *samples, size_t count) {
    if (count == 0)
        return INFINITY;

    double m = samples[0];
    for (size_t i = 1; i < count; i++)
        m = fmin(m, samples[i]);
    return m;
}

double max(double *samples, size_t count) {
    if (count == 0)
        return INFINITY;
    
    double m = samples[0];
    for (size_t i = 1; i < count; i++)
        m = fmax(m, samples[i]);
    return m;
}

double mdev(double *samples, size_t count) {
    if (count == 0)
        return INFINITY;

    double sum = sum_total(samples, count);
    double avg = sum / count;

    double variance = 0.0;
    for (size_t i = 0; i < count; i++) {
        double diff = samples[i] - avg;
        variance += diff * diff;
    }
    return sqrt(variance / count);
}
