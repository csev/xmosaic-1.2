# ----------------------------------------------------------------------------
# For normal machines with normal compilers:
CC = cc -mmacosx-version-min=10.5 -arch i386
# For Sun's and other non-at-least-pseudo-ANSI-C platforms:
# CC = gcc
# ----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
# For SGI's:
# CFLAGS = -g -cckr
# For HP's:
# CFLAGS = -g -Aa -D_HPUX_SOURCE
# For SCO ODT:
# CFLAGS = -g -Dsco
# For DEC Alpha OSF/1:
# CFLAGS = -g -DUSE_DIRENT -DUSE_FILENO
# For NeXT:
CFLAGS = -g -DNEXT 
# For Dell SVR4:
# CFLAGS = -g -DSVR4
# For Harris Nighthawk:
# CFLAGS = -g -DUSE_DIRENT -DUSE_FILENO
# For everyone else:
# CFLAGS = -g
# ----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
# For SGI's and SCO ODT:
# RANLIB = /bin/true
# For everyone else:
RANLIB = ranlib
# ----------------------------------------------------------------------------

# You shouldn't need to edit below here.

LIBTARGET = libwww.a

all: $(LIBTARGET)

CFILES = \
HTAccess.c \
HTAnchor.c \
HTAtom.c \
HTChunk.c \
HTFTP.c \
HTFile.c \
HTFormat.c \
HTGopher.c \
HTHistory.c \
HTList.c \
HTML.c \
HTNews.c \
HTParse.c \
HTString.c \
HTStyle.c \
HTTCP.c \
HTTP.c \
SGML.c

OBJS = $(CFILES:.c=.o)

$(LIBTARGET): $(OBJS)
	-rm -f $(LIBTARGET)
	ar rv $(LIBTARGET) $(OBJS)
	$(RANLIB) $(LIBTARGET)

clean:
	-rm $(LIBTARGET) *.o
tags:
	etags -t *.[ch]
