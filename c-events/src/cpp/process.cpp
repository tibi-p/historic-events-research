#include <algorithm>
#include <limits>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include "dictionary_reader.h"
#include "dictionary_types.h"
#include "gaussian_finder.h"
#include "gaussian_model.h"
#include "numerical_discrepancy.h"
#include "linear_model.h"
#include "series.h"
#include "util.h"

using namespace std;

void process_series_double_change(const char *word,
	const double *series, FILE *zeitgeist)
{
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
			if (count > 0)
				fprintf(zeitgeist, "%s\t%d\t%d\n", word, j, count);
		}
	}
}

void process_series_linear_model(const char *word,
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
		if (score >= 1.0) {
			for (size_t j = entry->left; j <= entry->right; j++)
				fprintf(zeitgeist, "%s\t%lu\t%d\n", word,
					(unsigned long) j, (int) score);
		}
	}
}

void print_series(const double *series)
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
	vector< pair<size_t, int> > relevant_counts;

	select_gaussians(series, inf, sup, gaussians);
	relevant_gaussians(gaussians, relevant_counts, widening);
	for (vector< pair<size_t, int> >::iterator it = relevant_counts.begin(); it != relevant_counts.end(); ++it) {
		size_t year = it->first;
		int count = it->second;
		fprintf(zeitgeist, "%s\t%u\t%d\n", word, (unsigned int) year, count);
	}
}

void process_series_numerical_discrepancy(const char *word, double *series, FILE *zeitgeist)
{
	vector< pair< pair<size_t, size_t>, int > > intervals;

	fit_discrepancy(series, 2, intervals);
	sort(intervals.begin(), intervals.end());
	for (size_t i = 0; i < intervals.size(); i++) {
		pair<size_t, size_t> interval = intervals[i].first;
		int score = 2 * intervals[i].second + 1;
		for (size_t j = interval.first; j <= interval.second; j++)
			fprintf(zeitgeist, "%s\t%u\t%d\n", word, (unsigned int) j, score);
	}
}

int main()
{
	const char *summary_filenames[] = {
		"data/zeitgeist/summary/double_change_summary.txt",
		"data/zeitgeist/summary/linear_model_summary.txt",
		"data/zeitgeist/summary/numerical_discrepancy_summary.txt",
		"data/zeitgeist/summary/s1_gaussian_summary.txt",
		"data/zeitgeist/summary/s2_gaussian_summary.txt",
		"data/zeitgeist/summary/s3_gaussian_summary.txt",
		"data/zeitgeist/summary/sinf_gaussian_summary.txt",
	};
	const double parameters[] = {
		1.0, 2.0, 3.0, numeric_limits<double>::max()
	};
	FILE *zeitgeists[sizeof(summary_filenames) / sizeof(*summary_filenames)];

	const gsl_multimin_fdfminimizer_type *T;
	gsl_multimin_function_fdf regression_func;

	struct time_entry table[MAX_YEARS];
	double series[MAX_YEARS];
	double smooth_series[MAX_YEARS];
	struct dictionary_reader dict;
	size_t num_read;
	const unsigned int smoothing_window = 2;
#if 1
	struct static_range training_data = { 0, MAX_YEARS, smoothing_window,
		smooth_series + smoothing_window, MAX_YEARS - 2 * smoothing_window };
#else
	struct static_range training_data = { 0, 300, 1700,
		smooth_series + 200, 300 };
#endif
	size_t percent = 0;
	int err = 0;

	T = gsl_multimin_fdfminimizer_conjugate_pr;

	regression_func.n = 2;
	regression_func.f = regression_f;
	regression_func.df = regression_df;
	regression_func.fdf = regression_fdf;
	regression_func.params = &training_data;

	err = init_dictreader(&dict, "data/sort/googlebooks-eng-all-1gram-20120701-database");
	if (err != 0)
		goto out;

	for (size_t i = 0; i < sizeof(zeitgeists) / sizeof(*zeitgeists); i++) {
		if (!file_exists(summary_filenames[i])) {
			zeitgeists[i] = fopen(summary_filenames[i], "wt");
			if (zeitgeists[i] == NULL)
				goto out_reader;
		}
	}

	init_partial_sums();
	memset(smooth_series, 0, sizeof(smooth_series));

	for (size_t i = 0; i < dict.num_words; i++) {
		const char *word = dict.words[i];
		if (strchr(word, '_') != NULL)
			continue;
		if (dict.database[i].total_match_count < (1 << 18))
			continue;
		size_t new_percent = (100 * i) / dict.num_words;
		if (new_percent > percent) {
			percent = new_percent;
			if (percent % 4 == 0)
				printf("%u%% done\n", (unsigned int) percent);
		}
		err = read_table(&dict, i, table, &num_read);
		if (err != 0)
			goto out_reader;

		table_to_series(&dict, table, num_read, series);
		smoothify_series(series, smooth_series, MAX_YEARS, smoothing_window);
#if 0
		if (strcmp(words[i], "plague") == 0)
			print_series(smooth_series);
#endif
		if (zeitgeists[0] != NULL)
			process_series_double_change(word, smooth_series, zeitgeists[0]);
		if (zeitgeists[1] != NULL)
			process_series_linear_model(word, T, &regression_func, zeitgeists[1]);
		if (zeitgeists[2] != NULL)
			process_series_numerical_discrepancy(word, smooth_series, zeitgeists[2]);
		for (size_t j = 0; j < sizeof(parameters) / sizeof(*parameters); j++)
			if (zeitgeists[j + 3] != NULL) {
				fit_gaussians(word, smooth_series, smoothing_window,
					MAX_YEARS - smoothing_window, parameters[j], zeitgeists[j + 3]);
			}
	}

out_reader:
	destroy_dictreader(&dict);
	for (size_t i = 0; i < sizeof(zeitgeists) / sizeof(*zeitgeists); i++)
		if (zeitgeists[i] != NULL)
			fclose(zeitgeists[i]);

out:
	return err;
}
