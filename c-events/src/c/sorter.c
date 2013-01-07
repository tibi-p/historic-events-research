#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "dictionary_reader.h"
#include "dictionary_types.h"
#include "util.h"

#define MAX_ENTRIES (1 << 20)

struct indexed_word words[MAX_ENTRIES];

int sort_binary_data()
{
	struct time_entry table[MAX_YEARS];
	const char *word;
	struct db_entry *entry;
	struct dictionary_reader dictreader;
	struct dictionary_files out_files;
	size_t tlength, wlength;
	size_t num_written;
	size_t i, index;
	uint32_t wr_toffset = 0;
	uint32_t wr_woffset = 0;
	int err = 0;

	err = init_dictreader(&dictreader, "data/raw/googlebooks-eng-all-1gram-20120701-database");
	if (err != 0) {
		fprintf(stderr, "Could not init the dictionary reader.\n");
		goto out;
	}

	err = init_dictfiles(&out_files, "data/temp/googlebooks-eng-all-1gram-20120701-database", "wb");
	if (err != 0) {
		fprintf(stderr, "Could not init the output files.\n");
		goto out_reader;
	}

	printf("num_words=%lu\n", dictreader.num_words);

	for (i = 0; i < dictreader.num_words; i++) {
		words[i].index = i;
		words[i].word = dictreader.words[i];
	}

	qsort(words, dictreader.num_words, sizeof(*words), iw_compare);

	for (i = 0; i < dictreader.num_words; i++) {
		word = words[i].word;
		index = words[i].index;

		err = read_table(&dictreader, index, table, &tlength);
		if (err != 0)
			goto out_files;

		entry = &dictreader.database[index];
		wlength = strlen(word);

		entry->word_offset = wr_woffset;
		entry->time_offset = wr_toffset;
		fwrite(entry, sizeof(*entry), 1, out_files.main_file);
		fwrite(word, sizeof(*word), wlength, out_files.word_file);
		num_written = fwrite(table, sizeof(*table), tlength, out_files.time_file);
		if (num_written != tlength) {
			fprintf(stderr, "Tried writing %lu entries in the time file, but managed only %lu.",
				(unsigned long) tlength, (unsigned long) num_written);
			err = 2;
			goto out_files;
		}

		wr_toffset += tlength * sizeof(*table);
		wr_woffset += wlength;
		if (i % 10000 == 0)
			printf("%lf percent done\n", (double) (100 * i) / dictreader.num_words);
	}

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
