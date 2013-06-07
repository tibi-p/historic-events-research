#ifndef KLEINBERG_H_
#define KLEINBERG_H_

#include <vector>
#include <cstring>

void init_ln_sums(size_t max_num_docs);
bool batch_viterbi(const std::vector<unsigned int> &docs, const std::vector<unsigned int> &relevant,
	std::vector<size_t> &hidden_states);

#endif /* KLEINBERG_H_ */
