#!/usr/bin/pypy
from __future__ import print_function
import os

def is_valid_word(word):
	return len(word) > 0 and word[0].isalpha() and not '.' in word and not ',' in word

def filter_word_pairs(word_pairs):
	word_map = { }
	for (word, mult) in word_pairs:
		tokens = word.split('_')
		if len(tokens) >= 1:
			word = tokens[0]
			if word in word_map:
				count = word_map[word]
				if mult > count:
					word_map[word] = mult
			else:
				word_map[word] = mult
	return word_map.iteritems()

def parse_summary(filename):
	v = [ [ ] for _ in xrange(2012) ]
	with open(summary_filename) as f:
		for line in f:
			tokens = line.rstrip().split()
			if len(tokens) >= 2:
				word = tokens[0]
				if is_valid_word(word):
					year = int(tokens[1])
					if len(tokens) >= 3:
						mult = int(tokens[2])
					else:
						mult = 1
					v[year].append((word, mult))

	for year, word_pairs in enumerate(v):
		words = [ ]
		for word, mult in filter_word_pairs(word_pairs):
			for _ in xrange(mult):
				words.append(word)
		v[year] = words

	return v

directory = 'data/zeitgeist'
summary_filename = os.path.join(directory, 'summary.txt')

with open('data/zeitgeist/history.csv', 'w') as f:
	v = parse_summary(summary_filename)
	id = 0
	line_fmt = '%d,%d,year %d,%s'
	for year, words in enumerate(v):
		if len(words) > 0:
			id += 1
			print(line_fmt % (id, year, year, ' '.join(words)), file=f)
