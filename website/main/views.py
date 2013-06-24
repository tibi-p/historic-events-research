from __future__ import print_function
import json
from django.http import HttpResponse
from django.shortcuts import get_object_or_404, render_to_response
from django.template import RequestContext

ngram_db = None
matrices = None

def get_actual_year(year):
	if year < 1500:
		return 1500
	elif year > 2008:
		return 2008
	else:
		return year

def index(request):
    return render_to_response('index.html',
        {
        },
        context_instance=RequestContext(request))

def row_to_series(row):
	series = 509 * [ 0 ]
	data = row.data
	indices = row.indices
	for i, index in enumerate(indices):
		series[index] = int(data[i])
	return series

def graph(request):
	global ngram_db, matrices
	result = { }

	if request.method == 'GET':
		params = request.GET
		smoothing = int(params[u'smoothing'])
		year_start = int(params[u'year_start'])
		year_end = int(params[u'year_end'])
		model_set = set(params.getlist(u'model_type'))

		year_start = get_actual_year(year_start)
		year_end = get_actual_year(year_end)
		if year_end < year_start:
			year_end = year_start

		content = params[u'content']
		tokens = content.split(',')
		ngrams = [ ]
		relevances = [ ]
		local_maxima = [ ]
		for i, word in enumerate(tokens):
			series = ngram_db.get_time_series(word, smoothing)
			index = ngram_db.word_indices.get(word, 0)
			series = series[(year_start - 1500):(year_end - 1500 + 1)]

			local_maxima.append(max(series))
			ngrams.append({
				'word': word,
				'series': series,
			})

			for model_type, matrix in matrices.iteritems():
				if model_type in model_set:
					row = matrix.getrow(index)
					row_series = row_to_series(row)
					row_series = row_series[(year_start - 1500):(year_end - 1500 + 1)]
					relevances.append({
						'word': '%s (%s)' % (word, model_type.replace('_', ' ')),
						'series': row_series,
					})

		if 'time_series' in model_set:
			indexed_maxima = list(enumerate(local_maxima))
			indexed_maxima = sorted([ (v, k) for k, v in indexed_maxima ], reverse=True)
			shrink_factors = { }
			for i, (locmax, index) in enumerate(indexed_maxima):
				shrink_factors[index] = 1 - .05 * i
			global_maximum = indexed_maxima[0][0]

			for i, ngram in enumerate(ngrams):
				locmax = local_maxima[i]
				if locmax > 0.0 and locmax != global_maximum:
					ratio = shrink_factors[i] * global_maximum / locmax
					ngram['word'] += ' (*{0:.2e})'.format(ratio)
					ngram['series'] = [ x * ratio for x in ngram['series'] ]

			for i, relevance in enumerate(relevances):
				ratio = global_maximum / 10
				relevance['word'] += ' (*{0:.2e})'.format(ratio)
				relevance['series'] = [ x * ratio for x in relevance['series'] ]
			ngrams.extend(relevances)
		else:
			ngrams = relevances

		result['year_start'] = year_start
		result['year_end'] = year_end
		result['ngrams'] = ngrams

	return HttpResponse(json.dumps(result), content_type="application/json")
