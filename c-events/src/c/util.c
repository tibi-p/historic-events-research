#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "util.h"

char * concatenate(const char *left, const char *right)
{
	char *s;
	size_t left_length, right_length;

	left_length = strlen(left);
	right_length = strlen(right);
	s = malloc((left_length + right_length + 1) * sizeof(*s));
	if (s != NULL) {
		memcpy(s, left, left_length * sizeof(*s));
		memcpy(s + left_length, right, right_length * sizeof(*s));
		s[left_length + right_length] = 0;
	}

	return s;
}

long get_file_size(FILE *f)
{
	long file_size;

	if (fseek(f, 0, SEEK_END) != 0)
		return -1L;

	file_size = ftell(f);
	if (file_size < 0)
		return file_size;

	if (fseek(f, 0, SEEK_SET) != 0)
		return -1L;

	return file_size;
}

long long get_file_size64(FILE *f)
{
	long long file_size;

	if (fseek64(f, 0, SEEK_END) != 0)
		return -1LL;

	file_size = ftell64(f);
	if (file_size < 0)
		return file_size;

	if (fseek64(f, 0, SEEK_SET) != 0)
		return -1LL;

	return file_size;
}

int file_exists(const char *filename)
{
	return access(filename, F_OK) == 0;
}
