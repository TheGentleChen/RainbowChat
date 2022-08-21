CFLAGS := -g -Wall -Wno-implicit-function-declaration
TARGETS := test_endpoint
all: $(TARGETS)

test_endpoint: test_endpoint.c ../endpoint.c ../user.c
	gcc $^ -o $@ $(CFLAGS)

clean:
	rm -rf $(TARGETS)

.PHONY: clean all
