/*			HyperText Object			HText.h
**			================
**	This is the C interface to the Objective-C HyperText class.
*/

#ifndef HTEXT_H
#define HTEXT_H
#include "HTAnchor.h"
#include "HTStyle.h"

#ifdef SHORT_NAMES
#define HTMainText			HTMaText
#define HTMainAnchor			HtMaAnch
#define HText_new			HTHTNew
#define HText_free			HTHTFree
#define HText_beginAppend		HTHTBeAp
#define HText_endAppend			HTHTEnAp
#define HText_setStyle			HTHTSeSt
#define HText_appendCharacter		HTHTApCh
#define HText_appendText		HTHTApTe
#define HText_appendParagraph		HTHTApPa
#define HText_beginAnchor		HTHTBeAn
#define HText_endAnchor			HTHTEnAn
#define HText_dump			HTHTDump
#define HText_nodeAnchor		HTHTNoAn
#define HText_select			HTHTSele
#define HText_selectAnchor		HTHTSeAn
#define HText_applyStyle		HTHTApSt
#define HText_updateStyle		HTHTUpSt
#define HText_selectionStyle		HTHTStyl
#define HText_replaceSel		HTHTRepl
#define HText_applyToSimilar		HTHTApTo
#define HText_selectUnstyled		HTHTSeUn
#define HText_unlinkSelection		HTHTUnSe
#define HText_linkSelTo			HTHTLiSe
#define HText_referenceSelected		HTHTRefS
#endif

#ifndef THINK_C
typedef struct _HText HText;
#else
class CHyperText;		/* Part of the Mac browser */
typedef CHyperText HText;
#endif

extern HText * HTMainText;		/* Pointer to current main text */
extern HTParentAnchor * HTMainAnchor;	/* Pointer to current text's anchor */

/*			Creation and deletion
**
**	Create hypertext object					HText_new
*/
 extern HText *	HText_new PARAMS((HTParentAnchor * anchor));

/*	Free hypertext object					HText_free
*/
extern void 	HText_free PARAMS((HText * me));


/*			Object Building methods
**			-----------------------
**
**	These are used by a parser to build the text in an object
**	HText_beginAppend must be called, then any combination of other
**	append calls, then HText_endAppend. This allows optimised
**	handling using buffers and caches which are flushed at the end.
*/
extern void HText_beginAppend PARAMS((HText * text));

extern void HText_endAppend PARAMS((HText * text));

/*	Set the style for future text
*/
extern void HText_setStyle PARAMS((HText * text, HTStyle * style));

/*	Add one character
*/
extern void HText_appendCharacter PARAMS((HText * text, char ch));

/*	Add a zero-terminated string
*/
extern void HText_appendText PARAMS((HText * text, CONST char * str));

/*	New Paragraph
*/
extern void HText_appendParagraph PARAMS((HText * text));

/*	Start/end sensitive text
**
** The anchor object is created and passed to HText_beginAnchor.
** The senstive text is added to the text object, and then HText_endAnchor
** is called. Anchors may not be nested.
*/

extern void HText_beginAnchor PARAMS((HText * text, HTChildAnchor * anc));
extern void HText_endAnchor PARAMS((HText * text));


/* 	Dump diagnostics to stderr
*/
extern void HText_dump PARAMS((HText * me));	

/*	Return the anchor associated with this node
*/
extern HTParentAnchor * HText_nodeAnchor PARAMS((HText * me));


/*		Browsing functions
**		------------------
*/

/*	Bring to front and highlight it
*/

extern BOOL HText_select PARAMS((HText * text)); 
extern BOOL HText_selectAnchor PARAMS((HText * text, HTChildAnchor* anchor)); 

/*		Editing functions
**		-----------------
**
**	These are called from the application. There are many more functions
**	not included here from the orginal text object. These functions
**	NEED NOT BE IMPLEMENTED in a browser which cannot edit.
*/

/*	Style handling:
*/
/*	Apply this style to the selection
*/
extern void HText_applyStyle PARAMS((HText * me, HTStyle *style));

/*	Update all text with changed style.
*/
extern void HText_updateStyle PARAMS((HText * me, HTStyle *style));

/*	Return style of  selection
*/
extern HTStyle * HText_selectionStyle PARAMS((
	HText * me,
	HTStyleSheet* sheet));

/*	Paste in styled text
*/
extern void HText_replaceSel PARAMS((HText * me,
	CONST char *aString, 
	HTStyle* aStyle));

/*	Apply this style to the selection and all similarly formatted text
**	(style recovery only)
*/
extern void HTextApplyToSimilar PARAMS((HText * me, HTStyle *style));
 
/*	Select the first unstyled run.
**	(style recovery only)
*/
extern void HTextSelectUnstyled PARAMS((HText * me, HTStyleSheet *sheet));


/*	Anchor handling:
*/
extern void		HText_unlinkSelection PARAMS((HText * me));
extern HTAnchor *	HText_referenceSelected PARAMS((HText * me));
extern HTAnchor *	HText_linkSelTo PARAMS((HText * me, HTAnchor* anchor));


#endif /* HTEXT_H */
