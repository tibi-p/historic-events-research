#include <stdlib.h>
#include "dictionary_files.h"
#include "util.h"

int init_dictfiles(struct dictionary_files *files, const char *base_filename,
	const char *mode)
{
	char *main_filename = NULL;
	char *word_filename = NULL;
	char *time_filename = NULL;
	int err = 0;

	main_filename = concatenate(base_filename, ".main");
	if (main_filename == NULL) {
		err = 1;
		goto out;
	}

	word_filename = concatenate(base_filename, ".words");
	if (word_filename == NULL) {
		err = 1;
		goto out_filename;
	}

	time_filename = concatenate(base_filename, ".time");
	if (time_filename == NULL) {
		err = 1;
		goto out_filename;
	}

	files->main_file = fopen(main_filename, mode);
	if (files->main_file == NULL) {
		err = 1;
		goto out_filename;
	}

	files->word_file = fopen(word_filename, mode);
	if (files->word_file == NULL) {
		err = 1;
		goto out_main_file;
	}

	files->time_file = fopen(time_filename, mode);
	if (files->time_file == NULL) {
		err = 1;
		goto out_word_file;
	}

out_filename:
	free(time_filename);
	free(word_filename);
	free(main_filename);
out:
	return err;

out_word_file:
	fclose(files->word_file);
out_main_file:
	fclose(files->main_file);
	goto out_filename;
}

void destroy_dictfiles(struct dictionary_files *files)
{
	fclose(files->time_file);
	fclose(files->word_file);
	fclose(files->main_file);
}
