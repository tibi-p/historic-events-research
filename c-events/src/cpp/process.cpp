#include <algorithm>
#include <bitset>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <gsl/gsl_randist.h>
#include "dictionary_reader.h"
#include "dictionary_types.h"
#include "gaussian_finder.h"
#include "gaussian_model.h"
#include "linear_model.h"
#include "series.h"
#include "util.h"

using namespace std;

#define BUFFER_SIZE 1008
#define MAX_ENTRIES (1 << 20)

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

void fit_gaussians(const char *word, const double *series, size_t inf, size_t sup,
	double widening, FILE *zeitgeist)
{
	vector<gaussian_entry> gaussians;
	vector< pair<size_t, int> > counts;
	bitset<MAX_YEARS> used;

	select_gaussians(series, inf, sup, gaussians);
	relevant_gaussians(gaussians, counts, widening);
	for (vector< pair<size_t, int> >::iterator it = counts.begin(); it != counts.end(); ++it) {
		size_t year = it->first;
		int count = it->second;
		if (year >= 1750)
			fprintf(zeitgeist, "%s\t%u\t%d\n", word, (unsigned int) year, count);
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
		smooth_series + smoothing_window, MAX_YEARS - 2 * smoothing_window };
#else
	struct static_range training_data = { 0, 300, 1700,
		smooth_series + 200, 300 };
#endif
	int err = 0;

	T = gsl_multimin_fdfminimizer_conjugate_fr;

	regression_func.n = 2;
	regression_func.f = regression_f;
	regression_func.df = regression_df;
	regression_func.fdf = regression_fdf;
	regression_func.params = &training_data;

	zeitgeist = fopen(summary_filename, "wt");
	if (zeitgeist == NULL) {
		fprintf(stderr, "Could create file %s\n", summary_filename);
		exit(EXIT_FAILURE);
	}

	err = init_dictreader(&dict, "data/sort/googlebooks-eng-all-1gram-20120701-database");
	if (err != 0)
		goto out;
	//printf("num_words=%lu\n", dict.num_words);

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
		smoothify_series(series, smooth_series, MAX_YEARS, smoothing_window);
#if 0
		if (strcmp(words[i], "plague") == 0)
			print_series(smooth_series);
#endif
		//process_series(word, smooth_series, zeitgeist);
		//process_series(word, T, &regression_func, zeitgeist);
		fit_gaussians(word, smooth_series, smoothing_window, MAX_YEARS - smoothing_window, 2.0, zeitgeist);
	}

out_reader:
	destroy_dictreader(&dict);
	fclose(zeitgeist);

out:
	return err;
}
