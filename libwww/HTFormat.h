/*		Manage different file formats			HTFormat.c
**		=============================
**
*/
#ifndef HTFORMAT_H
#define HTFORMAT_H

#include "HTUtils.h"

#ifdef SHORT_NAMES
#define HTOutputSource HTOuSour
#define HTOutputBinary HTOuBina
#endif

typedef int HTFormat;
				/* Can take the following values: */
#define WWW_INVALID   (-1)
#define WWW_SOURCE 	0	/* Whatever it was			*/
#define WWW_PLAINTEXT	1	/* plain ISO latin (ASCII)		*/
#define WWW_POSTSCRIPT	2	/* Postscript - encapsulated?		*/
#define	WWW_RICHTEXT	3	/* Microsoft RTF transfer format	*/
#define WWW_HTML	4	/* WWW HyperText Markup Language	*/
#define WWW_BINARY	5	/* Otherwise Unknown binary format */
#define WWW_GIF         6
#define WWW_JPEG        7
#define WWW_AUDIO       8
#define WWW_AIFF        9
#define WWW_DVI        10
#define WWW_MPEG       11
#define WWW_MIME       12
#define WWW_XWD        13
#define WWW_SGIMOVIE   14
#define WWW_EVLMOVIE   15
#define WWW_RGB        16
#define WWW_TIFF       17
#define WWW_TAR        18
#define WWW_CAVE       19
#define WWW_HDF        20

#define WWW_COMPRESSED 32
#define WWW_GZIPPED    33

/* Unknown is used to force a dump of binary data to disk. */
#define WWW_UNKNOWN    64
/* Impossible is when we have nothing useful to do -- 
   e.g., when we run into a Gopher CSO server, and the user
   is just plain shit outta luck. */
#define WWW_IMPOSSIBLE 65

#include "HTAnchor.h"



/*	Clear input buffer and set file number
*/
extern void HTInitInput PARAMS((int file_number));

/*	Get next character from buffer
*/
extern char HTGetChararcter NOPARAMS;


/*	Parse a file given its format
*/
extern void HTParseFormat PARAMS((
	HTFormat	format,
	HTParentAnchor	*anchor,
	int 		file_number,
        int             compressed));

extern BOOL HTOutputSource;	/* Flag: shortcut parser */
#endif
