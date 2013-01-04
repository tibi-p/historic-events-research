#include <cstring>
#include <ctime>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#endif

#include "dictionary_writer.h"
#include "util.h"

#define CSV_DIRECTORY "data/csv/"

typedef int (*process_file_f)(const char *, struct dictionary_writer *);

char * process_line(char *line, struct dictionary_writer *dict)
{
	char *word, *p;
	uint64_t match_count;
	int year, volume_count;

	word = strtok(line, "\t");
	p = strtok(NULL, "\t");
	year = atoi(p);
	p = strtok(NULL, "\t");
	match_count = strtoull(p, NULL, 10);
	p = strtok(NULL, "\t");
	volume_count = atoi(p);
	update_dictionary(dict, word, year, match_count, (uint32_t) volume_count);

	return word;
}

int read_input(const char *in_filename, struct dictionary_writer *dict)
{
	FILE *f;
	int num_lines;
	int err = 0;
	char line[BUFFER_SIZE + 1];

	f = fopen(in_filename, "rt");
	if (f == NULL) {
		err = 1;
		goto out;
	}

	for (num_lines = 0; fgets(line, sizeof(line), f) != NULL; num_lines++)
		process_line(line, dict);

	printf("num_lines=%d\n", num_lines);
	printf("num_words=%lu\n", dict->num_words);

	fclose(f);
out:
	return err;
}

#ifdef _WIN32

void do_for_each_file(struct dictionary_writer *dict, process_file_f callback)
{
	WIN32_FIND_DATA find_data;
	HANDLE h;
	char *filename;
	const char *directory_name = CSV_DIRECTORY;
	const char *search_key = CSV_DIRECTORY "google*";

	h = FindFirstFile(search_key, &find_data);
	if (h != INVALID_HANDLE_VALUE) {
		do {
			filename = concatenate(directory_name, find_data.cFileName);
			if (filename != NULL) {
				printf("filename: %s\n", filename);
				callback(filename, dict);
				free(filename);
			} else {
				fprintf(stderr, "Could not allocate filename\n");
			}
		} while (FindNextFile(h, &find_data));
		if (!FindClose(h))
			fprintf(stderr, "Could not close the file search handle\n");
	} else {
		fprintf(stderr, "Could not find files matching the given pattern\n");
	}
}

#else

int do_for_each_file(struct dictionary_writer *dict, process_file_f callback)
{
	DIR *dirp;
	struct dirent *dp;
	char *filename;
	int err = 0;
	const char *directory_name = CSV_DIRECTORY;
	const char *search_key = "google";
	const size_t key_length = strlen(search_key);

	dirp = opendir(directory_name);
	if (dirp == NULL) {
		fprintf(stderr, "Could not open directory: %s\n", directory_name);
		err = 1;
		goto out;
	}

	while ((dp = readdir(dirp)) != NULL) {
		filename = dp->d_name;
		if (strncmp(filename, search_key, key_length) == 0) {
			filename = concatenate(directory_name, filename);
			if (filename != NULL) {
				printf("filename: %s\n", filename);
				callback(filename, dict);
				free(filename);
			} else {
				fprintf(stderr, "Could not allocate filename\n");
			}
		}
	}

	if (closedir(dirp) != 0) {
		fprintf(stderr, "Could not close directory: %s\n", directory_name);
	}
out:
	return err;
}

#endif

int run_program()
{
	struct dictionary_writer dict;
	int err;

	err = init_dictionary(&dict, "googlebooks-eng-all-1gram-20120701-database");
	if (err != 0)
		goto out;

	do_for_each_file(&dict, read_input);
	flush_dictionary(&dict);

	destroy_dictionary(&dict);

out:
	return err;
}

int main()
{
	clock_t cs, ce;
	int err;

	cs = clock();
	err = run_program();
	ce = clock();
	printf("%f seconds\n", (float) (ce - cs) / CLOCKS_PER_SEC);

	return err;
}
