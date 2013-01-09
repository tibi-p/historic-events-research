#include <algorithm>
#include <bitset>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <gsl/gsl_randist.h>
#include "dictionary_reader.h"
#include "dictionary_types.h"
#include "linear_model.h"
#include "util.h"

#define BUFFER_SIZE 1008
#define MAX_ENTRIES (1 << 20)

#define EPSILON 1e-6

using namespace std;

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

void append_csv(const char *filename, const char *word,
	int *counts, size_t num_elems)
{
	FILE *f;
	size_t i;

	f = fopen(filename, "at");
	if (f == NULL) {
		fprintf(stderr, "Could not open %s for writing\n", filename);
		exit(EXIT_FAILURE);
	}

	fprintf(f, "%s", word);
	for (i = 0; i < num_elems; i++)
		fprintf(f, ",%d", counts[i]);
	fprintf(f, "\n");

	fclose(f);
}

void double_change_series_to_csv(const char *word, double *series)
{
	int counts[MAX_YEARS];

	memset(counts, 0, sizeof(counts));
	for (int j = 2; j < MAX_YEARS - 2; j++) {
		double a = series[j - 1];
		double b = series[j];
		double c = series[j + 1];
		if (a >= 0 && b >= 1e-4 && c >= 0) {
			int count = 0;
			while (count <= 8) {
				double sup_ratio = 1.0 + 0.1 * (count + 1);
				double dsup_ratio = 1.0 + 0.2 * (count + 1);
				double inf_ratio = 1.0 - 0.1 * (count + 1);
				double dinf_ratio = 1 / dsup_ratio;
				if ((b > sup_ratio * a && c > sup_ratio * b) || b > dsup_ratio * a ||
						(b < inf_ratio * a && c < inf_ratio * b) || b < dinf_ratio * a) {
					++count;
				} else {
					break;
				}
			}
			counts[j] = count;
		}
	}

	append_csv("data/relevance/double_change.csv", word, counts, MAX_YEARS);
}

#define NUM_MOMENTS 4

double partial_sums[MAX_YEARS + 1][NUM_MOMENTS];
double partial_moments[MAX_YEARS + 1][NUM_MOMENTS];

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

void init_partial_moments(double *series)
{
	double value;
	size_t i, j;

	for (j = 0; j < NUM_MOMENTS; j++)
		partial_moments[0][j] = 0.0;

	for (i = 0; i < MAX_YEARS; i++) {
		value = 1.;
		for (j = 0; j < NUM_MOMENTS; j++) {
			value *= i;
			partial_moments[i + 1][j] = partial_moments[i][j] + series[i] * value;
		}
	}
}

void linear_model_series_to_csv(const char *word,
	const gsl_multimin_fdfminimizer_type *T,
	gsl_multimin_function_fdf *fdf)
{
	int counts[MAX_YEARS];
	struct static_array ranges;
	const int score_threshold = 3;

	memset(counts, 0, sizeof(counts));
	normalize_generate_ranges(&ranges, T, fdf);
	for (size_t i = 0; i < ranges.size; i++) {
		const struct range_entry *entry = &ranges.array[i];
		double score = -log(fabs(entry->slope));
		score = 2 * (score_threshold - score);
		int count = (int) score;
#if 0
		printf("%zu-%zu : %d (score=%lf slope=%lf)\n", entry->left, entry->right,
			count > 0 ? count : 0, score, entry->slope);
#endif
		if (count > 0) {
			for (size_t j = entry->left; j <= entry->right; j++)
				counts[j - MIN_YEAR] = count;
		}
	}

	append_csv("data/relevance/linear_model.csv", word, counts, MAX_YEARS);
}

double query_moment(double v[][NUM_MOMENTS], size_t left, size_t right, size_t order)
{
	return v[right + 1][order] - v[left][order];
}

double query_full_moment(size_t left, size_t right, size_t order,
	double min_value, double min_sum)
{
	double x, y;

	x = query_moment(partial_moments, left, right, order);
	y = query_moment(partial_sums, left, right, order);

	return (x - min_value * y) / min_sum;
}

double compute_kurtosis(size_t left, size_t right,
	double min_value, double min_sum,
	double *r_mean, double *r_sigma)
{
	double mean;
	double sigma_2;
	double m_4;

	mean = query_full_moment(left, right, 0, min_value, min_sum);
	sigma_2 = query_full_moment(left, right, 1, min_value, min_sum) - mean * mean;
	m_4 = query_full_moment(left, right, 3, min_value, min_sum)
		- 4 * mean * query_full_moment(left, right, 2, min_value, min_sum)
		+ 6 * mean * mean * query_full_moment(left, right, 1, min_value, min_sum)
		- 4 * mean * mean * mean * query_full_moment(left, right, 0, min_value, min_sum)
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

	distance = 0.;
	emd = 0.;
	for (size_t i = left; i <= right; i++) {
		double r_value = (series[i] - min_value) / min_sum;
		double g_value = gsl_ran_gaussian_pdf(i - mean, sigma);
		emd += r_value - g_value;
		distance += fabs(emd);
	}

#if 0
	double ratio = min_sum / (right - left + 1);
	ratio *= 100;
#endif

	return distance;
}

