CC=g++
CXX=g++
CXXFLAGS=-Wall -Wextra -Wsign-conversion -I../../include -D_LARGEFILE64_SOURCE -O2
#CXXFLAGS+=-g
LDFLAGS=-lgsl -lgslcblas
CLUSTERING_PARSER_OBJS=clustering.o
CSV_PARSER_OBJS=csv_parser.o dictionary_files.o dictionary_writer.o util.o
PROCESS_OBJS=process.o dictionary_files.o dictionary_reader.o util.o \
	gaussian_finder.o numerical_discrepancy.o gaussian_model.o linear_model.o series.o static_array.o
RELEVANCE_OBJS=relevance.o dictionary_files.o dictionary_reader.o util.o \
	gaussian_finder.o numerical_discrepancy.o kleinberg.o gaussian_model.o linear_model.o series.o static_array.o
OUT_DIR=../../bin
OUT_CLUSTERING_PARSER_OBJS=$(addprefix $(OUT_DIR)/,$(CLUSTERING_PARSER_OBJS))
OUT_CSV_PARSER_OBJS=$(addprefix $(OUT_DIR)/,$(CSV_PARSER_OBJS))
OUT_PROCESS_OBJS=$(addprefix $(OUT_DIR)/,$(PROCESS_OBJS))
OUT_RELEVANCE_OBJS=$(addprefix $(OUT_DIR)/,$(RELEVANCE_OBJS))
.PHONY : clean

all: build

build: $(OUT_DIR)/clustering $(OUT_DIR)/csv_parser $(OUT_DIR)/process $(OUT_DIR)/relevance

$(OUT_DIR)/clustering: $(OUT_CLUSTERING_PARSER_OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

$(OUT_DIR)/csv_parser: $(OUT_CSV_PARSER_OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

$(OUT_DIR)/process: $(OUT_PROCESS_OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

$(OUT_DIR)/relevance: $(OUT_RELEVANCE_OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

$(OUT_DIR)/%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

run: csv_parser
	@./csv_parser

clean:
	rm -rf $(OUT_DIR)/*.o *~ $(OUT_DIR)/csv_parser $(OUT_DIR)/process $(OUT_DIR)/relevance
