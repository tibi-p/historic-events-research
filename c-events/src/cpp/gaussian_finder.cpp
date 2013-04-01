#include "gaussian_finder.h"
#include <algorithm>
#include <limits>
#include <gsl/gsl_randist.h>
#include "dictionary_reader.h"
#include "gaussian_model.h"

using namespace std;

void select_gaussians(const double *series, size_t inf, size_t sup, vector<gaussian_entry> &gaussians)
{
	double partial_moments[MAX_YEARS + 1][NUM_MOMENTS];
	double value, min_value;
	double sum, min_sum;
	double mean, sigma;
	double emd, kurtosis;

	init_partial_moments(partial_moments, series);

	gaussians.clear();
	for (size_t left = inf; left < sup; left++) {
		min_value = numeric_limits<double>::max();
		sum = 0.0;
		for (size_t right = left; right < sup && right <= left + 50; right++) {
			value = series[right];
			if (value < min_value)
				min_value = value;
			sum += value;
			if (right >= left + 4) {
				min_sum = sum - (right - left + 1) * min_value;
				kurtosis = compute_kurtosis(partial_moments, left, right,
					min_value, min_sum, &mean, &sigma);
				if (fabs(kurtosis) < .05) {
					emd = compute_emd(series, left, right, min_value, min_sum, mean, sigma);
					if (emd < .3) {
						double max_probability = gsl_ran_gaussian_pdf(0, sigma);
						double increase = min_sum * max_probability / min_value;
						//printf("{} %zu %zu :: (%lf, %lf) :: [%lf, %lf] :: <%lf, %lf> :: %lf\n", left, right,
						//	mean, sigma, emd, kurtosis, min_value, min_sum * max_probability, increase);
						gaussians.push_back(gaussian_entry(left, right, mean, sigma, emd, increase));
					}
				}
			}
		}
	}
	sort(gaussians.begin(), gaussians.end());
}
