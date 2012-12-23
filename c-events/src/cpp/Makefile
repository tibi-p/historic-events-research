CC=g++
CXX=g++
CXXFLAGS=-Wall -Wextra -Wsign-conversion -I../../include -D_LARGEFILE64_SOURCE -O2 # -g -O0
CSV_PARSER_OBJS=csv_parser.o dictionary_files.o dictionary_writer.o util.o
SORTER_OBJS=sorter.o dictionary_files.o dictionary_reader.o util.o
PROCESS_OBJS=process.o dictionary_files.o dictionary_reader.o util.o
OUT_DIR=../../bin
OUT_CSV_PARSER_OBJS=$(addprefix $(OUT_DIR)/,$(CSV_PARSER_OBJS))
OUT_SORTER_OBJS=$(addprefix $(OUT_DIR)/,$(SORTER_OBJS))
OUT_PROCESS_OBJS=$(addprefix $(OUT_DIR)/,$(PROCESS_OBJS))
.PHONY : clean

all: build

build: $(OUT_DIR)/csv_parser $(OUT_DIR)/sorter $(OUT_DIR)/process

$(OUT_DIR)/csv_parser: $(OUT_CSV_PARSER_OBJS)

$(OUT_DIR)/sorter: $(OUT_SORTER_OBJS)

$(OUT_DIR)/process: $(OUT_PROCESS_OBJS)

$(OUT_DIR)/%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

run: csv_parser
	@./csv_parser

clean:
	rm -rf $(OUT_DIR)/*.o *~ $(OUT_DIR)/csv_parser $(OUT_DIR)/sorter $(OUT_DIR)/process
