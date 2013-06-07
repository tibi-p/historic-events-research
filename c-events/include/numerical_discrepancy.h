#ifndef NUMERICAL_DISCREPANCY_H_
#define NUMERICAL_DISCREPANCY_H_

#include <vector>
#include <cstring>
#include "file.h"
#include "generic_processor.h"

void fit_discrepancy(const double *series, unsigned int smoothing_window,
	std::vector< std::pair< std::pair<size_t, size_t>, int > > &intervals);

class numerical_discrepancy_processor : public generic_processor {

public:

	numerical_discrepancy_processor(double *series, const char *filename);

	virtual ~numerical_discrepancy_processor();

	virtual void compute_relevance(const char *word);

	virtual void compute_summary(const char *word);

	static numerical_discrepancy_processor * create(double *series, const char *filename);

private:
	double *series;
	excl_file efile;

};

#endif /* NUMERICAL_DISCREPANCY_H_ */
