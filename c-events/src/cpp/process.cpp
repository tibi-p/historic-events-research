#include <algorithm>
#include <bitset>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <gsl/gsl_randist.h>
#include "dictionary_reader.h"
#include "dictionary_types.h"
#include "gaussian_model.h"
#include "linear_model.h"
#include "util.h"

using namespace std;

#define BUFFER_SIZE 1008
#define MAX_ENTRIES (1 << 20)

#define EPSILON 1e-6

struct gaussian_entry {

	size_t left, right;
	double mean, sigma;
	double distance, increase;

	gaussian_entry(size_t left, size_t right, double mean, double sigma, double distance, double increase) :
		left(left), right(right), mean(mean), sigma(sigma), distance(distance), increase(increase) { }

	bool operator<(const gaussian_entry &other) const
	{
		if (fabs(distance - other.distance) >= EPSILON)
			return distance < other.distance;
		if (fabs(increase - other.increase) >= EPSILON)
			return increase > other.increase;
		if (left != other.left)
			return left < other.left;
		if (right != other.right)
			return right < other.right;
		if (fabs(mean - other.mean) >= EPSILON)
			return mean < other.mean;
		if (fabs(sigma - other.sigma) >= EPSILON)
			return sigma < other.sigma;
		return false;
	}

};

double average_match_count(struct series_entry *series, size_t start, size_t end)
{
	double acc = 0;
	for (size_t i = start; i < end; i++)
		acc += series[i].match_count;
	return acc / (end - start);
}

void process_series_double_change(const char *word,
	const struct series_entry *series, FILE *zeitgeist)
{
	double x, y, z;

	for (int j = 2; j < MAX_YEARS - 2; j++) {
		x = series[j - 1].match_count;
		y = series[j].match_count;
		z = series[j + 1].match_count;
		if (x >= 0 && y >= 1e-4 && z >= 0) {
			int count = 0;
			while (count <= 8) {
				double sup_ratio = 1.0 + 0.1 * (count + 1);
				double dsup_ratio = 1.0 + 0.2 * (count + 1);
				double inf_ratio = 1.0 - 0.1 * (count + 1);
				double dinf_ratio = 1 / dsup_ratio;
				if ((y > sup_ratio * x && z > sup_ratio * y) || y > dsup_ratio * x ||
						(y < inf_ratio * x && z < inf_ratio * y) || y < dinf_ratio * x) {
					++count;
				} else {
					break;
				}
			}
			if (count > 0)
				fprintf(zeitgeist, "%s\t%d\t%d\n", word, 1500 + j, count);
		}
	}
}

void process_series(const char *word,
	const gsl_multimin_fdfminimizer_type *T,
	gsl_multimin_function_fdf *fdf,
	FILE *zeitgeist)
{
	struct static_array ranges;
	const int score_threshold = 3;

	normalize_generate_ranges(&ranges, T, fdf);
	for (size_t i = 0; i < ranges.size; i++) {
		const struct range_entry *entry = &ranges.array[i];
		double score = -log(fabs(entry->slope));
		score = 2 * (score_threshold - score);
		if (score >= 1.0 && entry->left >= 1750) {
			for (size_t j = entry->left; j <= entry->right; j++)
				fprintf(zeitgeist, "%s\t%lu\t%d\n", word,
					(unsigned long) j, (int) score);
		}
	}
}

void print_series(double *series)
{
	printf("c(");
	for (size_t j = 0; j < MAX_YEARS; j++) {
		if (j % 5 == 0)
			printf("\n\t");
		printf("%lf,", series[j]);
		if ((j + 1) % 5 != 0)
			printf(" ");
	}
	printf(")\n");
}

