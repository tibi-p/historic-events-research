#ifndef NUMERICAL_DISCREPANCY_H_
#define NUMERICAL_DISCREPANCY_H_

#include <vector>
#include <cstring>

void fit_discrepancy(const double *series, unsigned int smoothing_window,
	std::vector< std::pair< std::pair<size_t, size_t>, int > > &intervals);

#endif /* NUMERICAL_DISCREPANCY_H_ */
