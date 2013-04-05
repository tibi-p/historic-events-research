#!/usr/bin/pypy
from __future__ import print_function
import os
import sys

def is_valid_word(word):
	return len(word) > 0 and word[0].isalpha() and not '.' in word and not ',' in word

def filter_word_pairs(word_pairs):
	word_map = { }
	for (word, mult) in word_pairs:
		tokens = word.split('_')
		if len(tokens) == 1:
			word = tokens[0]
			if word in word_map:
				count = word_map[word]
				if mult > count:
					word_map[word] = mult
			else:
				word_map[word] = mult
	return word_map.iteritems()

def parse_summary(filename):
	v = [ [ ] for _ in xrange(512) ]
	with open(summary_filename) as f:
		for line in f:
			tokens = line.rstrip().split()
			if len(tokens) >= 2:
				word = tokens[0]
				if is_valid_word(word):
					try:
						year = int(tokens[1])
						if len(tokens) >= 3:
							mult = int(tokens[2])
						else:
							mult = 1
						v[year].append((word, mult))
					except IndexError, e:
						print(e, file=sys.stderr)

	for year, word_pairs in enumerate(v):
		words = [ ]
		if year >= 250:
			for word, mult in filter_word_pairs(word_pairs):
				for _ in xrange(mult):
					words.append(word)
		v[year] = words

	return v

directory = os.path.join('data', 'zeitgeist')
summary_directory = os.path.join(directory, 'summary')
history_directory = os.path.join(directory, 'history')

summary_suffix = '_summary.txt'

filenames = os.listdir(summary_directory)
filenames = filter(lambda x: x.endswith(summary_suffix), filenames)

for filename in filenames:
	pos = len(filename) - len(summary_suffix)
	prefix = filename[0:pos]
	summary_filename = os.path.join(summary_directory, filename)
	history_filename = os.path.join(history_directory, '%s_history.csv' % (prefix,))

	with open(history_filename, 'w') as f:
		v = parse_summary(summary_filename)
		row_id = 0
		line_fmt = '%d,%d,year %d,%s'
		for year, words in enumerate(v):
			if len(words) > 0:
				row_id += 1
				print(line_fmt % (row_id, year, 1500 + year, ' '.join(words)), file=f)
