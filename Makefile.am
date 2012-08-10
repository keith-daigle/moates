AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

bin_PROGRAMS = burn ostrich
burn_SOURCES = src/burn.cpp src/Burn/Burn.cpp src/Serial/Serial.cpp
burn_CPPFLAGS = -I$(top_srcdir)/src/Serial -I$(top_srcdir)/src/Burn
ostrich_SOURCES = src/Ostrich/util/OstrichDriver.cpp src/Ostrich/Ostrich.cpp src/Serial/Serial.cpp
ostrich_CPPFLAGS = -I$(top_srcdir)/src/Serial -I$(top_srcdir)/src/Ostrich
