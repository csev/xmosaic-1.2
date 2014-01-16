# ----------------------------------------------------------------------------
# For normal machines with normal compilers:
CC = cc
# For Sun's and other non-at-least-pseudo-ANSI-C platforms:
# CC = gcc
# ----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
# For SGI's:
CFLAGS = -g -cckr $(DTM_INCLUDES)
# For NeXT:
# CFLAGS = -g -DNEXT $(DTM_INCLUDES)
# For everyone else:
# CFLAGS = -g $(DTM_INCLUDES)
# ----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
# For SGI's:
RANLIB = /bin/true
# For everyone else:
# RANLIB = ranlib
# ----------------------------------------------------------------------------

# You shouldn't need to edit below here.

DTM_INCLUDES = -I../libdtm

LIBTARGET = libdmf.a

all: $(LIBTARGET)

CFILES = confirm.c eserv.c filexfer.c info.c inform.c inquire.c match.c query.c request.c widgets.c

OBJS = $(CFILES:.c=.o)

$(LIBTARGET): $(OBJS)
	-rm -f $(LIBTARGET)
	ar rv $(LIBTARGET) $(OBJS)
	$(RANLIB) $(LIBTARGET)

clean:
	-rm $(LIBTARGET) *.o
tags:
	etags -t *.[ch]
