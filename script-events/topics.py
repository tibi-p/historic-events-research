#!/usr/bin/python
from __future__ import print_function
import csv
import os
import sys

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

def process_topics(directory):
	distributions_filename = os.path.join(directory, 'document-topic-distributions.csv')
	topic_directory = get_topic_directory(directory)
	summary_filename = os.path.join(topic_directory, 'summary.txt')

	topic_words = { }
	with open(summary_filename, 'r') as f:
		current_topic = -1
		topic_words[current_topic] = [ ]
		for line in f:
			tokens = line.rstrip().split()
			num_tokens = len(tokens)
			if num_tokens >= 3:
				current_topic = int(tokens[1])
				topic_words[current_topic] = [ ]
			elif num_tokens >= 1:
				topic_words[current_topic].append(tokens[0])

	topic_years = { }
	with open(distributions_filename, 'r') as f:
		csvreader = csv.reader(f)
		for row in csvreader:
			if len(row) > 0:
				year = 1750 + int(row[0])
				values = [ float(value) for value in row[1:] ]
				max_value = max(values)
				topic = values.index(max_value)
				if topic in topic_years:
					topic_years[topic].append(year)
				else:
					topic_years[topic] = [ year ]

	mean_years = [ ]
	for topic, years in topic_years.iteritems():
		mean_year = sum(years) / float(len(years))
		mean_years.append((mean_year, topic))
	mean_years.sort()

	for _, topic in mean_years:
		years = topic_years[topic]
		if len(years) > 5:
			ranges = to_range_list(years)
			ranges = map(range_to_str, ranges)
			ranges = ', '.join(ranges)
			words = topic_words[topic]
			tokens = [ ranges ]
			tokens.extend(words)
			print('\n\t'.join(tokens))
			print()

if __name__ == '__main__':
	argv = sys.argv
	if len(argv) >= 2:
		process_topics(argv[1])
