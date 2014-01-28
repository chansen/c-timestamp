CC      = cc
GCOV    = gcov
CFLAGS  = $(DCFLAGS) -Wall -I. -I..
LDFLAGS += -lc $(DLDFLAGS)

SOURCES = \
	timestamp_compare.c \
	timestamp_format.c \
	timestamp_parse.c

OBJECTS = \
	timestamp_compare.o \
	timestamp_format.o \
	timestamp_parse.o

HARNESS_OBJS = \
	t/compare.o \
	t/format.o \
	t/parse_wellformed.o \
	t/parse_malformed.o

HARNESS_EXES = \
	t/compare.t \
	t/format.t \
	t/parse_wellformed.t \
	t/parse_malformed.t

HARNESS_DEPS = \
	$(OBJECTS) \
	t/tap.o

.SUFFIXES:
.SUFFIXES: .o .c .t

.PHONY: check-asan test gcov cover clean

.o.t:
	$(CC) $(LDFLAGS) $< $(HARNESS_DEPS) -o $@

t/compare.o: \
	$(HARNESS_DEPS) t/compare.c

t/format.o: \
	$(HARNESS_DEPS) t/format.c

t/parse_wellformed.o: \
	$(HARNESS_DEPS) t/parse_wellformed.c

t/parse_malformed.o: \
	$(HARNESS_DEPS) t/parse_malformed.c

test: $(HARNESS_EXES) 
	@prove $(HARNESS_EXES)

check-asan:
	@$(MAKE) DCFLAGS="-O1 -g -fsanitize=address -fno-omit-frame-pointer" \
	DLDFLAGS="-g -fsanitize=address" test

gcov:
	@$(MAKE) DCFLAGS="-O0 -g -coverage" DLDFLAGS="-coverage" test
	@$(GCOV) $(SOURCES)

cover:
	@$(MAKE) DCFLAGS="-O0 -g --coverage" DLDFLAGS="-coverage" test
	@$(GCOV) -abc $(SOURCES) 
	@gcov2perl *.gcov
	@cover --no-gcov

clean:
	rm -f $(HARNESS_DEPS) $(HARNESS_OBJS) $(HARNESS_EXES) *.gc{ov,da,no} t/*.gc{ov,da,no}

