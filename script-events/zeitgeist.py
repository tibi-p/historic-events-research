#!/usr/bin/pypy
from __future__ import print_function
import errno
import os
import sys

min_year = 1500
num_years = 509

def ensure_directory_exists(path):
	try:
		os.makedirs(path)
	except OSError as exception:
		if exception.errno != errno.EEXIST:
			raise

def is_valid_word(word):
	return word and word[0].isalpha() and not '.' in word and not ',' in word

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

def index_summary(filename):
	idx = [ [] for _ in xrange(num_years) ]
	with open(filename) as f:
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
						idx[year].append((word, mult))
					except IndexError, e:
						print(e, file=sys.stderr)
	return idx

def year_documents(idx):
	year_docs = [ [ ] for _ in xrange(num_years) ]
	for year, word_pairs in enumerate(idx):
		words = [ ]
		if year >= 250:
			for word, mult in filter_word_pairs(word_pairs):
				for _ in xrange(mult):
					words.append(word)
		year_docs[year] = words
	return year_docs

def word_documents(idx):
	word_docs = []
	words = { }
	for year, word_pairs in enumerate(idx):
		if year >= 250:
			for word, mult in filter_word_pairs(word_pairs):
				if word in words:
					index = words[word]
				else:
					index = len(words)
					word_docs.append([word])
					words[word] = index
				for _ in xrange(mult):
					word_docs[index].append(year)
	return word_docs

def main():
	directory = os.path.join('data', 'zeitgeist')
	summary_directory = os.path.join(directory, 'summary')
	history_directory = os.path.join(directory, 'history')
	revhist_directory = os.path.join(directory, 'revhist')

	ensure_directory_exists(history_directory)
	ensure_directory_exists(revhist_directory)

	summary_suffix = '_summary.txt'
	filenames = os.listdir(summary_directory)
	filenames = filter(lambda x: x.endswith(summary_suffix), filenames)

	for filename in filenames:
		pos = len(filename) - len(summary_suffix)
		prefix = filename[0:pos]
		summary_filename = os.path.join(summary_directory, filename)
		idx = index_summary(summary_filename)

		history_filename = os.path.join(history_directory, '%s_history.csv' % (prefix,))
		with open(history_filename, 'w') as f:
			year_docs = year_documents(idx)
			row_id = 0
			line_fmt = '%d,%d,year %d,%s'
			for year, words in enumerate(year_docs):
				if words:
					row_id += 1
					print(line_fmt % (row_id, year, min_year + year, ' '.join(words)), file=f)

		revhist_filename = os.path.join(revhist_directory, '%s_revhist.csv' % (prefix,))
		with open(revhist_filename, 'w') as f:
			word_docs = word_documents(idx)
			row_id = 0
			line_fmt = '%d,%s,word %s,%s'
			for years in word_docs:
				if years:
					row_id += 1
					word = years[0]
					years = map(lambda x: str(min_year + x), years[1:])
					print(line_fmt % (row_id, word, word, ' '.join(years)), file=f)

if __name__ == '__main__':
	main()