void fit_gaussians(double *series, size_t inf, size_t sup, int *counts)
{
	vector<gaussian_entry> gaussians;
	bitset<MAX_YEARS> used;
	double value, min_value;
	double sum, min_sum;
	double mean, sigma;
	double emd, kurtosis;

	for (size_t left = inf; left < sup; left++) {
		min_value = DBL_MAX;
		sum = 0.;
		for (size_t right = left; right < sup && right < left + 50; right++) {
			value = series[right];
			if (value < min_value)
				min_value = value;
			sum += value;
			if (right >= left + 4) {
				min_sum = sum - (right - left + 1) * min_value;
				kurtosis = compute_kurtosis(left, right, min_value, min_sum, &mean, &sigma);
				if (fabs(kurtosis) < .05) {
				//if (fabs(kurtosis) < .1) {
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
			if (count > 10)
				count = 10;
			//printf("count=%d\n", count);
			double ratio = (count + .5) / max_probability;
			for (size_t i = left; i <= right; i++) {
				used[i] = true;
				counts[i] = (int) (ratio * gsl_ran_gaussian_pdf(i - it->mean, it->sigma));
			}
			//printf("l=%zu r=%zu :: %lf -- (%lf, %lf)\n", it->left, it->right, it->distance, it->mean, it->sigma);
		}
	}
}

void gaussian_model_series_to_csv(const char *word, double *series)
{
	int counts[MAX_YEARS];

	init_partial_moments(series);
	memset(counts, 0, sizeof(counts));
	fit_gaussians(series, 2, MAX_YEARS - 2, counts);

	append_csv("data/relevance/gaussian_model.csv", word, counts, MAX_YEARS);
}

void smoothify_series(double *series, double *smooth_series)
{
	const unsigned int smoothing_window = 2;
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
			if (smoothing_sum < 0.)
				smoothing_sum = 0.;
			if (window_size == 2 * smoothing_window + 1) {
				smooth_series[pos] = (double) smoothing_sum / window_size;
			}
		}
	}
}

int handle_entry(const struct dictionary_reader *dictreader, size_t index,
	const gsl_multimin_fdfminimizer_type *T,
	gsl_multimin_function_fdf regression_func,
	double *smooth_series)
{
	struct time_entry table[MAX_YEARS];
	double series[MAX_YEARS];
	const char *word;
	size_t table_size;
	int err;

	err = read_table(dictreader, index, table, &table_size);
	if (err != 0)
		goto out;

	table_to_series(dictreader, table, table_size, series);
	smoothify_series(series, smooth_series);

	word = dictreader->words[index];
	double_change_series_to_csv(word, smooth_series);
	gaussian_model_series_to_csv(word, smooth_series);
	linear_model_series_to_csv(word, T, &regression_func);

out:
	return err;
}

int string_compare(const void *a, const void *b)
{
	const char **x = (const char **) a;
	const char **y = (const char **) b;
	return strcmp(*x, *y);
}

int main()
{
	const gsl_multimin_fdfminimizer_type *T;
	gsl_multimin_function_fdf regression_func;

	double smooth_series[MAX_YEARS];
	struct dictionary_reader dict;
	const unsigned int smoothing_window = 2;
	struct static_range training_data = { 0, MAX_YEARS, 1500 + smoothing_window,
		/*{ 0.0, 0.0, 0.0, },*/
		smooth_series + smoothing_window, MAX_YEARS - 2 * smoothing_window };
	int err = 0;
	const char *words[] = {
		"war",
#if 1
		"women",
		"communism",
		"atomic",
		"trench",
		"plague",
		"bomb",
		"Vietnam",
		"slavery",
		"famine",
		"Cuba",
		"terrorism",
		"assassination",
		"genocide",
		"revolution",
		"attack",
		"rebel",
		"fascism",
		"siege",
		"battle",
		"secession",
		"bombing",
		"Ceaucescu",
		"Jew",
#endif
	};

	T = gsl_multimin_fdfminimizer_conjugate_fr;

	regression_func.n = 2;
	regression_func.f = regression_f;
	regression_func.df = regression_df;
	regression_func.fdf = regression_fdf;
	regression_func.params = &training_data;

	err = init_dictreader(&dict, "data/sort/googlebooks-eng-all-1gram-20120701-database");
	if (err != 0)
		goto out;

	init_partial_sums();
	memset(smooth_series, 0, sizeof(smooth_series));

	remove("data/relevance/double_change.csv");
	remove("data/relevance/linear_model.csv");
	remove("data/relevance/gaussian_model.csv");

	for (size_t i = 0; i < sizeof(words) / sizeof(*words); i++) {
		void *p = bsearch(&words[i], dict.words, dict.num_words,
			sizeof(*dict.words), string_compare);
		if (p != NULL) {
			char **marker = (char **) p;
			ptrdiff_t index = marker - dict.words;
			err = handle_entry(&dict, (size_t) index, T, regression_func, smooth_series);
			if (err != 0)
				goto out_reader;
		}
	}

out_reader:
	destroy_dictreader(&dict);

out:
	return err;
}
