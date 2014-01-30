# Makefile for NCSA Mosaic for the X Window System.
# Copyright 1993 National Center for Supercomputing Applications
#


# ----------------------------------------------------------------------------
# For normal machines with normal compilers:
# CC = cc
# For Sun's and other non-at-least-pseudo-ANSI-C platforms:
# CC = gcc
# For stupid Ultrix:
# CC = cc -DSTUPID_ULTRIX
# For HP-UX 8.00:
# CC = cc -Aa -D_HPUX_SOURCE -I/usr/include/Motif1.1 -I/usr/include/X11R4
# For NeXT:

# For normal machines with normal compilers:
ifeq ($(DEV_ARCH), linux)
 CC = cc -I/usr/include/X11 -I/usr/include -Dlinux
endif

ifeq ($(DEV_ARCH), osx)
 CC = cc -DNEXT -I/usr/include/X11 -I/opt/X11/include -I/usr/OpenMotif/include -mmacosx-version-min=10.5 -arch i386 -Dosx
endif

ifeq ($(DEV_ARCH), solaris9)
 CC = gcc
endif

# For Dell SVR4:
# CC = cc -DSVR4
# ----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
# For SGI's only:
# AUX_CFLAGS = -cckr
# For normal platforms:
AUX_CFLAGS =  
# ----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
# For SGI's only:
# X_LIBS = -lXm_s -lXmu -lXt_s -lX11_s
# For HP-UX 8.00:
# X_LIBS = -L/usr/lib/Motif1.1 -lXm -L/usr/lib/X11R4 -lXmu -lXt -lX11
# For NeXT:
ifeq ($(DEV_ARCH), linux)
 X_LIBS = -lXm -lXmu -lXt -lX11 -L/usr/lib -L/lib -L/usr/lib/x86_64-linux-gnu
endif

ifeq ($(DEV_ARCH), osx)
 X_LIBS = -L/usr/OpenMotif/lib -L/usr/X11/lib -L/opt/X11/lib -lXm -lXmu -lXt -lX11
endif

ifeq ($(DEV_ARCH), solaris9)
 X_LIBS = -lXm -lXmu -lXt -lX11
endif

# For nearly everyone else:
# X_LIBS = -lXm -lXmu -lXt -lX11
# For Sun's (at least running X/Motif as installed on our machines):
# X_LIBS = /usr/lib/libXm.a /usr/lib/libXmu.a /usr/lib/libXt.a /usr/lib/libXext.a /usr/lib/libX11.a -lm
# For Dell SVR4:
# X_LIBS = -L/usr/X5/lib -lXm -lXmu -lXt -lXext -lX11
# ----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
# For SGI's only:
# SYS_LIBS = -lPW -lsun -lmalloc
# For most other Motif platforms:
# SYS_LIBS = -lPW
# For SCO ODT:
# SYS_LIBS = -lPW -lsocket -lmalloc
# For Sun's and Ultrix:
SYS_LIBS = 
# For Sun's with no DNS:
# SYS_LIBS = -lresolv
# For Dell SVR4:
# SYS_LIBS = -lnsl -lsocket -lc -lucb

ifeq ($(DEV_ARCH), solaris9)
 SYS_LIBS = -lnsl -lsocket
endif

# ----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
# If you want to compile DTM support in, use these lines...
# DTMSRCS = net.c netdata.c list.c mesg.c
# DTMOBJS = net.o netdata.o list.o mesg.o
# DTM_FLAGS = -DHAVE_DTM -DHAVE_DMF -I..
# DTM_LIBS = ../libdtm/libdtm.a ../libdmf/libdmf.a
# Else use these lines...
DTMSRCS = 
DTMOBJS =
DTM_FLAGS =
DTM_LIBS =
# See the toplevel Mosaic README for more information on DTM.
# ----------------------------------------------------------------------------

# ----------------------------------------------------------------------------
# If you want Mosaic to come up with monochrome colors by default,
# use this line:
# COLOR_FLAGS = -DMONO_DEFAULT
# Else use this line:
COLOR_FLAGS =
# ----------------------------------------------------------------------------

# You shouldn't need to edit below here.

all: xmosaic

LIBWWW_DIR = ../libwww
LIBWWW_INC = ../libwww
HTMLW_LIBDIR = ../libhtmlw

INCLUDES = -I.. -I$(LIBWWW_INC)
OPTS = -g
# Do NOT compile with -DINSTRUMENT, please please please.
# OPTS = -g -DINSTRUMENT
PROFILE_FLAGS =
CFLAGS = $(OPTS) $(INCLUDES) $(DTM_FLAGS) $(COLOR_FLAGS)

PROGRAM_LIBS = $(HTMLW_LIBDIR)/libhtmlw.a $(LIBWWW_DIR)/libwww.a
LIBS = $(PROGRAM_LIBS) $(X_LIBS) $(DTM_LIBS) $(SYS_LIBS)

CFILES = main.c gui.c gui2.c mo-www.c mo-dtm.c hotlist.c whine.c history.c\
  annotate.c pan.c grpan.c grpan-www.c audan.c mo-dmf.c globalhist.c img.c\
  instrument.c startup.c
OBJS = main.o gui.o gui2.o mo-www.o mo-dtm.o hotlist.o whine.o history.o\
  annotate.o pan.o grpan.o grpan-www.o audan.o mo-dmf.o globalhist.o img.o\
  instrument.o startup.o

AUXCFILES = Xmx.c Xmx2.c picread.c gifread.c medcut.c
AUXOBJS = Xmx.o Xmx2.o picread.o gifread.o medcut.o

Xmx.o: Xmx.c
	$(CC) -c $(CFLAGS) $(AUX_CFLAGS) Xmx.c
Xmx2.o: Xmx2.c
	$(CC) -c $(CFLAGS) $(AUX_CFLAGS) Xmx2.c
picread.o: picread.c
	$(CC) -c $(CFLAGS) $(AUX_CFLAGS) picread.c
gifread.o: gifread.c
	$(CC) -c $(CFLAGS) $(AUX_CFLAGS) gifread.c
medcut.o: medcut.c
	$(CC) -c $(CFLAGS) $(AUX_CFLAGS) medcut.c

xmosaic: $(OBJS) $(AUXOBJS) $(DTMOBJS) $(PROGRAM_LIBS)
	$(CC) $(PROFILE_FLAGS) -o xmosaic $(OBJS) $(AUXOBJS) $(DTMOBJS) $(LIBS)

HFILES = mosaic.h
$(OBJS): $(HFILES)
gui.o: xresources.h

wipe:
	-rm xmosaic $(OBJS)
clean:
	-rm xmosaic *.o
tags:
	etags -t *.[ch]
