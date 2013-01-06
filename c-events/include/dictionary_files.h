#ifndef DICTIONARY_FILES_H_
#define DICTIONARY_FILES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

struct dictionary_files {
	FILE *main_file;
	FILE *word_file;
	FILE *time_file;
};

int init_dictfiles(struct dictionary_files *files, const char *base_filename,
	const char *mode);
void destroy_dictfiles(struct dictionary_files *files);

#ifdef __cplusplus
}
#endif

#endif /* DICTIONARY_FILES_H_ */
