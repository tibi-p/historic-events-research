#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "dictionary_reader.h"

#define MAX_NUM_WORDS 250000
#define BUFFER_SIZE 5000

using namespace std;

int counts[MAX_YEARS][MAX_NUM_WORDS];
int year_sum[MAX_YEARS];
float relevance[MAX_YEARS][MAX_NUM_WORDS];
float dist[2 * MAX_YEARS - 1][2 * MAX_YEARS - 1];
int parent[2 * MAX_YEARS - 1];

bool entry_compare(const pair<float, int> &x, const pair<float, int> &y)
{
	if (x.first != y.first)
		return x.first > y.first;
	return x.second < y.second;
}

static int square(int x)
{
	return x * x;
}

float cosine_similarity(int x, int y, int num_words)
{
	float sum = 0.0f;
	for (int j = 0; j < num_words; j++)
		sum += relevance[x][j] * relevance[y][j];
	return sum;
}

bool multi_erase(multimap<float, int> &mm, float key, int value)
{
	multimap<float, int>::iterator it = mm.find(key);
	while (it != mm.end() && it->first == key && it->second != value) {
		++it;
	}
	if (it != mm.end() && it->first == key && it->second == value) {
		mm.erase(it);
		return true;
	} else {
		return false;
	}
}

void process_relevance(const char *in_filename, const char *out_filename)
{
	FILE *f;
	char *p;
	int num_words;
	vector<int> year_index[MAX_YEARS];
	vector< pair<float, int> > year_reverse[MAX_YEARS];
	char str[BUFFER_SIZE + 2];

	f = fopen(in_filename, "rt");
	if (f == NULL)
		return;

	num_words = 0;
	while (fgets(str, BUFFER_SIZE + 2, f) != NULL) {
		int i = 0;
		p = strtok(str, ",");
		while (p != NULL) {
			counts[i][num_words] = atoi(p);
			p = strtok(NULL, ",");
			i++;
		}
		num_words++;
	}

	fclose(f);

	for (int i = 0; i < MAX_YEARS; i++) {
		int sum = 0;
		for (int j = 0; j < num_words; j++)
			sum += square(counts[i][j]);
		year_sum[i] = sum;
		float length = sqrt(sum);
		for (int j = 0; j < num_words; j++)
			relevance[i][j] = (float) counts[i][j] / length;
	}

	for (int i = 0; i < MAX_YEARS; i++) {
		for (int j = 0; j < num_words; j++)
			if (counts[i][j] > 0) {
				year_index[i].push_back(j);
				year_reverse[i].push_back(pair<float, int>(relevance[i][j], j));
			}
		sort(year_reverse[i].begin(), year_reverse[i].end(), entry_compare);
		//printf("year=%d length=%zu\n", 1500 + i, year_reverse[i].size());
	}

	fill(parent, parent + 2 * MAX_YEARS - 1, -1);

	set<int> relevant_years;
	for (int i = 0; i < MAX_YEARS; i++)
		if (year_sum[i] > 0)
			relevant_years.insert(i);
	//printf("# relevant years: %zu\n", relevant_years.size());

	for (int i = 0; i < 2 * MAX_YEARS - 1; i++)
		for (int j = 0; j < 2 * MAX_YEARS - 1; j++)
			dist[i][j] = 1.0f;

	set<int> nodes;
	for (int j = 0; j < num_words; j++)
		nodes.insert(j);

	multimap<float, int> pq[2 * MAX_YEARS - 1];

	for (int i = 0; i < MAX_YEARS; i++)
		if (year_sum[i] > 0) {
			for (int j = i + 1; j < MAX_YEARS; j++)
				if (year_sum[j] > 0) {
					float sim = cosine_similarity(i, j, num_words);
					sim = max(min(sim, 1.0f), 0.0f);
					dist[i][j] = 1 - sim;
					pq[i].insert(pair<float, int>(dist[i][j], j));
				}
		}

	int k = MAX_YEARS;
	for (; ; k++) {
		int mx = -1, my = -1;
		float min_dist = numeric_limits<float>::max();
		for (int i = 0; i < k; i++)
			if (!pq[i].empty()) {
				multimap<float, int>::const_iterator it = pq[i].begin();
				float cand_dist = it->first;
				if (cand_dist < min_dist) {
					min_dist = cand_dist;
					mx = i;
					my = it->second;
				}
			}
		//fprintf(stderr, "k=%d (%d, %d) %f\n", k - MAX_YEARS, mx, my, min_dist);
		if (mx == -1 || my == -1)
			break;

		parent[mx] = k;
		parent[my] = k;
		for (int i = 0; i < k; i++)
			if (parent[i] < 0) {
#if 0
				float new_dist = numeric_limits<float>::max();
				if (i < mx)
					new_dist = min(new_dist, dist[i][mx]);
				else
					new_dist = min(new_dist, dist[mx][i]);
				if (i < my)
					new_dist = min(new_dist, dist[i][my]);
				else
					new_dist = min(new_dist, dist[my][i]);
#else
				float new_dist = -numeric_limits<float>::max();
				if (i < mx)
					new_dist = max(new_dist, dist[i][mx]);
				else
					new_dist = max(new_dist, dist[mx][i]);
				if (i < my)
					new_dist = max(new_dist, dist[i][my]);
				else
					new_dist = max(new_dist, dist[my][i]);
#endif
				if (new_dist < 1.0f) {
					dist[i][k] = new_dist;
					pq[i].insert(pair<float, int>(new_dist, k));
				}
			}

		pq[mx].clear();
		for (int i = 0; i < mx; i++)
			multi_erase(pq[i], dist[i][mx], mx);
		pq[my].clear();
		for (int i = 0; i < my; i++)
			multi_erase(pq[i], dist[i][my], my);
	}

	f = fopen(out_filename, "wt");
	if (f == NULL)
		return;

	for (int i = 0; i < k; i++)
		fprintf(f, "%d,%d\n", i, parent[i]);

	fclose(f);
}

int main()
{
	printf("%zu KB\n", sizeof(relevance) >> 10);
	process_relevance("data/relevance/double_change.csv", "data/clusters/double_change.csv");
	process_relevance("data/relevance/linear_model.csv", "data/clusters/linear_model.csv");
	process_relevance("data/relevance/gaussian_model.csv", "data/clusters/gaussian_model.csv");
	return 0;
}
