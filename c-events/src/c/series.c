#include "series.h"

#define SIGNIFICANT_RATIO 150.0

static double get_significant_value(double value, double max_value)
{
	if (value >= max_value / SIGNIFICANT_RATIO)
		return value;
	else
		return 0.0;
}

void smoothify_series(const double *in, double *out, unsigned int size, unsigned int smoothing_window)
{
	double max_value = 0.0;
	double smoothing_sum = 0.0;
	unsigned int window_size = 0;
	unsigned int i;

	for (i = 0; i < size; i++)
		if (in[i] > max_value)
			max_value = in[i];

	for (i = 0; i < size + smoothing_window; i++) {
		if (i < size) {
			smoothing_sum += get_significant_value(in[i], max_value);
			window_size++;
		}
		if (i >= 2 * smoothing_window + 1) {
			smoothing_sum -= get_significant_value(in[i - 2 * smoothing_window - 1], max_value);
			window_size--;
		}
		if (window_size == 2 * smoothing_window + 1) {
			if (smoothing_sum < 0.0)
				smoothing_sum = 0.0;
			if (i >= smoothing_window) {
				unsigned int pos = i - smoothing_window;
				out[pos] = smoothing_sum / window_size;
			}
		}
	}
}
