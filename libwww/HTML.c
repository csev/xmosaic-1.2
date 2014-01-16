/*		HTML Parser
**		===========
*/
#include <ctype.h>
#include <stdio.h>

#include "HTUtils.h"
#include "SGML.h"
#include "HTAtom.h"
#include "HTChunk.h"
#include "HText.h"
#include "HTStyle.h"
#include "HTML.h"


/*				SPECIAL HTML CODE
**				=================
*/

extern HTStyleSheet * styleSheet;	/* Application-wide */

/*	HTML parsing context
**	--------------------
*/
PRIVATE HTParentAnchor * node_anchor;
PRIVATE HText * text;

PRIVATE HTStyle * glossary_style;
PRIVATE HTStyle * list_compact_style;
PRIVATE HTStyle * glossary_compact_style;

PRIVATE HTChunk title = { 0, 128, 0, 0 };	/* Grow by 128 */

PRIVATE int got_styles = 0;
PRIVATE void get_styles NOPARAMS;

PRIVATE	BOOL		style_change;
PRIVATE HTStyle *	new_style;
PRIVATE HTStyle *	old_style;
PRIVATE BOOL		in_word;  /* Have just had a non-white character */


/*	Forward declarations of routines for DTD
*/
PRIVATE void no_change PARAMS((HTTag * t, HTElement * e));
PRIVATE void begin_litteral PARAMS((HTTag * t, HTElement * e));
PRIVATE void begin_element PARAMS((HTTag * t, HTElement * e));
PRIVATE void end_element PARAMS((HTTag * t, HTElement * e));
PRIVATE void begin_document PARAMS((HTTag * t, HTElement * e));
PRIVATE void end_document PARAMS((HTTag * t, HTElement * e));
PRIVATE void begin_anchor PARAMS((HTTag * t, HTElement * e));
PRIVATE void end_anchor PARAMS((HTTag * t, HTElement * e));
PRIVATE void begin_list PARAMS((HTTag * t, HTElement * e));
PRIVATE void list_element PARAMS((HTTag * t, HTElement * e));
PRIVATE void end_list PARAMS((HTTag * t, HTElement * e));
PRIVATE void begin_glossary PARAMS((HTTag * t, HTElement * e));
PRIVATE void end_glossary PARAMS((HTTag * t, HTElement * e));

PRIVATE void actually_set_style NOPARAMS;
PRIVATE void change_style PARAMS((HTStyle * style));

/*	Style buffering avoids dummy paragraph begin/ends.
*/
#define UPDATE_STYLE if (style_change) { actually_set_style(); }


/*	Things affecting the anchor but not the document itself
**	-------------------------------------------------------
*/


/*		TITLE
*/

/*	Accumulate a character of title
*/
#ifdef __STDC__
static void accumulate_string(char c)
#else
static void accumulate_string(c)
    char c;
#endif
{
    HTChunkPutc(&title, c);
}


/*		Clear the title
*/
PRIVATE  void clear_string ARGS2(HTTag *,t, HTElement *,e)
{
    HTChunkClear(&title);
}

PRIVATE void set_title ARGS2(HTTag *,t, HTElement *,e)
{
    HTChunkTerminate(&title);
    HTAnchor_setTitle(node_anchor, title.data);
}

PRIVATE void set_index ARGS2(HTTag *,t, HTElement *,e)
{
    HTAnchor_setIndex(node_anchor);
}

/*			Things affecting the document
**			-----------------------------
*/
/*		Character handling
*/
PRIVATE void pass_character ARGS1(char, c)
{
    if (style_change) {
        if ((c=='\n') || (c==' ')) return;	/* Ignore it */
        UPDATE_STYLE;
    }
    if (c=='\n') {
        if (in_word) {
	    HText_appendCharacter(text, ' ');
	    in_word = NO;
	}
    } else {
        HText_appendCharacter(text, c);
	in_word = YES;
    }
}

PRIVATE void litteral_text ARGS1(char, c)
{
/*	We guarrantee that the style is up-to-date in begin_litteral
*/
    HText_appendCharacter(text, c);		/* @@@@@ */
}

PRIVATE void ignore_text ARGS1(char, c)
{
    /* Do nothing */
}

PRIVATE void set_next_id  ARGS2(HTTag *,t, HTElement *,e)
{
    /* @@@@@  Bad SGML anyway */
}

PRIVATE void new_paragraph  ARGS2(HTTag *,t, HTElement *,e)
{
    UPDATE_STYLE;
    HText_appendParagraph(text);
    in_word = NO;
}

