#include <stdlib.h>
#include <string.h>
#include "dictionary_reader.h"
#include "dictionary_types.h"
#include "util.h"

#define BUFFER_SIZE 11000

static int load_frequencies(struct total_counts_entry *frequencies,
	const char *filename);

int init_dictreader(struct dictionary_reader *dict, const char *base_filename)
{
	size_t word_file_size;
	size_t num_read;
	long main_file_size;
	int err = 0;

	err = init_dictfiles(&dict->files, base_filename, "rb");
	if (err != 0) {
		goto out;
	}

	memset(dict->frequencies, 0, sizeof(dict->frequencies));
	err = load_frequencies(dict->frequencies, "googlebooks-eng-all-totalcounts-20120701.txt");
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

	dict->all_words = malloc(word_file_size);
	if (dict->all_words == NULL) {
		err = 1;
		goto out_files;
	}

	num_read = fread(dict->all_words, 1, word_file_size, dict->files.word_file);
	if (num_read != word_file_size) {
		err = 1;
		goto out_words;
	}

out:
	return err;

out_words:
	free(dict->all_words);
out_files:
	destroy_dictfiles(&dict->files);
	goto out;
}

void destroy_dictreader(struct dictionary_reader *dict)
{
	free(dict->all_words);
	destroy_dictfiles(&dict->files);
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
