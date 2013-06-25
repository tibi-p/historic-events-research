#ifndef DICTIONARY_READER_H_
#define DICTIONARY_READER_H_

#include "dictionary_files.h"
#include "dictionary_types.h"

#define MAX_YEARS 509

#ifdef __cplusplus
extern "C" {
#endif

struct dictionary_reader {
	struct dictionary_files files;
	struct db_entry *database;
	char **words;
	size_t num_words;
	long long time_file_size;
	long word_file_size;
	struct total_counts_entry frequencies[MAX_YEARS];
};

struct indexed_word {
	size_t index;
	char *word;
};

typedef unsigned int (*time_feature_f)(const struct time_entry *);

int init_dictreader(struct dictionary_reader *dict, const char *base_filename);

void destroy_dictreader(struct dictionary_reader *dict);

int read_table(const struct dictionary_reader *dict, size_t index,
	struct time_entry *table, size_t *table_size);

void table_to_series(const struct dictionary_reader *dictreader,
	const struct time_entry *table, size_t table_size,
	double *series);

void table_to_feature_counts(const struct time_entry *table,
	size_t table_size, unsigned int *counts, time_feature_f feature);

int is_in_word_bounds(const struct dictionary_reader *dict,
	uint32_t word_offset, uint32_t word_length);

int iw_compare(const void *a, const void *b);

unsigned int volume_time_feature(const struct time_entry *entry);

unsigned int match_time_feature(const struct time_entry *entry);

#ifdef __cplusplus
}
#endif

#endif /* DICTIONARY_READER_H_ */
