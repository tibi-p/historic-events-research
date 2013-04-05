from django.conf.urls import patterns, url

from main import dictionary
from main import matrix_io
from main import views

views.ngram_db = dictionary.load_dictionary('../data')
views.matrices = matrix_io.read_matrices('../data/sparse_relevance')

urlpatterns = patterns('',
    url(r'^$', views.index, name='index'),
    url(r'^graph$', views.graph, name='graph'),
)
