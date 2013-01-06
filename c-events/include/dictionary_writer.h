#ifndef DICTIONARY_WRITER_H_
#define DICTIONARY_WRITER_H_

#include <stdio.h>
#include "dictionary_files.h"
#include "dictionary_types.h"

#define BUFFER_SIZE 1008
#define MAX_YEARS 520

#ifdef __cplusplus
extern "C" {
#endif

struct dictionary_writer {
	struct dictionary_files files;
	char last_word[BUFFER_SIZE + 1];
	struct db_entry current_entry;
	struct time_entry table[MAX_YEARS];
	size_t table_size;
	size_t num_words;
};

int init_dictionary(struct dictionary_writer *dict, const char *base_filename);
void destroy_dictionary(struct dictionary_writer *dict);
size_t flush_dictionary(struct dictionary_writer *dict);
void update_dictionary(struct dictionary_writer *dict, char *word,
	int year, uint64_t match_count, uint32_t volume_count);

#ifdef __cplusplus
}
#endif

#endif /* DICTIONARY_WRITER_H_ */
