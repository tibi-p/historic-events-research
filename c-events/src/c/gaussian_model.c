#include "gaussian_model.h"
#include <math.h>
#include <gsl/gsl_randist.h>
#include "dictionary_reader.h"

static double query_moment(double v[][NUM_MOMENTS],
	size_t left, size_t right, size_t order);

static double query_full_moment(double moments[][NUM_MOMENTS],
	size_t left, size_t right, size_t order,
	double min_value, double min_sum);

static double partial_sums[MAX_YEARS + 1][NUM_MOMENTS];

void init_partial_sums()
{
	double value;
	size_t i, j;

	for (j = 0; j < NUM_MOMENTS; j++)
		partial_sums[0][j] = 0.0;

	for (i = 0; i < MAX_YEARS; i++) {
		value = 1.;
		for (j = 0; j < NUM_MOMENTS; j++) {
			value *= i;
			partial_sums[i + 1][j] = partial_sums[i][j] + value;
		}
	}
}

void init_partial_moments(double moments[][NUM_MOMENTS], const double *series)
{
	double value;
	size_t i, j;

	for (j = 0; j < NUM_MOMENTS; j++)
		moments[0][j] = 0.0;

	for (i = 0; i < MAX_YEARS; i++) {
		value = 1.;
		for (j = 0; j < NUM_MOMENTS; j++) {
			value *= i;
			moments[i + 1][j] = moments[i][j] + series[i] * value;
		}
	}
}

double compute_kurtosis(double moments[][NUM_MOMENTS],
	size_t left, size_t right,
	double min_value, double min_sum,
	double *r_mean, double *r_sigma)
{
	double mean;
	double sigma_2;
	double m_4;

	mean = query_full_moment(moments, left, right, 0, min_value, min_sum);
	sigma_2 = query_full_moment(moments, left, right, 1, min_value, min_sum) - mean * mean;
	m_4 = query_full_moment(moments, left, right, 3, min_value, min_sum)
		- 4 * mean * query_full_moment(moments, left, right, 2, min_value, min_sum)
		+ 6 * mean * mean * query_full_moment(moments, left, right, 1, min_value, min_sum)
		- 4 * mean * mean * mean * query_full_moment(moments, left, right, 0, min_value, min_sum)
		+ mean * mean * mean * mean;

	*r_mean = mean;
	*r_sigma = sqrt(sigma_2);

	return m_4 / (sigma_2 * sigma_2) - 3;
}

double compute_emd(const double *series, size_t left, size_t right,
	double min_value, double min_sum,
	double mean, double sigma)
{
	double distance;
	double emd;
	size_t i;

	distance = 0.;
	emd = 0.;
	for (i = left; i <= right; i++) {
		double r_value = (series[i] - min_value) / min_sum;
		double g_value = gsl_ran_gaussian_pdf(i - mean, sigma);
		emd += r_value - g_value;
		distance += fabs(emd);
	}

	return distance;
}

static double query_moment(double v[][NUM_MOMENTS],
	size_t left, size_t right, size_t order)
{
	return v[right + 1][order] - v[left][order];
}

static double query_full_moment(double moments[][NUM_MOMENTS],
	size_t left, size_t right, size_t order,
	double min_value, double min_sum)
{
	double x, y;

	x = query_moment(moments, left, right, order);
	y = query_moment(partial_sums, left, right, order);

	return (x - min_value * y) / min_sum;
}
