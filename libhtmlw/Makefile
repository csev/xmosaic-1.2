# ----------------------------------------------------------------------------
# For normal machines with normal compilers:
CC = cc
# For Sun's and other non-at-least-pseudo-ANSI-C platforms:
# CC = gcc
# ----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
# If you are using Motif 1.2, you should define MOTIF1_2 (e.g. SCO
# below).  Else Motif 1.1 is assumed.  (The widget will work
# standalone without Motif, but Mosaic requires Motif.)
# For SGI's:
OTHERFLAGS = -cckr
# For HP's:
# OTHERFLAGS = -I/usr/include/Motif1.1 -I/usr/include/X11R4
# For SCO:
# OTHERFLAGS = -DMOTIF1_2
# For NeXT:
# OTHERFLAGS = -DNEXT -I/usr/include/X11
# For everyone else:
# OTHERFLAGS = 
# ----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
# For SGI's:
RANLIB = /bin/true
# For everyone else:
# RANLIB = ranlib
# ----------------------------------------------------------------------------

# You shouldn't need to edit below here.

CFLAGS = -g -DMOTIF -DXMOSAIC $(OTHERFLAGS)

LIBTARGET = libhtmlw.a

all: $(LIBTARGET)

CFILES = HTML.c HTMLlists.c HTMLformat.c HTMLparse.c HTMLimages.c HTML-PSformat.c

OBJS = $(CFILES:.c=.o)

$(LIBTARGET): $(OBJS)
	-rm -f $(LIBTARGET)
	ar rv $(LIBTARGET) $(OBJS)
	$(RANLIB) $(LIBTARGET)

clean:
	-rm $(LIBTARGET) *.o
tags:
	etags -t *.[ch]
