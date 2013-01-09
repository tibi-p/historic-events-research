CC=gcc
CFLAGS=-Wall -Wextra -Wsign-conversion -I../../include -D_LARGEFILE64_SOURCE -O2
#CFLAGS+=-g
CACHE_OBJS=precache.o
SORTER_OBJS=sorter.o dictionary_files.o dictionary_reader.o util.o
UTIL_OBJS=dictionary_files.o dictionary_reader.o dictionary_writer.o \
	gaussian_model.o linear_model.o static_array.o util.o
OUT_DIR=../../bin
OUT_CACHE_OBJS=$(addprefix $(OUT_DIR)/,$(CACHE_OBJS))
OUT_SORTER_OBJS=$(addprefix $(OUT_DIR)/,$(SORTER_OBJS))
OUT_UTIL_OBJS=$(addprefix $(OUT_DIR)/,$(UTIL_OBJS))
.PHONY : clean

all: build

build: $(OUT_DIR)/precache $(OUT_DIR)/sorter build_utils

$(OUT_DIR)/precache: $(OUT_CACHE_OBJS)

$(OUT_DIR)/sorter: $(OUT_SORTER_OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

build_utils: $(OUT_UTIL_OBJS)

$(OUT_DIR)/%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

run: precache
	@./precache

clean:
	rm -rf $(OUT_DIR)/*.o *~ $(OUT_DIR)/precache $(OUT_DIR)/sorter
