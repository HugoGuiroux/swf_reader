CC=gcc
CFLAGS=-ggdb3 -Wall -Werror
LDFLAGS=-lz
OBJECTS=swf_read_header.o swf_decompress.o swf_print_tag.o swf_extract_binary_data.o
EXE=$(patsubst %.o, %, $(OBJECTS))
CFILES=$(patsubst %.o, %.c, $(OBJECTS))
COMMON=swf.o


all: $(EXE)


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

swf_read_header: $(COMMON) swf_read_header.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

swf_decompress: $(COMMON) swf_decompress.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

swf_print_tag: swf_print_tag.o $(COMMON) 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

swf_extract_binary_data: swf_extract_binary_data.o $(COMMON) 
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^


.PHONY: clean
clean:
	rm -f $(EXE) $(COMMON) $(OBJECTS) *.o *~
