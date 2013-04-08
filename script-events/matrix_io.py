#!/usr/bin/python
from __future__ import print_function
import csv
import numpy as np
import os
from scipy.sparse import csr_matrix

def save_csr_matrix(filename, matrix):
	np.savez(filename, data=matrix.data, indices=matrix.indices,
		indptr=matrix.indptr, shape=matrix.shape)

def load_csr_matrix(filename):
	zmat = np.load(filename)
	return csr_matrix((zmat['data'], zmat['indices'], zmat['indptr']), zmat['shape'], dtype=np.uint8)

def save_csv_as_csr_matrix(csv_filename, matrix_filename):
	values = [ ]
	row_indices = [ ]
	column_indices = [ ]
	m = 0
	n = 0
	with open(csv_filename, 'r') as f:
		reader = csv.reader(f)
		for i, row in enumerate(reader):
			if i % 10000 == 0:
				print('Progress:', i)
			for j, elem in enumerate(map(int, row)):
				if elem != 0:
					values.append(elem)
					row_indices.append(i)
					column_indices.append(j)
			n = max(n, len(row))
			m += 1
	indices = (row_indices, column_indices)
	matrix = csr_matrix((values, indices), (m, n), dtype=np.uint8)
	save_csr_matrix(matrix_filename, matrix)

def transforms_csvs(in_directory, out_directory):
	filenames = os.listdir(in_directory)
	filenames.sort()
	for filename in filenames:
		base_filename = os.path.splitext(filename)[0]
		full_filename = os.path.join(in_directory, filename)
		print('Processing', full_filename)
		matrix_filename = os.path.join(out_directory, base_filename + '.npz')
		if not os.path.isfile(matrix_filename):
			save_csv_as_csr_matrix(full_filename, matrix_filename)

def read_matrices(directory):
	filenames = os.listdir(directory)
	matrices = { }
	for filename in filenames:
		base_filename = os.path.splitext(filename)[0]
		full_filename = os.path.join(directory, filename)
		matrix = load_csr_matrix(full_filename)
		matrices[base_filename] = matrix
	return matrices

def main():
	csv_directory = os.path.join('data', 'relevance')
	sparse_directory = os.path.join('data', 'sparse_relevance')
	transforms_csvs(csv_directory, sparse_directory)
	matrices = read_matrices(sparse_directory)
	print(matrices)

if __name__ == '__main__':
	main()
