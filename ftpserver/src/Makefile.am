bin_PROGRAMS = ftpserver
ftpserver_LDFLAGS = -pthread
ftpserver_CPPFLAGS = -I $(top_srcdir)/inc
ftpserver_SOURCES = main.c util.c ftpserver.c
ftpserverdatadir = $(datadir)/ftpserver
ftpserverdata_DATA = users
EXTRA_DIST = users
