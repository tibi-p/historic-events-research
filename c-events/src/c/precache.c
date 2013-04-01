#include <stdio.h>

#define TIME_FILENAME "data/sort/googlebooks-eng-all-1gram-20120701-database.time"
#define BUFFER_SIZE (1 << 16)

int precache_file(const char *filename)
{
	FILE *f;
	unsigned long long s;
	size_t i, num_read, total_read;
	int err = 0;
	unsigned char buffer[BUFFER_SIZE];

	f = fopen(filename, "rb");
	if (f == NULL) {
		fprintf(stderr, "Cannot open: %s\n", filename);
		err = 1;
		goto out;
	}

	total_read = 0;
	s = 0;
	for (;;) {
		num_read = fread(buffer, 1, sizeof(buffer), f);
		total_read += num_read;
		if (num_read != sizeof(buffer))
			break;
		for (i = 0; i < num_read; i++)
			s += buffer[i];
	}
	printf("s=%llu\n", s);

	fclose(f);
out:
	return err;
}

int main()
{
	precache_file(TIME_FILENAME);
	return 0;
}
