#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cctype>
#include "dictionary_reader.h"
#include "dictionary_types.h"
#include "linear_model.h"
#include "util.h"

#define BUFFER_SIZE 1008
#define MAX_ENTRIES (1 << 20)

struct db_entry database[MAX_ENTRIES];
char *words[MAX_ENTRIES];

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
	for (int j = 2; j < MAX_YEARS - 2; j++) {
		double a = series[j - 1].match_count;
		double b = series[j].match_count;
		double c = series[j + 1].match_count;
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
				fprintf(zeitgeist, "%s\t%d\t%d\n", word, 1500 + j, count);
		}
	}
}

void process_series(const char *word,
	const gsl_multimin_fdfminimizer_type *T,
	gsl_multimin_function_fdf *fdf,
	FILE *zeitgeist)
{
	struct static_range *training_data;
	struct static_array ranges;
	const int score_threshold = 3;

	training_data = (struct static_range *) fdf->params;
	normalize_standard_score(training_data->array, training_data->size);
	generate_ranges(&ranges, T, fdf);
	for (size_t i = 0; i < ranges.size; i++) {
		const struct range_entry *entry = &ranges.array[i];
		int score = (int) -log(fabs(entry->slope));
		//printf("%zu-%zu: %lf\n", entry->left, entry->right, -log(fabs(entry->slope)));
		if (score < score_threshold && entry->left >= 1750) {
			for (size_t j = entry->left; j <= entry->right; j++)
				fprintf(zeitgeist, "%s\t%lu\t%d\n", word,
					(unsigned long) j, score_threshold - score);
		}
	}
}

void build_words(const struct dictionary_reader *dict)
{
	for (size_t i = 0; i < dict->num_words; i++) {
		uint32_t word_offset = database[i].word_offset;
		uint32_t word_length = database[i].word_length;
		if (!is_in_word_bounds(dict, word_offset, word_length)) {
			fprintf(stderr, "Invalid position in words file (%u +%u).\n",
				word_offset, word_length);
			exit(EXIT_FAILURE);
		}
		const char *p = &dict->all_words[word_offset];
		char *word = (char *) malloc((word_length + 1) * sizeof(*word));
		memcpy(word, p, word_length * sizeof(*word));
		word[word_length] = 0;
		words[i] = word;
	}
}

size_t read_table(const struct dictionary_reader *dict, const struct db_entry *entry,
	struct time_entry *table)
{
	size_t num_read;

	uint32_t offset = entry->time_offset;
	uint32_t length = entry->time_length;
	if (fseek64(dict->files.time_file, offset, SEEK_SET) != 0)
		exit(EXIT_FAILURE);

	num_read = fread(table, sizeof(*table), length, dict->files.time_file);
	if (num_read != length)
		exit(EXIT_FAILURE);

	return num_read;
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

int main()
{
	clock_t cs, ce;
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
	struct static_range training_data = { 0, MAX_YEARS, 1500 + smoothing_window,
		/*{ 0.0, 0.0, 0.0, },*/
		smooth_series + smoothing_window, MAX_YEARS - 2 * smoothing_window };

	T = gsl_multimin_fdfminimizer_conjugate_fr;

	regression_func.n = 2;
	regression_func.f = regression_f;
	regression_func.df = regression_df;
	regression_func.fdf = regression_fdf;
	regression_func.params = &training_data;

	cs = clock();

	init_dictreader(&dict, "data/sort/googlebooks-eng-all-1gram-20120701-database");
	printf("num_words=%lu\n", dict.num_words);

	zeitgeist = fopen(summary_filename, "wt");
	if (zeitgeist == NULL) {
		fprintf(stderr, "Could create file %s\n", summary_filename);
		exit(EXIT_FAILURE);
	}

	num_read = fread(database, sizeof(*database), dict.num_words, dict.files.main_file);
	if (num_read != dict.num_words)
		exit(EXIT_FAILURE);
	build_words(&dict);

	memset(smooth_series, 0, sizeof(smooth_series));

	for (size_t i = 0; i < dict.num_words; i++) {
		if (database[i].total_match_count < (1 << 21))
			continue;
		if (i % 1000 == 0)
			printf("i=%zu:: %lf seconds\n", i, (double) (clock() - cs) / CLOCKS_PER_SEC);
		num_read = read_table(&dict, &database[i], table);

		memset(series, 0, sizeof(series));
		for (size_t j = 0; j < num_read; j++) {
			struct time_entry *entry = &table[j];
			if (entry->year < 1500 || entry->year >= 1500 + MAX_YEARS) {
				fprintf(stderr, "The %luth entry has an invalid year: %lu\n",
					(unsigned long) j, (unsigned long) entry->year);
				exit(EXIT_FAILURE);
			}
			int pos = entry->year - 1500;
			uint64_t year_match_count = dict.frequencies[pos].match_count;
			series[pos] = (double) (100 * entry->match_count) / year_match_count;
		}

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
		if (strcmp(words[i], "plague") == 0)
			print_series(smooth_series);
		//process_series(words[i], smooth_series, zeitgeist);
		process_series(words[i], T, &regression_func, zeitgeist);
	}

	for (size_t i = 0; i < dict.num_words; i++)
		free(words[i]);

	destroy_dictreader(&dict);
	fclose(zeitgeist);

	ce = clock();
	printf("%f seconds\n", (float) (ce - cs) / CLOCKS_PER_SEC);

	return 0;
}