PRIVATE void term ARGS2(HTTag *,t, HTElement *,e)
{
    if (!style_change) {
        HText_appendParagraph(text);
	in_word = NO;
    }
}

PRIVATE void definition ARGS2(HTTag *,t, HTElement *,e)
{
    UPDATE_STYLE;
    pass_character('\t');	/* Just tab out one stop */
    in_word = NO;
}

/*		Our Static DTD for HTML
**		-----------------------
*/

static entity entities[] = {
	{ "lt",	"<" },
	{ "gt", ">" },
	{ "amp", "&" },
#ifdef NeXT
	{ "bullet" , "\267" },			/* @@@ NeXT only */
#endif
/* The following accesnted characters are from peter Flynn, curia project */

/* these ifdefs don't solve the problem of a simple terminal emulator
** with a different character set to the client machine. But nothing does,
** except looking at the TERM setting */

        { "ocus" , "&" },       /* for CURIA */
#ifdef IBMPC
        { "aacute" , "\240" },	/* For PC display */
        { "eacute" , "\202" },
        { "iacute" , "\241" },
        { "oacute" , "\242" },
        { "uacute" , "\243" },
        { "Aacute" , "\101" },
        { "Eacute" , "\220" },
        { "Iacute" , "\111" },
        { "Oacute" , "\117" },
        { "Uacute" , "\125" },
#else
        { "aacute" , "\341" },	/* Works for openwindows -- Peter Flynn */
        { "eacute" , "\351" },
        { "iacute" , "\355" },
        { "oacute" , "\363" },
        { "uacute" , "\372" },
        { "Aacute" , "\301" },
        { "Eacute" , "\310" },
        { "Iacute" , "\315" },
        { "Oacute" , "\323" },
        { "Uacute" , "\332" }, 
#endif
	{ 0,	0 }  /* Terminate list */
};

static attr no_attr[] = {{ 0, 0 , 0}};

static attr a_attr[] = {				/* Anchor attributes */
#define A_ID 0
	{ "NAME", 0, 0 },				/* Should be ID */
#define A_TYPE 1
	{ "TYPE", 0, 0 },
#define A_HREF 2
	{ "HREF", 0, 0 },
	{ 0, 0 , 0}	/* Terminate list */
};	
static attr list_attr[] = {
#define LIST_COMPACT 0
	{ "COMPACT", 0, 0 },
	{ 0, 0, 0 }	/* Terminate list */
};

static attr glossary_attr[] = {
#define GLOSSARY_COMPACT 0
	{ "COMPACT", 0, 0 },
	{ 0, 0, 0 }	/* Terminate list */
};

static HTTag default_tag =
    { "DOCUMENT", no_attr , 0, 0, begin_document, pass_character, end_document };
/*	NAME ATTR  STYLE LITERAL?  ON_BEGIN   ON__CHARACTER     ON_END
*/
static HTTag tags[] = {
#define TITLE_TAG 0
    { "TITLE", no_attr, 0, 0, clear_string, accumulate_string, set_title },
#define ISINDEX_TAG 1
    { "ISINDEX", no_attr, 0, 0, set_index, 0 , 0 },
#define NEXTID_TAG 2
    { "NEXTID", no_attr, 0, 0, set_next_id, 0, 0 },
#define ADDRESS_TAG 3
    { "ADDRESS"	, no_attr, 0, 0, begin_element, pass_character, end_element },
#define H1_TAG 4
    { "H1"	, no_attr, 0, 0, begin_element, pass_character, end_element },
    { "H2"	, no_attr, 0, 0, begin_element, pass_character, end_element },
    { "H3"	, no_attr, 0, 0, begin_element, pass_character, end_element },
    { "H4"	, no_attr, 0, 0, begin_element, pass_character, end_element },
    { "H5"	, no_attr, 0, 0, begin_element, pass_character, end_element },
    { "H6"	, no_attr, 0, 0, begin_element, pass_character, end_element },
    { "H7"	, no_attr, 0, 0, begin_element, pass_character, end_element },
#define UL_TAG 11
    { "UL"	, list_attr, 0, 0, begin_list, pass_character, end_list },
#define OL_TAG 12
    { "OL"	, list_attr, 0, 0, begin_list, pass_character, end_list },
#define MENU_TAG 13
    { "MENU"	, list_attr, 0, 0, begin_list, pass_character, end_list },
#define DIR_TAG 14
    { "DIR"	, list_attr, 0, 0, begin_list, pass_character, end_list },
#define LI_TAG 15
    { "LI"	, no_attr, 0, 0, list_element, pass_character, 0 },
#define DL_TAG 16
    { "DL"	, list_attr, 0, 0, begin_glossary, pass_character, end_glossary },
    { "DT"	, no_attr, 0, 0, term, pass_character, 0 },
    { "DD"	, no_attr, 0, 0, definition, pass_character, 0 },
    { "A"	, a_attr,  0, 0, begin_anchor, pass_character, end_anchor },
#define P_TAG 20
    { "P"	, no_attr, 0, 0, new_paragraph, pass_character, 0 },
#define XMP_TAG 21
  { "XMP"	, no_attr, 0, YES, begin_litteral, litteral_text, end_element },
#define PRE_TAG 22
  { "PRE"	, no_attr, 0, 0, begin_litteral, litteral_text, end_element },
#define LISTING_TAG 23
  { "LISTING"	, no_attr, 0, YES,begin_litteral, litteral_text, end_element },
#define PLAINTEXT_TAG 24
  { "PLAINTEXT", no_attr, 0, YES, begin_litteral, litteral_text, end_element },
#define COMMENT_TAG 25
    { "COMMENT", no_attr, 0, YES, no_change, ignore_text, no_change },
    { 0, 0, 0, 0,  0, 0 , 0}	/* Terminate list */
};

