#include <stdlib.h>
#include <string.h>
#include "dictionary_writer.h"
#include "util.h"

#define MIN_MATCH_COUNT (1 << 14)

int init_dictionary(struct dictionary_writer *dict, const char *base_filename)
{
	int err = 0;

	memset(dict, 0, sizeof(*dict));

	err = init_dictfiles(&dict->files, base_filename, "wb");
	return err;
}

void destroy_dictionary(struct dictionary_writer *dict)
{
	destroy_dictfiles(&dict->files);
}

size_t flush_dictionary(struct dictionary_writer *dict)
{
	size_t num_written, word_len;

	if (dict->current_entry.total_match_count < MIN_MATCH_COUNT)
		return 0;
	//printf("%s: %u\n", dict->last_word, dict->current_entry.total_match_count);

	word_len = strlen(dict->last_word);
	dict->current_entry.word_length = word_len;
	dict->current_entry.time_length = dict->table_size;

	num_written = fwrite(&dict->current_entry, sizeof(dict->current_entry), 1, dict->files.main_file);
	if (num_written != 1) {
		if (ferror(dict->files.main_file)) {
			fprintf(stderr, "Could not write to the main file\n");
			exit(EXIT_FAILURE);
		}
	}

	num_written = fwrite(dict->last_word, sizeof(*dict->last_word), word_len, dict->files.word_file);
	if (num_written != word_len) {
		if (ferror(dict->files.word_file)) {
			fprintf(stderr, "Could not write to the word file\n");
			exit(EXIT_FAILURE);
		}
	}

	num_written = fwrite(dict->table, sizeof(*dict->table), dict->table_size, dict->files.time_file);
	if (num_written != dict->table_size) {
		if (ferror(dict->files.time_file)) {
			fprintf(stderr, "Could not write to the time file\n");
			exit(EXIT_FAILURE);
		}
	}

	return word_len;
}

void update_dictionary(struct dictionary_writer *dict, char *word,
	int year, uint64_t match_count, uint32_t volume_count)
{
	size_t word_len;

	if (strcmp(dict->last_word, word) != 0) {
		word_len = flush_dictionary(dict);
		if (word_len > 0) {
			dict->current_entry.word_offset += word_len;
			dict->current_entry.time_offset += dict->table_size * sizeof(*dict->table);
			dict->num_words++;
		}
		dict->current_entry.total_match_count = match_count;
		dict->current_entry.total_volume_count = volume_count;
		strcpy(dict->last_word, word);
		dict->table_size = 0;
	} else {
		dict->current_entry.total_match_count += match_count;
		dict->current_entry.total_volume_count += volume_count;
	}
	dict->table[dict->table_size].year = year;
	dict->table[dict->table_size].match_count = match_count;
	dict->table[dict->table_size].volume_count = volume_count;
	dict->table_size++;
}
