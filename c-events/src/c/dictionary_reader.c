#include <stdlib.h>
#include <string.h>
#include "dictionary_reader.h"
#include "dictionary_types.h"
#include "util.h"

#define BUFFER_SIZE 11000

static int load_frequencies(struct total_counts_entry *frequencies,
	const char *filename);
static int load_database(struct dictionary_reader *self);
static int load_words(struct dictionary_reader *self, size_t word_file_size);
static void free_words(char **words, size_t num_words);

static char * build_all_words(struct dictionary_reader *dict, size_t word_file_size);

int init_dictreader(struct dictionary_reader *dict, const char *base_filename)
{
	size_t word_file_size;
	long main_file_size;
	int err = 0;

	err = init_dictfiles(&dict->files, base_filename, "rb");
	if (err != 0) {
		goto out;
	}

	memset(dict->frequencies, 0, sizeof(dict->frequencies));
	err = load_frequencies(dict->frequencies, "data/googlebooks-eng-all-totalcounts-20120701.txt");
	if (err != 0) {
		goto out_files;
	}

	main_file_size = get_file_size(dict->files.main_file);
	if (main_file_size < 0) {
		err = 1;
		goto out_files;
	}

	dict->word_file_size = get_file_size(dict->files.word_file);
	if (dict->word_file_size < 0) {
		err = 1;
		goto out_files;
	}
	word_file_size = (size_t) dict->word_file_size;

	dict->time_file_size = get_file_size64(dict->files.time_file);
	if (dict->time_file_size < 0) {
		err = 1;
		goto out_files;
	}

	dict->num_words = (size_t) main_file_size / sizeof(struct db_entry);

	err = load_database(dict);
	if (err != 0)
		goto out_files;

	err = load_words(dict, word_file_size);
	if (err != 0)
		goto out_database;

out:
	return err;

out_database:
	free(dict->database);
out_files:
	destroy_dictfiles(&dict->files);
	goto out;
}

void destroy_dictreader(struct dictionary_reader *dict)
{
	free_words(dict->words, dict->num_words);
	free(dict->database);
	destroy_dictfiles(&dict->files);
}

int read_table(const struct dictionary_reader *dict, size_t index,
	struct time_entry *table, size_t *table_size)
{
	const struct db_entry *entry;
	size_t num_read;
	uint32_t offset, length;
	int err = 0;

	entry = &dict->database[index];
	offset = entry->time_offset;
	length = entry->time_length;

	if (length == 0) {
		fprintf(stderr, "Invalid word length: %lu\n",
			(unsigned long) length);
		err = 2;
		goto out;
	}

	err = fseek64(dict->files.time_file, offset, SEEK_SET);
	if (err != 0) {
		fprintf(stderr, "Could not seek in the time file to offset: %lu\n",
			(unsigned long) offset);
		err = 1;
		goto out;
	}

	num_read = fread(table, sizeof(*table), length, dict->files.time_file);
	if (num_read != length) {
		fprintf(stderr, "Tried reading %lu entries from the time file, managed only %lu.",
			(unsigned long) length, (unsigned long) num_read);
		err = 1;
		goto out;
	}

	*table_size = num_read;
out:
	return err;
}

void table_to_series(const struct dictionary_reader *dictreader,
	const struct time_entry *table, size_t table_size,
	double *series)
{
	const struct time_entry *entry;
	uint64_t year_match_count;
	size_t j;
	int pos;

	memset(series, 0, MAX_YEARS * sizeof(*series));
	for (j = 0; j < table_size; j++) {
		entry = &table[j];
		if (entry->year < MIN_YEAR || entry->year >= MIN_YEAR + MAX_YEARS) {
			fprintf(stderr, "The %luth entry has an invalid year: %u\n",
				(unsigned long) j, (unsigned int) entry->year);
			exit(EXIT_FAILURE);
		}
		pos = entry->year - MIN_YEAR;
		year_match_count = dictreader->frequencies[pos].match_count;
		series[pos] = (double) (100 * entry->match_count) / year_match_count;
	}
}

int is_in_word_bounds(const struct dictionary_reader *dict,
	uint32_t word_offset, uint32_t word_length)
{
	unsigned long file_size = (unsigned long) dict->word_file_size;
	return word_offset < file_size && word_length <= file_size - word_offset;
}

