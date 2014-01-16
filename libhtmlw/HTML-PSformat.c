/* HTML-PSformat.c -  Module for NCSA's Mosaic software
 *
 * Purpose:     to parse Hypertext widget contents into appropriate PostScript
 *
 * Author:      Ameet A. Raval (aar@gfdl.gov)
 *
 * Institution: Geophysical Fluid Dynamics Laboratory,
 *              National Oceanic and Atmospheric Administration,
 *              U.S. Department of Commerce
 *              P.O. Box 308
 *              Princeton, NJ 08542
 *
 * Date:        15 April 1993
 *
 * Copyright:   This work is the product of the United States Government,
 *              and is precluded from copyright protection.  It is hereby
 *              released into the public domain.
 */

#ifdef TIMING
#include <sys/time.h>
struct timeval Tv;
struct timezone Tz;
#endif

#include <stdio.h>
#include <ctype.h>
#include "HTMLP.h"


extern int SwapElements();

#define MAXLINENO       66
#define TOP_MARGIN      (10*72)
#define BOT_MARGIN      (1*72)
#define LEFT_MARGIN     (1*72)
#define PS_charBullet   "\\267"
#define L_PAREN         '('
#define R_PAREN         ')'
#define stradd1(bp,f,v) {sprintf(PS_str, f, v), stradd(bp,PS_str);}

typedef enum {
                           TR=0, TI,   TB,   CR
} PS_fontname;
static char fnchar[][3] = {"TR", "TI", "TB", "CR"};
static int PS_curr_fontsize;
static char PS_str[BUFSIZ];
static int PS_curr_ypos;
static int PS_curr_page;

/* stradd - dynamic string concatenation function.
 *
 *  At first invocation (*sp does not point to a valid memory block),
 *  allocates some memory for string s, and initializes it to contents of
 *  string t.
 *
 *  In successive calls, the string t will be appended to s, and if s
 *  needs more space, this will be allocated.
 *
 *  If enough memory cannot be allocated, stradd returns NULL,
 *  otherwise it returns a pointer to the string.
 */

char *
#ifdef _NO_PROTO
stradd(sp, t)
	String *sp;
	String t;
#else
stradd(String *sp, String t)
#endif /* _NO_PROTO */
{
#define MEMBLOCK        1024    /* # of char.s to affix to a growing string */
    static int splen=0;
    int newlen;
    
/* Calculate how much string memory we will need */
    if (*sp==NULL)                      /* First invocation */
        newlen = strlen(t);
    else                                /* Successive invocation */
        newlen = strlen(t) + strlen(*sp);
    
/* Round it up in units of MEMBLOCK */
    while (splen<=newlen)
        splen += MEMBLOCK;

    if (*sp==NULL) {                    /* First invocation */
        *sp = (String) malloc(splen);
        if (*sp) **sp='\0';
    }
    else                                /* Successive invocation */
        *sp = (String) realloc(*sp, splen);
    
    if (*sp)
        strcat(*sp, t);
    else {
        fprintf(stderr, "stradd malloc failed\n");
        return(NULL);
    }
    return(*sp);
#undef MEMBLOCK
}

/* show the current page and restore any changes to the printer state */
void
#ifdef _NO_PROTO
PSshowpage(sp)
	String *sp;
#else
PSshowpage(String *sp)
#endif /* _NO_PROTO */
{
#ifdef ANSI_C
    stradd(sp,
           "SP\n"
           "restore\n");
#else
    stradd(sp, "\
SP\n\
restore\n");
#endif /* ANSI_C */
}

/* begin a fresh page, including the structured comment conventions */
void
#ifdef _NO_PROTO
PSnewpage(sp)
	String *sp;
#else
PSnewpage(String *sp)
#endif /* _NO_PROTO */
{
    void PSfont();

    PS_curr_page ++;

    stradd(sp, "%%Page: ");
    stradd1(sp, "%d\n", PS_curr_page);
#ifdef ANSI_C
    stradd(sp,
           "save\n"
           "N\n");
#else
    stradd(sp, "\
save\n\
N\n");
#endif /* ANSI_C */
    PSfont(sp, NULL, NULL);     /* force re-flush of last font used */
    PS_curr_ypos= TOP_MARGIN;   /* replicates the postscript variable "yp" */
}

/* print out initializing PostScript text */
void
#ifdef _NO_PROTO
PSinit(sp)
	String *sp;
