/*		The HTML Parser					HTML.h
**		---------------
*/

#ifndef HTML_H
#define HTML_H

#include "HTUtils.h"
#include "HTAnchor.h"
#include "SGML.h"

extern SGML_dtd HTML_dtd;	/* The DTD */
extern void HTML_begin PARAMS((HTParentAnchor * anchor));
extern BOOL HTML_Parse PARAMS((
	HTParentAnchor * anchor,
	char (*next_char)() ));
#endif