int iw_compare(const void *a, const void *b)
{
	const struct indexed_word *x = a;
	const struct indexed_word *y = b;

	return strcmp(x->word, y->word);
}

static int load_frequencies(struct total_counts_entry *frequencies,
	const char *filename)
{
	FILE *f;
	char *p;
	uint64_t match_count;
	int year, pos;
	int page_count, volume_count;
	int err = 0;
	char line[BUFFER_SIZE];

	f = fopen(filename, "rt");
	if (f == NULL) {
		fprintf(stderr, "Could not open for reading: %s\n", filename);
		err = 1;
		goto out;
	}

	if (fgets(line, sizeof(line), f) == NULL) {
		fprintf(stderr, "Could not read line from: %s\n", filename);
		err = 1;
		goto out_file;
	}

	p = strtok(line, " ,\t");
	while (p != NULL) {
		year = atoi(p);
		p = strtok(NULL, ",\t");
		match_count = strtoull(p, NULL, 10);
		p = strtok(NULL, ",\t");
		page_count = atoi(p);
		p = strtok(NULL, ",\t");
		volume_count = atoi(p);
		p = strtok(NULL, ",\t");

		pos = year - MIN_YEAR;
		frequencies[pos].match_count = match_count;
		frequencies[pos].page_count = (uint32_t) page_count;
		frequencies[pos].volume_count = (uint32_t) volume_count;
	}

out_file:
	fclose(f);
out:
	return err;
}

static int load_database(struct dictionary_reader *self)
{
	struct db_entry *database;
	size_t num_read;
	int err = 0;

	database = malloc(self->num_words * sizeof(*database));
	if (database == NULL) {
		fprintf(stderr, "Could not allocate memory for database\n");
		err = 1;
		goto out;
	}

	num_read = fread(database, sizeof(*database), self->num_words, self->files.main_file);
	if (num_read != self->num_words) {
		fprintf(stderr, "Tried reading %lu entries from the main file, managed only %lu.",
			(unsigned long) self->num_words, (unsigned long) num_read);
		err = 1;
		goto out_database;
	}

	self->database = database;

out:
	return err;

out_database:
	free(database);
	goto out;
}

static int load_words(struct dictionary_reader *self, size_t word_file_size)
{
	char **words;
	const char *p;
	char *all_words, *word;
	size_t i;
	int err = 0;

	all_words = build_all_words(self, word_file_size);
	if (all_words == NULL) {
		err = 1;
		goto out;
	}

	words = malloc(self->num_words * sizeof(*words));
	if (words == NULL) {
		err = 1;
		goto out_all_words;
	}

	for (i = 0; i < self->num_words; i++) {
		uint32_t word_offset = self->database[i].word_offset;
		uint32_t word_length = self->database[i].word_length;
		if (word_length == 0) {
			fprintf(stderr, "Invalid word length: %lu\n",
				(unsigned long) word_length);
			err = 2;
			goto out_words;
		}
		if (!is_in_word_bounds(self, word_offset, word_length)) {
			fprintf(stderr, "Invalid position in the words file (%u +%u).\n",
				word_offset, word_length);
			err = 1;
			goto out_words;
		}
		p = &all_words[word_offset];
		word = malloc((word_length + 1) * sizeof(*word));
		memcpy(word, p, word_length * sizeof(*word));
		word[word_length] = 0;
		words[i] = word;
	}

	self->words = words;
out_all_words:
	free(all_words);
out:
	return err;

out_words:
	free_words(words, i);
	goto out_all_words;
}

static void free_words(char **words, size_t num_words)
{
	size_t i;

	for (i = 0; i < num_words; i++)
		free(words[i]);
	memset(words, 0, num_words * sizeof(*words));
	free(words);
}

static char * build_all_words(struct dictionary_reader *dict, size_t word_file_size)
{
	char *all_words;
	size_t num_read;

	all_words = malloc(word_file_size);
	if (all_words == NULL) {
		fprintf(stderr, "Could not allocate memory for all words\n");
		goto out;
	}

	num_read = fread(all_words, 1, word_file_size, dict->files.word_file);
	if (num_read != word_file_size) {
		fprintf(stderr, "Tried reading %lu entries from the word file, managed only %lu.",
			(unsigned long) word_file_size, (unsigned long) num_read);
		goto out_words;
	}

out:
	return all_words;

out_words:
	free(all_words);
	return NULL;
}