#else
PSinit(String *sp)
#endif /* _NO_PROTO */
{
#ifdef ANSI_C
    stradd (sp,"%!PS-Adobe-1.0\n"
               "%%DocumentFonts: Times-Roman Times-Bold Times-Italic Courier\n"
               "%%Pages: (atend)\n"
               "%%EndComments\n"
               "save\n"
               "/D {def} def\n"
               "/M {fnsiz mul xmargin add yp moveto} D\n"
               "/N {/yp yp fnsiz 2 add sub D xmargin yp moveto} D\n"
               "/S {show} D\n"
               "/SF {/fnsiz exch D findfont fnsiz scalefont setfont} D\n"
               "/TR {/Times-Roman} D\n"
               "/TB {/Times-Bold} D\n"
               "/TI {/Times-Italic} D\n"
               "/CR {/Courier} D\n"
               "/SP {showpage} D\n"
               "/NP {} D\n");
#else
    stradd (sp,"%!PS-Adobe-1.0\n\
%%DocumentFonts: Times-Roman Times-Bold Times-Italic Courier\n\
%%Pages: (atend)\n\
%%EndComments\n\
save\n\
/D {def} def\n\
/M {fnsiz mul xmargin add yp moveto} D\n\
/N {/yp yp fnsiz 2 add sub D xmargin yp moveto} D\n\
/S {show} D\n\
/SF {/fnsiz exch D findfont fnsiz scalefont setfont} D\n\
/TR {/Times-Roman} D\n\
/TB {/Times-Bold} D\n\
/TI {/Times-Italic} D\n\
/CR {/Courier} D\n\
/SP {showpage} D\n\
/NP {} D\n");
#endif /* ANSI_C */
    stradd1(sp,"/xmargin %d D ",        LEFT_MARGIN);
    stradd1(sp,"/yp %d D",              TOP_MARGIN);
#ifdef ANSI_C
    stradd(sp,  "/fnsiz 0 D\n"
		"%%EndProlog\n");
#else
    stradd (sp,"\
/fnsiz 0 D\n\
%%EndProlog\n");
#endif /* ANSI_C */

    PS_curr_page = 0;
    PSnewpage(sp);
}

/* print out trailing PostScript text */
void
#ifdef _NO_PROTO
PSclose(sp)
	String *sp;
#else
PSclose(String *sp)
#endif /* _NO_PROTO */
{
    PSshowpage(sp);
#ifdef ANSI_C
    stradd(sp,
           "%%Trailer\n"
           "restore\n"
           "%%Pages: "
           );
#else
    stradd(sp, "\
%%Trailer\n\
restore\n\
%%Pages: "
           );
#endif /* ANSI_C */
    stradd1(sp, "%d\n", PS_curr_page);
}

/* add commands to move margin to the "tab" position. */
void
#ifdef _NO_PROTO
PSindent(sp, tab)
	String *sp;
	int tab;
#else
PSindent(String *sp, int tab)
#endif /* _NO_PROTO */
{
    stradd1(sp, "%d M\n", tab);
}

/* add commands into buf to show text "t". */
void
#ifdef _NO_PROTO
PStext(sp, t)
	String *sp;
	String t;
#else
PStext(String *sp, String t)
#endif /* _NO_PROTO */
{
    String tp, t2;
    int nparen=0;
    
    tp=t;
/* count # of paren's in text */
    while (*tp != NULL) {
        if (*tp == L_PAREN || *tp != R_PAREN)
            nparen++;
        tp++;
    }
    
/* create t2 to hold original text + "\"'s */
    t2=(String) malloc((tp-t) + nparen + 1);
    
/* for each char in orig string t, if it is a PAREN, insert "\" into the
   new string t2, then insert the actual char
*/
    tp=t2;
    while (*t != NULL) {
        if (*t == L_PAREN || *t == R_PAREN)
            *(tp++) = '\\';
        *(tp++) = *(t++);
    }
    *(tp) = '\0';
    
    stradd(sp, "(");
    stradd(sp, t2);
    stradd(sp, ")S\n");
    free(t2);
}

/* change local font in buf to "font" */
void
#ifdef _NO_PROTO
PSfont(sp, hw, font)
	String *sp;
	HTMLWidget hw;
	XFontStruct *font;
#else
PSfont(String *sp, HTMLWidget hw, XFontStruct *font)
#endif /* _NO_PROTO */
{
    static PS_fontname oldfn=TR;
    static int oldfs=14;
    PS_fontname fn;
    int fs;
    
/* NULL case - reflush old font or the builtin default: */
    if (hw==NULL || font==NULL)                 {fn=oldfn, fs=oldfs, oldfs=0;}
    else if (font == hw->html.font)             {fn=TR, fs=12;}
    else if (font == hw->html.italic_font)      {fn=TI, fs=12;}
    else if (font == hw->html.bold_font)        {fn=TB, fs=12;}
    else if (font == hw->html.fixed_font)       {fn=CR, fs=12;}
    else if (font == hw->html.header1_font)     {fn=TB, fs=20;}
    else if (font == hw->html.header2_font)     {fn=TB, fs=18;}
    else if (font == hw->html.header3_font)     {fn=TB, fs=14;}
    else if (font == hw->html.header4_font)     {fn=TB, fs=12;}
    else if (font == hw->html.header5_font)     {fn=TB, fs=11;}
    else if (font == hw->html.header6_font)     {fn=TB, fs=10;}
    else if (font == hw->html.address_font)     {fn=TI, fs=12;}
    else if (font == hw->html.plain_font)       {fn=CR, fs=12;}

    if (fn != oldfn || fs != oldfs) {
        stradd(sp, fnchar[fn]);
        stradd1(sp, " %d SF\n", fs);
        oldfn=fn, oldfs=fs;
        PS_curr_fontsize=fs;
    }
}

