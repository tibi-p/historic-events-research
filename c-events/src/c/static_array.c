#include "static_array.h"

void sa_init(struct static_array *self)
{
	self->size = 0;
}

void sa_append(struct static_array *self, size_t left, size_t right, double slope)
{
	if (self->size < STATIC_ARRAY_SIZE) {
		self->array[self->size].left = left;
		self->array[self->size].right = right;
		self->array[self->size].slope = slope;
		self->size++;
	} else {
		fprintf(stderr, "The static array is at full capacity\n");
	}
}
