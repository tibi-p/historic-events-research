#ifndef KLEINBERG_H_
#define KLEINBERG_H_

#include <vector>
#include <cstring>
#include "file.h"
#include "generic_processor.h"

void init_ln_sums(size_t max_num_docs);
bool batch_viterbi(const std::vector<unsigned int> &docs, const std::vector<unsigned int> &relevant,
	std::vector<size_t> &hidden_states);

class kleinberg_processor : public generic_processor {

public:

	kleinberg_processor(std::vector<unsigned int> &docs, std::vector<unsigned int> &relevant,
		const char *filename);

	virtual ~kleinberg_processor();

	virtual void compute_relevance(const char *word);

	virtual void compute_summary(const char *word);

	static kleinberg_processor * create(std::vector<unsigned int> &docs,
		std::vector<unsigned int> &relevant, const char *filename);

private:
	std::vector<unsigned int> &docs;
	std::vector<unsigned int> &relevant;
	excl_file efile;

};

#endif /* KLEINBERG_H_ */
