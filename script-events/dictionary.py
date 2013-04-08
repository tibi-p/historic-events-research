#!/usr/bin/python
from __future__ import print_function
import os
import struct

class NgramDatabaseEntry(object):
	def __init__(self, entry):
		self.word_offset = entry[0]
		self.time_offset = entry[1]
		self.word_length = entry[2]
		self.time_length = entry[3]
		self.total_match_count = entry[4]
		self.total_volume_count = entry[5]

class NgramWord(object):
	def __init__(self, word, db_entry):
		self.word = word
		self.time_offset = db_entry.time_offset
		self.time_length = db_entry.time_length
		self.total_match_count = db_entry.total_match_count
		self.total_volume_count = db_entry.total_volume_count

class NgramTotalCountsEntry(object):
	def __init__(self, row):
		self.year = row[0]
		self.match_count = float(row[1])
		self.page_count = float(row[2])
		self.volume_count = float(row[3])

class NgramDatabase(object):
	def __init__(self, directory, base_filename):
		self.base_filename = os.path.join(directory, base_filename)
		self.total_counts = { }
		self.db_words = { }
		self.word_indices = { }

		total_counts_filename = os.path.join(directory, 'googlebooks-eng-all-totalcounts-20120701.txt')
		with open(total_counts_filename, 'r') as f:
			for line in f:
				for row in line.split('\t'):
					row = row.lstrip()
					if row:
						row = map(int, row.split(','))
						counts_entry = NgramTotalCountsEntry(row)
						self.total_counts[counts_entry.year] = counts_entry

		with open(self.base_filename + '.words', 'rb') as words_file:
			with open(self.base_filename + '.main', 'rb') as f:
				index = 0
				while True:
					bytes = f.read(32)
					if not bytes:
						break
					entry = struct.unpack('IIHHQQ', bytes)
					db_entry = NgramDatabaseEntry(entry)
					words_file.seek(db_entry.word_offset)
					word = words_file.read(db_entry.word_length)
					self.db_words[word] = NgramWord(word, db_entry)
					self.word_indices[word] = index
					index += 1

		self.time_file = open(self.base_filename + '.time', 'rb')

	def get_sparse_series(self, word):
		entries = [ ]
		if word not in self.db_words:
			return entries
		db_entry = self.db_words[word]
		self.time_file.seek(db_entry.time_offset)
		for _ in xrange(db_entry.time_length):
			bytes = self.time_file.read(16)
			entry = struct.unpack('QIHH', bytes)
			year = entry[2]
			all_count = self.total_counts[year].match_count
			match_freq = entry[0] / all_count
			entries.append((year, match_freq))
		return entries

	def get_time_series(self, word, smoothing_window=0):
		sparse_series = self.get_sparse_series(word)
		series = 509 * [ 0 ]
		for year, count in sparse_series:
			series[year - 1500] = count
		if smoothing_window:
			smooth_series = [ ]
			smoothing_sum = 0.0
			series_size = len(series)
			window_size = 0
			for i in xrange(series_size + smoothing_window):
				if i < series_size:
					smoothing_sum += series[i]
					window_size += 1
				if i > 2 * smoothing_window:
					smoothing_sum -= series[i - 2 * smoothing_window - 1]
					window_size -= 1
				if smoothing_sum < 0.0:
					smoothing_sum = 0.0
				if i >= smoothing_window:
					smooth_series.append(smoothing_sum / window_size)
			return smooth_series
		else:
			return series

def load_dictionary(directory):
	base_filename = os.path.join('sort', 'googlebooks-eng-all-1gram-20120701-database')
	ngram_db = NgramDatabase(directory, base_filename)
	return ngram_db

def main():
	ngram_db = load_dictionary('data')
	ngram_db.get_time_series('Microsoft')

if __name__ == '__main__':
	main()
