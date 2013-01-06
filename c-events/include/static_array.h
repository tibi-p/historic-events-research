#ifndef STATIC_ARRAY_H_
#define STATIC_ARRAY_H_

#define STATIC_ARRAY_SIZE 513

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

struct range_entry {
	size_t left, right;
	double slope;
};

struct static_array {
	size_t size;
	struct range_entry array[STATIC_ARRAY_SIZE];
};

void sa_init(struct static_array *self);

void sa_append(struct static_array *self, size_t left, size_t right, double slope);

#ifdef __cplusplus
}
#endif

#endif /* STATIC_ARRAY_H_ */
