#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <cctype>
#include "dictionary_reader.h"
#include "dictionary_types.h"
#include "util.h"

#define BUFFER_SIZE 1008
#define MAX_ENTRIES (1 << 20)

struct db_entry database[MAX_ENTRIES];
struct indexed_word words[MAX_ENTRIES];

double average_match_count(struct series_entry *series, size_t start, size_t end)
{
	double acc = 0;
	for (size_t i = start; i < end; i++)
		acc += series[i].match_count;
	return acc / (end - start);
}

int main()
{
	clock_t cs, ce;
	const char *summary_filename = "data/zeitgeist/summary.txt";
	FILE *zeitgeist;
	struct dictionary_reader dict;

	cs = clock();

	init_dictreader(&dict, "data/sort/googlebooks-eng-all-1gram-20120701-database");
	printf("num_words=%lu\n", dict.num_words);

#ifdef _WIN32
	_setmaxstdio(1024);
#endif

	zeitgeist = fopen(summary_filename, "wt");
	if (zeitgeist == NULL) {
		fprintf(stderr, "Could create file %s\n", summary_filename);
		exit(EXIT_FAILURE);
	}

	fread(database, sizeof(*database), dict.num_words, dict.files.main_file);
	for (size_t i = 0; i < dict.num_words; i++) {
		uint32_t word_offset = database[i].word_offset;
		uint32_t word_length = database[i].word_length;
		if (!is_in_word_bounds(&dict, word_offset, word_length)) {
			fprintf(stderr, "Invalid position in words file (%u +%u).\n",
				word_offset, word_length);
			exit(EXIT_FAILURE);
		}
		char *p = &dict.all_words[word_offset];
		char *word = (char *) malloc((word_length + 1) * sizeof(*word));
		memcpy(word, p, word_length * sizeof(*word));
		word[word_length] = 0;
		words[i].index = i;
		words[i].word = word;
	}

	for (size_t i = 0; i < dict.num_words; i++) {
		struct time_entry table[MAX_YEARS];
		struct series_entry series[MAX_YEARS];
		struct series_entry smooth_series[MAX_YEARS];
		struct db_entry *current_entry = &database[i];

		uint32_t offset = current_entry->time_offset;
		uint32_t length = current_entry->time_length;
		if (fseek64(dict.files.time_file, offset, SEEK_SET) != 0)
			exit(EXIT_FAILURE);
		size_t num_read = fread(table, sizeof(*table), length, dict.files.time_file);
		if (num_read != length)
			exit(EXIT_FAILURE);

		memset(series, 0, sizeof(series));
		for (size_t j = 0; j < num_read; j++) {
			struct time_entry *entry = &table[j];
			if (entry->year < 1500 || entry->year >= 1500 + MAX_YEARS) {
				fprintf(stderr, "The %luth entry has an invalid year: %lu (mc=%lu, vc=%lu)\n",
					(unsigned long) j, (unsigned long) entry->year,
					(unsigned long) entry->match_count, (unsigned long) entry->volume_count);
				exit(EXIT_FAILURE);
			}
			int pos = entry->year - 1500;
			series[pos].match_count = (double) (100 * entry->match_count) / dict.frequencies[pos].match_count;
			series[pos].volume_count = entry->volume_count;
		}

		double a = average_match_count(series, 430, 440);
		double b = average_match_count(series, 440, 445);
		double c = average_match_count(series, 445, 455);
#if 0
		if (a < b && b > c && b >= 1.e-4) {
			if (1.5 * a < b || b > 1.5 * c)
			{
				//if (words[i].word[0] == 'd')
				printf("w=%s (%lf, %lf, %lf)\n", words[i].word, a, b, c);
			}
		}
#endif
		int smoothing_window = 2;
		double smoothing_sum = 0.0;
		unsigned int window_size = 0;
		for (int j = 0; j < MAX_YEARS + smoothing_window; j++) {
			if (j < MAX_YEARS) {
				smoothing_sum += series[j].match_count;
				window_size++;
			}
			if (j >= 2 * smoothing_window + 1) {
				smoothing_sum -= series[j - 2 * smoothing_window - 1].match_count;
				window_size--;
			}
			smooth_series[j - smoothing_window].volume_count = series[j].volume_count;
			smooth_series[j - smoothing_window].match_count = (double) smoothing_sum / window_size;
		}
		if (strcmp(words[i].word, "war") == 0) {
			printf("c(");
			for (size_t j = 430; j < 455; j++) {
				//printf("%lf, ", smooth_series[j].match_count);
				printf("%lf, ", series[j].match_count);
				if (j % 5 == 4)
					printf("| ");
			}
			printf(")\n");
			printf("> %lf %lf %lf\n", a, b, c);
		}
		//for (int j = 1; j < MAX_YEARS - 1; j++) {
		for (int j = 251; j < 2008 - 1500; j++) {
			//a = series[j - 1].match_count;
			//b = series[j].match_count;
			//c = series[j + 1].match_count;
			a = smooth_series[j - 1].match_count;
			b = smooth_series[j].match_count;
			c = smooth_series[j + 1].match_count;
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
					fprintf(zeitgeist, "%s\t%d\t%d\n", words[i].word, 1500 + j, count);
			}
		}
	}

	for (size_t i = 0; i < dict.num_words; i++)
		free(words[i].word);

	destroy_dictreader(&dict);
	fclose(zeitgeist);

	ce = clock();
	printf("%f seconds\n", (float) (ce - cs) / CLOCKS_PER_SEC);

	return 0;
}
