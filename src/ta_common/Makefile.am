
noinst_LTLIBRARIES = libta_common.la
AM_CPPFLAGS = -I../ta_common/

libta_common_la_SOURCES = ta_global.c \
	ta_retcode.c \
	ta_version.c \
	ta_accel.c \
	ta_accel_plan.c

if ENABLE_METAL_ACCEL
libta_common_la_SOURCES += ta_accel_metal.mm
libta_common_la_LIBADD = $(TA_ACCEL_METAL_LIBS)
libta_common_la_LDFLAGS = $(TA_ACCEL_METAL_LIBS)
AM_OBJCXXFLAGS = $(AM_CPPFLAGS)
endif

libta_commondir=$(includedir)/ta-lib/
libta_common_HEADERS = ../../include/ta_defs.h \
	../../include/ta_libc.h \
	../../include/ta_common.h \
	../../include/ta_accel.h \
	../../include/ta_accel_plan.h
