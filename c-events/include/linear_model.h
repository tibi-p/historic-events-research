#ifndef LINEAR_MODEL_H_
#define LINEAR_MODEL_H_

#include "static_array.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <gsl/gsl_multimin.h>
#include <gsl/gsl_statistics_double.h>

struct static_range {
	size_t begin, end;
	size_t year_offset;
	double *array;
	size_t size;
};

void generate_ranges(struct static_array *ranges,
	const gsl_multimin_fdfminimizer_type *T,
	gsl_multimin_function_fdf *fdf);

void normalize_standard_score(double *data, size_t num_elems);

void normalize_generate_ranges(struct static_array *ranges,
	const gsl_multimin_fdfminimizer_type *T,
	gsl_multimin_function_fdf *fdf);

double regression_f(const gsl_vector *v, void *params);

void regression_df(const gsl_vector *v, void *params, gsl_vector *df);

void regression_fdf(const gsl_vector *v, void *params, double *f, gsl_vector *df);

#ifdef __cplusplus
}
#endif

#endif /* LINEAR_MODEL_H_ */
