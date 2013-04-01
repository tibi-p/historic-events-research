#include "series.h"

void smoothify_series(const double *in, double *out, unsigned int size, unsigned int smoothing_window)
{
	double smoothing_sum = 0.0;
	unsigned int window_size = 0;
	unsigned int j;

	for (j = 0; j < size + smoothing_window; j++) {
		if (j < size) {
			smoothing_sum += in[j];
			window_size++;
		}
		if (j >= 2 * smoothing_window + 1) {
			smoothing_sum -= in[j - 2 * smoothing_window - 1];
			window_size--;
		}
		if (window_size == 2 * smoothing_window + 1) {
			if (smoothing_sum < 0.0)
				smoothing_sum = 0.0;
			if (j >= smoothing_window) {
				unsigned int pos = j - smoothing_window;
				out[pos] = (double) smoothing_sum / window_size;
			}
		}
	}
}
