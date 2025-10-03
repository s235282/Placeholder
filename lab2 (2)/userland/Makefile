CC := gcc
CFLAGS := -Wall -Wextra -Werror -g -O3 -march=native
LDLIBS := -pthread

COMMON_SRCS := utils.c worker_thread.c
COMMON_OBJS := \
	$(COMMON_SRCS:.c=.o)

LAB2_TEST_OBJS := lab2_test.o $(COMMON_OBJS)
UNITTEST_OBJS := unittest.o utils.o

.PHONY: all clean test

all: lab2_test unittest

test: unittest

lab2_test: $(LAB2_TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

unittest: $(UNITTEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f lab2_test unittest $(COMMON_OBJS) lab2_test.o unittest.o
