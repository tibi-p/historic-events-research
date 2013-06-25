#include "kleinberg.h"
#include <algorithm>
#include <limits>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include "dictionary_reader.h"

using namespace std;

vector<double> ln_sums;

void init_ln_sums(size_t max_num_docs)
{
	ln_sums.push_back(0.0);
	for (size_t i = 1; i <= max_num_docs; i++)
		ln_sums.push_back(ln_sums.back() + log((double) i));
}

template<class T>
size_t find_min_position(const vector<T> &v)
{
	typename vector<T>::difference_type diff = min_element(v.begin(), v.end()) - v.begin();
	return (size_t) diff;
}

bool batch_viterbi(const vector<unsigned int> &docs, const vector<unsigned int> &relevant,
	vector<size_t> &hidden_states)
{
	if (docs.empty() || docs.size() != relevant.size())
		return false;

	const double s = 2.0;
	const double gamma = 1.0;
	size_t n = docs.size();
	vector<double> dp[2];
	vector<double> *prev = &dp[0];
	vector<double> *next = &dp[1];
	vector< vector<size_t> > psi(n);

	unsigned int total_docs = 0, total_relevant = 0;
	for (vector<unsigned int>::const_iterator it = docs.begin(); it != docs.end(); ++it)
		total_docs += *it;
	for (vector<unsigned int>::const_iterator it = relevant.begin(); it != relevant.end(); ++it)
		total_relevant += *it;

	if (total_relevant == total_docs)
		return false;

	vector<double> alphas;
	alphas.push_back((double) total_relevant / total_docs);
	while (alphas.back() * s <= 1.0)
		alphas.push_back(alphas.back() * s);
	size_t num_states = alphas.size();

	for (size_t i = 0; i < sizeof(dp) / sizeof(*dp); i++)
		dp[i].resize(num_states);

	(*prev)[0] = 0.0;
	for (size_t i = 1; i < prev->size(); i++)
		(*prev)[i] = numeric_limits<double>::max();

	for (size_t i = 0; i < n; i++) {
		unsigned int r = relevant[i];
		unsigned int nr = docs[i] - r;
		double choose = ln_sums[r] + ln_sums[nr] - ln_sums[docs[i]];
		for (size_t j = 0; j < num_states; j++) {
			double min_value = numeric_limits<double>::max();
			size_t min_index = numeric_limits<size_t>::max();
			for (size_t k = 0; k < num_states; k++) {
				double cand = (*prev)[k];
				if (cand != numeric_limits<double>::max()) {
					if (j > k)
						cand += double(j - k) * gamma * log(n);
					if (cand < min_value) {
						min_value = cand;
						min_index = k;
					}
				}
			}
			if (min_index == numeric_limits<size_t>::max()) {
				fprintf(stderr, "Could not a minimal index for batch Viterbi\n");
				exit(EXIT_FAILURE);
			}
			if (min_value != numeric_limits<double>::max())
				(*next)[j] = min_value + choose - r * log(alphas[j]) - nr * log(1 - alphas[j]);
			else
				(*next)[j] = numeric_limits<double>::max();
			psi[i].push_back(min_index);
		}
		swap(prev, next);
	}

	size_t d = find_min_position(*prev);
	hidden_states.push_back(d);
	for (size_t i = n; i > 0;) {
		--i;
		d = psi[i][d];
		hidden_states.push_back(d);
	}
	reverse(hidden_states.begin(), hidden_states.end());

	return true;
}

kleinberg_processor::kleinberg_processor(vector<unsigned int> &docs,
	vector<unsigned int> &relevant, const char *filename)
	: docs(docs), relevant(relevant), efile(filename) { }

kleinberg_processor::~kleinberg_processor() { }

void kleinberg_processor::compute_relevance(const char *word)
{
	vector<size_t> hidden_states;
	int counts[MAX_YEARS];

	batch_viterbi(docs, relevant, hidden_states);

	const size_t num_elems = min<size_t>(hidden_states.size(), MAX_YEARS);
	for (size_t i = 0; i < num_elems; i++)
		counts[i] = hidden_states[i];
	fill(counts + num_elems, counts + MAX_YEARS, 0);

	print_relevance_csv(efile.f, word, counts, MAX_YEARS);
}

void kleinberg_processor::compute_summary(const char *word)
{
	vector<size_t> hidden_states;

	batch_viterbi(docs, relevant, hidden_states);

	const size_t num_elems = min<size_t>(hidden_states.size(), MAX_YEARS);
	for (size_t i = 0; i < num_elems; i++) {
		int score = hidden_states[i];
		if (score > 0)
			print_summary_txt(efile.f, word, (unsigned int) i, score);
	}
}

kleinberg_processor * kleinberg_processor::create(vector<unsigned int> &docs,
	vector<unsigned int> &relevant, const char *filename)
{
	try {
		return new kleinberg_processor(docs, relevant, filename);
	} catch (file_exception &fe) {
		return NULL;
	}
}