void fit_gaussians(const char *word, double *series, size_t inf, size_t sup, FILE *zeitgeist)
{
	vector<gaussian_entry> gaussians;
	bitset<MAX_YEARS> used;
	double partial_moments[MAX_YEARS + 1][NUM_MOMENTS];
	double value, min_value;
	double sum, min_sum;
	double mean, sigma;
	double emd, kurtosis;

	init_partial_moments(partial_moments, series);

	for (size_t left = inf; left < sup; left++) {
		min_value = DBL_MAX;
		sum = 0.;
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

	for (vector<gaussian_entry>::iterator it = gaussians.begin(); it != gaussians.end(); ++it) {
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
			double increase = it->increase;
			if (increase > 1.)
				increase = 1.;
			int count = (int) (10 * increase);
			//printf("count=%d\n", count);
			double ratio = (count + .5) / max_probability;
			for (size_t i = left; i <= right; i++) {
				used[i] = true;
				//int current_count = (int) (ratio * gsl_ran_gaussian_pdf(i - it->mean, it->sigma));
				int current_count = (int) (ratio * gsl_ran_gaussian_pdf(i - it->mean, 2 * it->sigma));
				if (current_count > 0 && i >= 250)
					fprintf(zeitgeist, "%s\t%d\t%d\n", word, (int) (1500 + i), current_count);
			}
		}
	}
}

int main()
{
	const char *summary_filename = "data/zeitgeist/summary.txt";
	FILE *zeitgeist;

	const gsl_multimin_fdfminimizer_type *T;
	gsl_multimin_function_fdf regression_func;

	struct time_entry table[MAX_YEARS];
	double series[MAX_YEARS];
	double smooth_series[MAX_YEARS];
	struct dictionary_reader dict;
	size_t num_read;
	const unsigned int smoothing_window = 2;
#if 1
	struct static_range training_data = { 0, MAX_YEARS, 1500 + smoothing_window,
		/*{ 0.0, 0.0, 0.0, },*/
		smooth_series + smoothing_window, MAX_YEARS - 2 * smoothing_window };
#else
	struct static_range training_data = { 0, 300, 1700,
		/*{ 0.0, 0.0, 0.0, },*/
		smooth_series + 200, 300 };
#endif
	int err = 0;

	T = gsl_multimin_fdfminimizer_conjugate_fr;

	regression_func.n = 2;
	regression_func.f = regression_f;
	regression_func.df = regression_df;
	regression_func.fdf = regression_fdf;
	regression_func.params = &training_data;

	err = init_dictreader(&dict, "data/sort/googlebooks-eng-all-1gram-20120701-database");
	if (err != 0)
		goto out;
	//printf("num_words=%lu\n", dict.num_words);

	zeitgeist = fopen(summary_filename, "wt");
	if (zeitgeist == NULL) {
		fprintf(stderr, "Could create file %s\n", summary_filename);
		exit(EXIT_FAILURE);
	}

	init_partial_sums();
	memset(smooth_series, 0, sizeof(smooth_series));

	for (size_t i = 0; i < dict.num_words; i++) {
		const char *word = dict.words[i];
#if 0
		if (
				strcmp(word, "war") != 0 &&
				strcmp(word, "trench") != 0 &&
				1)
			continue;
#endif
		if (strchr(word, '_') != NULL)
			continue;
		if (dict.database[i].total_match_count < (1 << 20))
			continue;
		if (i % 1000 == 0)
			printf("iter=%zu\n", i);
		err = read_table(&dict, i, table, &num_read);
		if (err != 0)
			goto out_reader;

		table_to_series(&dict, table, num_read, series);

		double smoothing_sum = 0.0;
		unsigned int window_size = 0;
		for (unsigned int j = 0; j < MAX_YEARS + smoothing_window; j++) {
			if (j < MAX_YEARS) {
				smoothing_sum += series[j];
				window_size++;
			}
			if (j >= 2 * smoothing_window + 1) {
				smoothing_sum -= series[j - 2 * smoothing_window - 1];
				window_size--;
			}
			if (j >= smoothing_window) {
				unsigned int pos = j - smoothing_window;
				if (window_size == 2 * smoothing_window + 1)
					smooth_series[pos] = (double) smoothing_sum / window_size;
			}
		}
#if 0
		if (strcmp(words[i], "plague") == 0)
			print_series(smooth_series);
#endif
		//process_series(word, smooth_series, zeitgeist);
		//process_series(word, T, &regression_func, zeitgeist);
		fit_gaussians(word, smooth_series, smoothing_window, MAX_YEARS - smoothing_window, zeitgeist);
	}

out_reader:
	destroy_dictreader(&dict);
	fclose(zeitgeist);

out:
	return err;
}
