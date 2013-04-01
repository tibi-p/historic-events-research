#ifndef GAUSSIAN_FINDER_H_
#define GAUSSIAN_FINDER_H_

#include <vector>
#include <cmath>
#include <cstdlib>

#define EPSILON 1e-6

struct gaussian_entry {

	size_t left, right;
	double mean, sigma;
	double distance, increase;

	gaussian_entry(size_t left, size_t right, double mean, double sigma, double distance, double increase) :
		left(left), right(right), mean(mean), sigma(sigma), distance(distance), increase(increase) { }

	bool operator<(const gaussian_entry &other) const
	{
		if (fabs(distance - other.distance) >= EPSILON)
			return distance < other.distance;
		if (fabs(increase - other.increase) >= EPSILON)
			return increase > other.increase;
		if (left != other.left)
			return left < other.left;
		if (right != other.right)
			return right < other.right;
		if (fabs(mean - other.mean) >= EPSILON)
			return mean < other.mean;
		if (fabs(sigma - other.sigma) >= EPSILON)
			return sigma < other.sigma;
		return false;
	}

};

void select_gaussians(const double *series, size_t inf, size_t sup, std::vector<gaussian_entry> &gaussians);

void relevant_gaussians(const std::vector<gaussian_entry> &gaussians,
	std::vector< std::pair<size_t, int> > &counts, double widening);

#endif /* GAUSSIAN_FINDER_H_ */