PUBLIC SGML_dtd HTML_dtd = { tags, &default_tag, entities };


/*		Flattening the style structure
**		------------------------------
**
On the NeXT, and on any read-only browser, it is simpler for the text to have
a sequence of styles, rather than a nested tree of styles. In this
case we have to flatten the structure as it arrives from SGML tags into
a sequence of styles.
*/

/*		If style really needs to be set, call this
*/
PRIVATE void actually_set_style NOARGS
{
    if (!text) {			/* First time through */
	    text = HText_new(node_anchor);
	    HText_beginAppend(text);
	    HText_setStyle(text, new_style);
	    in_word = NO;
    } else {
	    HText_setStyle(text, new_style);
    }
    old_style = new_style;
    style_change = NO;
}

/*      If you THINK you need to change style, call this
*/

PRIVATE void change_style ARGS1(HTStyle *,style)
{
    if (new_style!=style) {
    	style_change = YES /* was old_style == new_style */ ;
	new_style = style;
    }
}

/*	Anchor handling
**	---------------
*/
PRIVATE void begin_anchor ARGS2(HTTag *,t, HTElement *,e)
{
    HTChildAnchor * source = HTAnchor_findChildAndLink(
    	node_anchor,						/* parent */
	a_attr[A_ID].present	? a_attr[A_ID].value : 0,	/* Tag */
	a_attr[A_HREF].present	? a_attr[A_HREF].value : 0,	/* Addresss */
	a_attr[A_TYPE].present	? 
		(HTLinkType*)HTAtom_for(a_attr[A_TYPE].value)
		 : 0);
    
    UPDATE_STYLE;
    HText_beginAnchor(text, source);
}

PRIVATE void end_anchor ARGS2(HTTag *,	 t,
			HTElement *,	e)
{
    UPDATE_STYLE;
    HText_endAnchor(text);
}


/*	General SGML Element Handling
**	-----------------------------
*/
PRIVATE void begin_element ARGS2(HTTag *,t, HTElement *,e)
{
    change_style((HTStyle*)(t->style));
}
PRIVATE void no_change ARGS2(HTTag *,t, HTElement *,e)
{
    /* Do nothing */;
}
PRIVATE void begin_litteral ARGS2(HTTag *,t, HTElement *,e)
{
    change_style(t->style);
    UPDATE_STYLE;
}
/*		End Element
**
**	When we end an element, the style must be returned to that
**	in effect before that element.  Note that anchors (etc?)
**	don't have an associated style, so that we must scan down the
**	stack for an element with a defined style. (In fact, the styles
**	should be linked to the whole stack not just the top one.)
**	TBL 921119
*/
PRIVATE void end_element ARGS2(HTTag *,t, HTElement *,e)
{
 /*   if (e) change_style(e->tag->style); */
    while (e) {
	if (e->tag->style) {
		change_style(e->tag->style);
		return;
	}
	e = e->next;
    }
}

/*			Lists
*/
PRIVATE void begin_list ARGS2(HTTag *,t, HTElement *,e)
{
    change_style(list_attr[LIST_COMPACT].present
    		? list_compact_style
		: (HTStyle*)(t->style));
    in_word = NO;
}

