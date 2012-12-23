CC=gcc
CFLAGS=-Wall -Wextra -Wsign-conversion -I../../include -D_LARGEFILE64_SOURCE -O2 # -g -O0
CACHE_OBJS=precache.o
UTIL_OBJS=dictionary_files.o dictionary_reader.o dictionary_writer.o util.o
OUT_DIR=../../bin
OUT_CACHE_OBJS=$(addprefix $(OUT_DIR)/,$(CACHE_OBJS))
OUT_UTIL_OBJS=$(addprefix $(OUT_DIR)/,$(UTIL_OBJS))
.PHONY : clean

all: build

build: $(OUT_DIR)/precache build_utils

$(OUT_DIR)/precache: $(OUT_CACHE_OBJS)

build_utils: $(OUT_UTIL_OBJS)

$(OUT_DIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

run: precache
	@./precache

clean:
	rm -rf $(OUT_DIR)/*.o *~ $(OUT_DIR)/precache
