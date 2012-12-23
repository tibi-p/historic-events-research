#include <cstdlib>
#include <cstring>
#include <ctime>
#include "dictionary_reader.h"
#include "dictionary_types.h"
#include "util.h"

#define BUFFER_SIZE 1008
#define MAX_ENTRIES (1 << 20)

struct db_entry database[MAX_ENTRIES];
struct indexed_word words[MAX_ENTRIES];

int sort_binary_data()
{
	struct dictionary_reader dictreader;
	struct dictionary_files out_files;
	size_t num_read, num_written;
	size_t good_words = 0;
	uint32_t wr_toffset = 0;
	uint32_t wr_woffset = 0;
	int err = 0;

	err = init_dictreader(&dictreader, "unsorted_data/googlebooks-eng-all-1gram-20120701-database");
	if (err != 0) {
		fprintf(stderr, "Could not init the dictionary reader.\n");
		goto out;
	}

	err = init_dictfiles(&out_files, "tmp/googlebooks-eng-all-1gram-20120701-database", "wb");
	if (err != 0) {
		fprintf(stderr, "Could not init the output files.\n");
		goto out_reader;
	}

	printf("num_words=%lu\n", dictreader.num_words);

	num_read = fread(database, sizeof(*database), dictreader.num_words, dictreader.files.main_file);
	if (num_read != dictreader.num_words) {
		fprintf(stderr, "Tried reading %lu entries from the main file, managed only %lu.",
			(unsigned long) dictreader.num_words, (unsigned long) num_read);
		err = 1;
		goto out_files;
	}

	for (size_t i = 0; i < dictreader.num_words; i++) {
		uint32_t word_offset = database[i].word_offset;
		uint32_t word_length = database[i].word_length;
		if (!is_in_word_bounds(&dictreader, word_offset, word_length)) {
			fprintf(stderr, "Invalid position in words file (%u +%u).\n",
				word_offset, word_length);
			exit(EXIT_FAILURE);
		}
		char *p = &dictreader.all_words[word_offset];
		char *word = (char *) malloc((word_length + 1) * sizeof(*word));
		memcpy(word, p, word_length * sizeof(*word));
		word[word_length] = 0;
		words[i].index = i;
		words[i].word = word;
	}

	qsort(words, dictreader.num_words, sizeof(*words), iw_compare);

	for (size_t i = 0; i < dictreader.num_words; i++) {
		struct time_entry table[MAX_YEARS];
		const char *word = words[i].word;
		size_t index = words[i].index;
		struct db_entry *current_entry = &database[index];

		uint32_t toffset = current_entry->time_offset;
		uint32_t tlength = current_entry->time_length;
		if (fseek(dictreader.files.time_file, toffset, SEEK_SET) != 0)
			exit(EXIT_FAILURE);

		uint32_t wlength = strlen(word);

		num_read = fread(table, sizeof(*table), tlength, dictreader.files.time_file);
		if (num_read != tlength)
			exit(EXIT_FAILURE);

		current_entry->word_offset = wr_woffset;
		current_entry->time_offset = wr_toffset;
		fwrite(current_entry, sizeof(*current_entry), 1, out_files.main_file);
		fwrite(word, sizeof(*word), wlength, out_files.word_file);
		num_written = fwrite(table, sizeof(*table), tlength, out_files.time_file);
		if (num_written != tlength) {
			fprintf(stderr, "Tried writing %u entries in the time file, but managed only %lu.",
				tlength, (unsigned long) num_written);
			exit(EXIT_FAILURE);
		}

		wr_toffset += tlength * sizeof(*table);
		wr_woffset += wlength;
		if (i % 2000 == 0)
			printf("Iteration: %lu\n", (unsigned long) i);
	}
	printf("good_words=%lu\n", good_words);

out_files:
	destroy_dictfiles(&out_files);
out_reader:
	destroy_dictreader(&dictreader);
out:
	return err;
}

int main()
{
	int err;

	err = sort_binary_data();

	return err;
}
