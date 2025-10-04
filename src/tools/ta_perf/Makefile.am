
# Performance micro-benchmarks for TA-Lib acceleration.
noinst_PROGRAMS = ta_perf

ta_perf_SOURCES = ta_perf.c

ta_perf_CPPFLAGS = -I../../ta_func \
                   -I../../ta_common \
                   -I../../ta_abstract \
                   -I../../include

ta_perf_LDFLAGS = -no-undefined

ta_perf_LDADD = ../../libta-lib.la $(LIBM)

all-local: ta_perf
	$(LIBTOOL) --mode=execute ../post-build-bin.sh ta_perf

