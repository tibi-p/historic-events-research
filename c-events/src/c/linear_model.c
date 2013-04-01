#include "linear_model.h"

#define MAX_ITERATIONS 100

double compute_log_error(const gsl_multimin_fdfminimizer_type *T,
	gsl_multimin_function_fdf *fdf, gsl_vector *x)
{
	gsl_multimin_fdfminimizer *s;
	double f;
	size_t iter;
	int status;

	s = gsl_multimin_fdfminimizer_alloc(T, 2);
	gsl_multimin_fdfminimizer_set(s, fdf, x, 0.01, 1e-4);

	iter = 0;
	do {
		iter++;
		status = gsl_multimin_fdfminimizer_iterate(s);

		if (status)
			break;

		status = gsl_multimin_test_gradient(s->gradient, 1e-3);
	} while (status == GSL_CONTINUE && iter < MAX_ITERATIONS);

	gsl_vector_memcpy(x, s->x);
	f = s->f;
	gsl_multimin_fdfminimizer_free(s);

	return log(f);
}

void append_range(struct static_array *ranges,
	const struct static_range *training_data, const gsl_vector *x)
{
	size_t year_offset;
	size_t year_begin, year_end;

	year_offset = training_data->year_offset;
	year_begin = year_offset + training_data->begin;
	year_end = year_offset + training_data->end - 1;
	sa_append(ranges, year_begin, year_end, gsl_vector_get(x, 0));
}

void generate_ranges(struct static_array *ranges,
	const gsl_multimin_fdfminimizer_type *T,
	gsl_multimin_function_fdf *fdf)
{
	struct static_range *training_data;
	gsl_vector *x;
	double err;
	size_t i;

	training_data = fdf->params;
	x = gsl_vector_alloc(2);
	gsl_vector_set(x, 0, 0.0);
	gsl_vector_set(x, 1, 0.0);
	training_data->begin = 0;

	sa_init(ranges);

	for (i = 1; i < training_data->size; i++) {
		training_data->end = i;
		err = compute_log_error(T, fdf, x);
		if (err >= -5.0) {
			append_range(ranges, training_data, x);
			gsl_vector_set(x, 0, 0.0);
			gsl_vector_set(x, 1, 0.0);
			training_data->begin = i;
		}
	}
	append_range(ranges, training_data, x);

	gsl_vector_free(x);
}

void normalize_standard_score(double *data, size_t num_elems)
{
	double mean;
	double sigma;
	size_t i;

	mean = gsl_stats_mean(data, 1, num_elems);
	sigma = gsl_stats_sd_m(data, 1, num_elems, mean);

	for (i = 0; i < num_elems; i++)
		data[i] = (data[i] - mean) / sigma;
}

void normalize_generate_ranges(struct static_array *ranges,
	const gsl_multimin_fdfminimizer_type *T,
	gsl_multimin_function_fdf *fdf)
{
	const struct static_range *training_data;

	training_data = fdf->params;
	normalize_standard_score(training_data->array, training_data->size);
	generate_ranges(ranges, T, fdf);
}

static double range_get(const struct static_range *self, size_t index)
{
	double value;

	if (index >= self->begin && index <= self->end) {
		value = self->array[index];
	} else {
		fprintf(stderr, "range_get: the index is not within bounds\n");
		exit(EXIT_FAILURE);
	}

	return value;
}

double regression_f(const gsl_vector *v, void *params)
{
	struct static_range *training_data;
	double error;
	double x, y;
	double value, diff;
	size_t i, j;

	training_data = params;
	x = gsl_vector_get(v, 0);
	y = gsl_vector_get(v, 1);

	error = 0.0;
	for (i = training_data->begin; i <= training_data->end; i++) {
		j = i - training_data->begin;
		value = range_get(training_data, i);
		diff = x * j + y - value;
		error += diff * diff;
	}

	return error / 2;
}

/* The gradient of f, df = (df/dx, df/dy). */
void regression_df(const gsl_vector *v, void *params, gsl_vector *df)
{
	struct static_range *training_data;
	double derivative[2];
	double x, y;
	double value, diff;
	size_t i, j;

	training_data = params;
	x = gsl_vector_get(v, 0);
	y = gsl_vector_get(v, 1);

	for (i = 0; i < 2; i++)
		derivative[i] = 0.0;

	for (i = training_data->begin; i <= training_data->end; i++) {
		j = i - training_data->begin;
		value = range_get(training_data, i);
		diff = x * j + y - value;
		derivative[0] += diff * j;
		derivative[1] += diff;
	}

	for (i = 0; i < 2; i++)
		gsl_vector_set(df, i, derivative[i]);
}

/* Compute both f and df together. */
void regression_fdf(const gsl_vector *v, void *params, double *f, gsl_vector *df)
{
	struct static_range *training_data;
	double derivative[2];
	double error;
	double x, y;
	double value, diff;
	size_t i, j;

	training_data = params;
	x = gsl_vector_get(v, 0);
	y = gsl_vector_get(v, 1);

	error = 0.0;
	for (i = 0; i < 2; i++)
		derivative[i] = 0.0;

	for (i = training_data->begin; i <= training_data->end; i++) {
		j = i - training_data->begin;
		value = range_get(training_data, i);
		diff = x * j + y - value;
		error += diff * diff;
		derivative[0] += diff * j;
		derivative[1] += diff;
	}

	*f = error / 2;
	for (i = 0; i < 2; i++)
		gsl_vector_set(df, i, derivative[i]);
}
