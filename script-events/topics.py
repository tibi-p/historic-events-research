#!/usr/bin/python
from __future__ import print_function
import csv
import errno
import gzip
import os
import sys
import numpy as np
from collections import defaultdict

def get_last_directory(directory):
	subdir = os.path.basename(directory)
	if not subdir:
		directory = os.path.dirname(directory)
		subdir = os.path.basename(directory)
	return subdir

def get_topic_directory(directory):
	dir_list = os.listdir(directory)
	max_id = -sys.maxint - 1
	max_dir_entry = ''
	for dir_entry in dir_list:
		try:
			entry_id = int(dir_entry)
			if entry_id > max_id:
				max_id = entry_id
				max_dir_entry = dir_entry
		except ValueError, e:
			print(e, file=sys.stderr)
	return os.path.join(directory, max_dir_entry)

def to_range_list(values):
	ranges = [ ]
	start_value = None
	prev_value = None
	for value in values:
		if value - 1 != prev_value:
			if start_value is not None:
				ranges.append((start_value, prev_value))
			start_value = value
		prev_value = value
	if start_value is not None:
		ranges.append((start_value, prev_value))
	return ranges

def range_to_str(value):
	if value[0] != value[1]:
		return '%s-%s' % value
	else:
		return str(value[0])

def extract_topic_words(summary_filename):
	topic_words = defaultdict(lambda: [ ])
	with open(summary_filename, 'r') as f:
		current_topic = -1
		for line in f:
			tokens = line.rstrip().split()
			num_tokens = len(tokens)
			if num_tokens >= 3:
				current_topic = int(tokens[1])
			elif num_tokens >= 1:
				topic_words[current_topic].append(tokens[0])
	return topic_words

def extract_topic_years(year_map, distributions_filename):
	topic_years = defaultdict(lambda: [ ])
	with open(distributions_filename, 'r') as f:
		csvreader = csv.reader(f)
		for row in csvreader:
			if len(row) > 0:
				index = int(row[0])
				year = 1500 + year_map[index]
				values = [ (float(value), i) for i, value in enumerate(row[1:]) ]
				max_value, topic = max(values)
				topic_years[topic].append(year)
	return topic_years

def process_topics(directory):
	model_type = get_last_directory(directory).split('-')[0]
	history_filename = os.path.join('data', 'zeitgeist', 'history', model_type + '_history.csv')
	year_map = { }
	history_rows = [ ]
	with open(history_filename, 'r') as f:
		reader = csv.reader(f)
		for row in reader:
			index = int(row[0])
			year = int(row[1])
			year_map[index] = year
			history_rows.append(row)

	distributions_filename = os.path.join(directory, 'document-topic-distributions.csv')
	topic_directory = get_topic_directory(directory)
	summary_filename = os.path.join(topic_directory, 'summary.txt')

	subcorpus_directory = os.path.join('data', 'subcorpus')
	submodel_directory = os.path.join(subcorpus_directory, model_type)
	try:
		os.makedirs(submodel_directory)
	except OSError as e:
		if e.errno != errno.EEXIST:
			raise

	terms = [ ]
	term_filename = os.path.join(topic_directory, 'term-index.txt')
	print(term_filename)
	with open(term_filename, 'r') as f:
		for line in f:
			terms.append(line.rstrip())

	term_distro_filename = os.path.join(topic_directory, 'topic-term-distributions.csv.gz')
	topics = [ ]
	with gzip.open(term_distro_filename) as f:
		reader = csv.reader(f)
		for i, row in enumerate(reader):
			topic = [ ]
			for j, elem in enumerate(map(int, row)):
				if elem != 0:
					topic.append((j, elem))
			topic_words = map(lambda x: terms[x[0]], topic)
			topics.append(set(topic_words))

	for i, topic in enumerate(topics):
		subtopic_filename = os.path.join(submodel_directory, 'topic%03d_history.csv' % (i,))
		if not os.path.isfile(subtopic_filename):
			with open(subtopic_filename, 'w') as f:
				writer = csv.writer(f)
				for row in history_rows:
					subtopic_row = row[:-1]
					words = row[-1]
					subtopic_words = [ ]
					for word in words.split():
						if word.lower() in topic:
							subtopic_words.append(word)
					subtopic_row.append(' '.join(subtopic_words))
					writer.writerow(subtopic_row)

	topic_words = extract_topic_words(summary_filename)
	topic_years = extract_topic_years(year_map, distributions_filename)

	mean_years = [ ]
	for topic, years in topic_years.iteritems():
		mean_year = np.mean(years)
		mean_years.append((mean_year, topic))
	mean_years.sort()

	topic_filename = os.path.join('data', 'topics', model_type + '_historic_topics.txt')
	with open(topic_filename, 'w') as f:
		for _, topic in mean_years:
			years = topic_years[topic]
			if len(years) > 2:
				ranges = to_range_list(years)
				ranges = map(range_to_str, ranges)
				ranges = ', '.join(ranges)
				words = topic_words[topic]
				tokens = [ ranges ]
				tokens.extend(words)
				print('\n\t'.join(tokens), file=f)
				print(file=f)

def main(argv):
	csv.field_size_limit(sys.maxsize)
	lda_directory = os.path.join('data', 'lda')
	directories = os.listdir(lda_directory)
	for directory in directories:
		full_directory = os.path.join(lda_directory, directory)
		process_topics(full_directory)

if __name__ == '__main__':
	main(sys.argv)