/* add command into buf to move point to left margin, next line;
   if length is approaching pagelength, call "showpage" and begin
   new page.
*/
void
#ifdef _NO_PROTO
PSnewline(sp)
	String *sp;
#else
PSnewline(String *sp)
#endif /* _NO_PROTO */
{
    PS_curr_ypos -= PS_curr_fontsize+2; /* replicates postscript "yp" */
    if (PS_curr_ypos < BOT_MARGIN) {    /* at the bottom margin ? */
        PSshowpage(sp);
        PSnewpage(sp);
    }
    else
        stradd(sp, "N\n");
}


/*
 * Parse all the formatted text elements from start to end
 * into an ascii text string, and return it.
 * Very like ParseTextToString() except the text is prettied up
 * into Postscript (aar - 14 Apr 1993)
 * to show headers and the like.
 * space_width and lmargin tell us how many spaces
 * to indent lines.
 */
String ParseTextToPSString(hw, elist, startp, endp, start_pos, end_pos,
                           space_width, lmargin)
HTMLWidget hw;
struct ele_rec *elist;
struct ele_rec *startp;
struct ele_rec *endp;
int start_pos, end_pos;
int space_width;
int lmargin;
{
    int line;
    int newline, char_count;
    int epos;
    String buf = NULL;
    String *bufp = &buf;
    struct ele_rec *eptr;
    struct ele_rec *start;
    struct ele_rec *end;
    struct ele_rec *last;
    
    if (startp == NULL)
        return(NULL);
    
    if (SwapElements(startp, endp, start_pos, end_pos)) {
        start = endp;
        end = startp;
        epos = start_pos;
        start_pos = end_pos;
        end_pos = epos;
    }
    else {
        start = startp;
        end = endp;
    }
    
    /*
     * We need to know if we should consider the indentation or bullet
     * that might be just before the first selected element to also be
     * selected.  This current hack looks to see if they selected the
     * Whole line, and assumes if they did, they also wanted the beginning.
     *
     * If we are at the beginning of the list, or the beginning of
     * a line, or just behind a bullet, assume this is the start of
     * a line that we may want to include the indent for.
     */
    if ((start_pos == 0) &&
        ((start->prev == NULL) ||
         (start->prev->type == E_BULLET) ||
         (start->prev->line_number != start->line_number)
         ))
        {
            eptr = start;
            while ((eptr != NULL) && (eptr != end) &&
                   (eptr->type != E_LINEFEED))
                eptr = eptr->next;

            if ((eptr != NULL) && (eptr->type == E_LINEFEED))
                newline = 1;
            else
                newline = 0;
        }
    else
        newline = 0;
    
    PSinit(bufp);
    
    last = start;
    eptr = start;
    line = eptr->line_number;
    while ((eptr != NULL) && (eptr != end)) {
/* Skip the special internal text added for multi-page documents.
*/
        if (eptr->internal == True) {
            eptr = eptr->next;
            continue;
        }
        
        switch(eptr->type) {

          case E_BULLET: {
              int i, spaces;
                  
              PSfont(bufp, hw, eptr->font);     /* set font appropriately */
              if (newline) {
                  spaces = (eptr->x - lmargin) / space_width;
                  spaces -= 2;
                  if (spaces < 0) spaces = 0;

                  do {
                      PSnewline(bufp);          /* add newlines */
                  } while (--newline);

                  PSindent(bufp,spaces);        /* tab before the bullet */
                  PStext(bufp,PS_charBullet);   /* insert the bullet */
                  PSindent(bufp, spaces+2);     /* tab after the bullet */
              }
              else
                  PStext(bufp,PS_charBullet);   /* just insert bullet */
              break;
          }

          case E_TEXT: {
              int i, spaces;
              String tptr;
              
              if (eptr == start)
                  tptr = (String)(eptr->edata + start_pos);
              else
                  tptr = (String)eptr->edata;
              
              PSfont(bufp,hw,eptr->font);       /* set font appropriately */
              if (newline) {
                  spaces = (eptr->x - lmargin) / space_width;
                  if (spaces < 0) spaces = 0;

                  do {
                      PSnewline(bufp);          /* add newlines */
                  } while (--newline);

                  PSindent(bufp,spaces);        /* tab to curr. margin */
                  PStext(bufp,tptr);            /* insert text */
              }
              else
                  PStext(bufp,tptr);            /* just insert the text */
              break;
          }
          case E_LINEFEED: {
              newline ++;
              break;
          }
        }
        last = eptr;
        eptr = eptr->next;
    }
    
    PSclose(bufp);
    return(*bufp);
}