PRIVATE void end_list ARGS2(HTTag *,t, HTElement *,e)
{
    change_style(e->tag->style);
    in_word = NO;
}

PRIVATE void list_element  ARGS2(HTTag *,t, HTElement *,e)
{
    if (e->tag != &tags[DIR_TAG])
	HText_appendParagraph(text);
    else
        HText_appendCharacter(text, '\t');	/* Tab @@ nl for UL? */
    in_word = NO;
}


PRIVATE void begin_glossary ARGS2(HTTag *,t, HTElement *,e)
{
    change_style(glossary_attr[GLOSSARY_COMPACT].present
    		? glossary_compact_style
		: glossary_style);
    in_word = NO;
}

PRIVATE void end_glossary ARGS2(HTTag *,t, HTElement *,e)
{
    change_style(e->tag->style);
    in_word = NO;
}


/*	Begin and End document
**	----------------------
*/
PUBLIC void HTML_begin ARGS1(HTParentAnchor *,anchor)
{
    node_anchor = anchor;
}

PRIVATE void begin_document ARGS2(HTTag *, t, HTElement *, e)
{
    if (!got_styles) get_styles();
    style_change = YES;	/* Force check leading to htext creation */
    text = 0;
    
#ifdef ORIGINAL_CODE
    text = HText_new(node_anchor);
    HText_beginAppend(text);
    HText_setStyle(text, default_tag.style);
    old_style = 0;
    style_change = NO;
    in_word = NO;
#endif
}

PRIVATE void end_document ARGS2(HTTag *, t, HTElement *, e)
/* If the document is empty, the text object will not yet exist.
   So we could in fact abandon creating the document and return
   an error code.  In fact an empty document is an important type
   of document, so we don't.
*/
{
    UPDATE_STYLE;		/* Create empty document here! */
    HText_endAppend(text);

}

/*	Get Styles from style sheet
**	---------------------------
*/
PRIVATE void get_styles NOARGS
{
    got_styles = YES;
    
    tags[P_TAG].style =
    default_tag.style =		HTStyleNamed(styleSheet, "Normal");
    tags[H1_TAG].style =	HTStyleNamed(styleSheet, "Heading1");
    tags[H1_TAG+1].style =	HTStyleNamed(styleSheet, "Heading2");
    tags[H1_TAG+2].style =	HTStyleNamed(styleSheet, "Heading3");
    tags[H1_TAG+3].style =	HTStyleNamed(styleSheet, "Heading4");
    tags[H1_TAG+4].style =	HTStyleNamed(styleSheet, "Heading5");
    tags[H1_TAG+5].style =	HTStyleNamed(styleSheet, "Heading6");
    tags[H1_TAG+6].style =	HTStyleNamed(styleSheet, "Heading7");
    tags[DL_TAG].style =	HTStyleNamed(styleSheet, "Glossary");
    tags[UL_TAG].style =	HTStyleNamed(styleSheet, "List");
    tags[OL_TAG].style =	HTStyleNamed(styleSheet, "List");
    tags[MENU_TAG].style =	HTStyleNamed(styleSheet, "Menu");
    list_compact_style =
    tags[DIR_TAG].style =	HTStyleNamed(styleSheet, "Dir");    
    glossary_style =		HTStyleNamed(styleSheet, "Glossary");
    glossary_compact_style =	HTStyleNamed(styleSheet, "GlossaryCompact");
    tags[ADDRESS_TAG].style=	HTStyleNamed(styleSheet, "Address");
    tags[PLAINTEXT_TAG].style =
    tags[XMP_TAG].style =	HTStyleNamed(styleSheet, "Example");
    tags[PRE_TAG].style =	HTStyleNamed(styleSheet, "Preformatted");
    tags[LISTING_TAG].style =	HTStyleNamed(styleSheet, "Listing");
}


/*	Parse an HTML file
**	------------------
**
**	This version takes a pointer to the routine to call
**	to get each character.
*/
BOOL HTML_Parse
#ifdef __STDC__
  (HTParentAnchor * anchor, char (*next_char)() )
#else
  (anchor, next_char)
    HTParentAnchor * anchor;
    char (*next_char)();
#endif
{
	HTSGMLContext context;
        HTML_begin(anchor);
	context = SGML_begin(&HTML_dtd);
	for(;;) {
	    char character;
	    character = (*next_char)();
	    if (character == (char)EOF) break;
    
	    SGML_character(context, character);           
         }
	SGML_end(context);
	return YES;
}
