#include "numerical_discrepancy.h"
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <cstring>
#include "dictionary_reader.h"

using namespace std;

pair<size_t, size_t> add_to_pair(pair<size_t, size_t> p, size_t offset)
{
	return make_pair(p.first + offset, p.second + offset);
}

void append_indices(const vector< pair<size_t, size_t> > &indices,
	vector< pair< pair<size_t, size_t>, int > > &intervals,
	size_t offset, int level)
{
	for (size_t i = 0; i < indices.size(); i++)
		intervals.push_back(make_pair(add_to_pair(indices[i], offset), level));
}

template<class T>
void compute_max_sequences(const T *v, size_t n, vector< pair<size_t, size_t> > &indices)
{
	vector<size_t> back_ptr;
	vector<T> sums;

	sums.push_back((T) 0);
	for (size_t i = 0; i < n; i++)
		sums.push_back(sums.back() + v[i]);

	size_t inf_index = 0;
	for (size_t i = 0; i < n; i++) {
		if (v[i] > 0) {
			size_t si = i;
			while (si + 1 < n && v[si + 1] > 0)
				++si;
			indices.push_back(make_pair(i, si));
			back_ptr.push_back(n);
			i = si;
			while (true) {
				T left = sums[indices.back().first];
				size_t j = back_ptr.back();
				bool found = (j < n);
				if (!found) {
					if (indices.size() >= inf_index + 2) {
						for (j = indices.size() - 2; j != n; j = back_ptr[j])
							if (sums[indices[j].first] < left) {
								found = true;
								back_ptr.back() = j;
								break;
							}
					}
				}
				if (found && sums[indices[j].second + 1] < sums[indices.back().second + 1]) {
					indices[j].second = i;
					indices.resize(j + 1);
					back_ptr.resize(j + 1);
				} else {
					if (!found) {
						inf_index = indices.size() - 1;
					}
					break;
				}
			}
		}
	}

#if 0
	for (size_t l = 0; l < indices.size(); l++) {
		printf("%zu - [%zu, %zu] => {%lf, %lf}\n", l, indices[l].first, indices[l].second,
			sums[indices[l].first], sums[indices[l].second + 1]);
	}
#endif
}

void analyze_burstiness(const double *series, size_t num_elems, vector< pair<size_t, size_t> > &indices)
{
	double burstiness[MAX_YEARS];
	double sum = 0.0;

	for (size_t i = 0; i < num_elems; i++)
		sum += series[i];

	for (size_t i = 0; i < num_elems; i++)
		burstiness[i] = series[i] / sum - 1. / num_elems;

	compute_max_sequences(burstiness, num_elems, indices);
}

void fit_discrepancy(const double *series, unsigned int smoothing_window,
	vector< pair< pair<size_t, size_t>, int > > &intervals)
{
	vector< pair<size_t, size_t> > indices;

	size_t inf = smoothing_window;
	size_t sup = MAX_YEARS - smoothing_window;
	analyze_burstiness(series + inf, sup - inf, indices);
	append_indices(indices, intervals, inf, 0);

	for (size_t i = 0; i < intervals.size();) {
		size_t diff = intervals[i].first.second - intervals[i].first.first + 1;
		if (diff >= 32) {
			inf = intervals[i].first.first;
			int level = intervals[i].second;
			intervals[i] = intervals.back();
			intervals.pop_back();

			indices.clear();
			analyze_burstiness(series + inf, diff, indices);
			append_indices(indices, intervals, inf, level + 1);
		} else {
			i++;
		}
	}
}

numerical_discrepancy_processor::numerical_discrepancy_processor(double *series, const char *filename)
	: series(series), efile(filename) { }

numerical_discrepancy_processor::~numerical_discrepancy_processor() { }

void numerical_discrepancy_processor::compute_relevance(const char *word)
{
	vector< pair< pair<size_t, size_t>, int > > intervals;
	int counts[MAX_YEARS];

	fit_discrepancy(series, 2, intervals);
	memset(counts, 0, sizeof(counts));
	for (size_t i = 0; i < intervals.size(); i++) {
		pair<size_t, size_t> interval = intervals[i].first;
		int score = 2 * intervals[i].second + 1;
		for (size_t j = interval.first; j <= interval.second; j++)
			counts[j] = score;
	}

	print_relevance_csv(efile.f, word, counts, MAX_YEARS);
}

void numerical_discrepancy_processor::compute_summary(const char *word)
{
	vector< pair< pair<size_t, size_t>, int > > intervals;

	fit_discrepancy(series, 2, intervals);
	sort(intervals.begin(), intervals.end());
	for (size_t i = 0; i < intervals.size(); i++) {
		pair<size_t, size_t> interval = intervals[i].first;
		int score = intervals[i].second / 2 + 1;
		for (size_t j = interval.first; j <= interval.second; j++)
			print_summary_txt(efile.f, word, (unsigned int) j, score);
	}
}

numerical_discrepancy_processor * numerical_discrepancy_processor::create(double *series, const char *filename)
{
	try {
		return new numerical_discrepancy_processor(series, filename);
	} catch (file_exception &fe) {
		return NULL;
	}
}
