#ifndef GAUSSIAN_MODEL_H_
#define GAUSSIAN_MODEL_H_

#define NUM_MOMENTS 4

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

void init_partial_sums();

void init_partial_moments(double moments[][NUM_MOMENTS], const double *series);

double compute_kurtosis(double moments[][NUM_MOMENTS],
	size_t left, size_t right,
	double min_value, double min_sum,
	double *r_mean, double *r_sigma);

double compute_emd(const double *series, size_t left, size_t right,
	double min_value, double min_sum,
	double mean, double sigma);

#ifdef __cplusplus
}
#endif

#endif /* GAUSSIAN_MODEL_H_ */
