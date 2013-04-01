#ifndef SERIES_H_
#define SERIES_H_

#ifdef __cplusplus
extern "C" {
#endif

void smoothify_series(const double *in, double *out, unsigned int size, unsigned int smoothing_window);

#ifdef __cplusplus
}
#endif

#endif /* SERIES_H_ */
