#include "gaussian_finder.h"
#include <algorithm>
#include <bitset>
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

void relevant_gaussians(const vector<gaussian_entry> &gaussians,
	vector< pair<size_t, int> > &relevant_counts, double widening)
{
	bitset<MAX_YEARS> used;

	relevant_counts.clear();
	for (vector<gaussian_entry>::const_iterator it = gaussians.begin(); it != gaussians.end(); ++it) {
		size_t left = it->left;
		size_t right = it->right;
		bool valid = true;
		for (size_t i = left; i <= right; i++)
			if (used[i]) {
				valid = false;
				break;
			}
		if (valid) {
			double max_probability = gsl_ran_gaussian_pdf(0, it->sigma);
			double increase = min(it->increase, 1.0);
			int count = (int) (10 * increase);
			double ratio = (count + .5) / max_probability;
			double mean = it->mean;
			double sigma = it->sigma;
			for (size_t i = left; i <= right; i++) {
				used[i] = true;
				int entry_count;
				if (widening != numeric_limits<double>::max()) {
					double sample = gsl_ran_gaussian_pdf(i - mean, widening * sigma);
					entry_count = (int) (ratio * sample);
				} else {
					entry_count = count;
				}
				if (entry_count > 0)
					relevant_counts.push_back(make_pair(i, entry_count));
			}
		}
	}
}
