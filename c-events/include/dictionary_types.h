#ifndef DICTIONARY_TYPES_H_
#define DICTIONARY_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MIN_YEAR 1500

struct db_entry {
	uint32_t word_offset;
	uint32_t time_offset;
	uint16_t word_length;
	uint16_t time_length;
	uint64_t total_match_count;
	uint64_t total_volume_count;
};

struct time_entry {
	uint64_t match_count;
	uint32_t volume_count;
	uint16_t year;
};

struct series_entry {
	double match_count;
	uint32_t volume_count;
};

struct total_counts_entry {
	uint64_t match_count;
	uint32_t page_count;
	uint32_t volume_count;
};

#ifdef __cplusplus
}
#endif

#endif /* DICTIONARY_TYPES_H_ */
