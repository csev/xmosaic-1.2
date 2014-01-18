/****************************************************************************
 * NCSA Mosaic for the X Window System                                      *
 * Software Development Group                                               *
 * National Center for Supercomputing Applications                          *
 * University of Illinois at Urbana-Champaign                               *
 * 605 E. Springfield, Champaign IL 61820                                   *
 * mosaic@ncsa.uiuc.edu                                                     *
 *                                                                          *
 * Copyright (C) 1993, Board of Trustees of the University of Illinois      *
 *                                                                          *
 * NCSA Mosaic software, both binary and source (hereafter, Software) is    *
 * copyrighted by The Board of Trustees of the University of Illinois       *
 * (UI), and ownership remains with the UI.                                 *
 *                                                                          *
 * The UI grants you (hereafter, Licensee) a license to use the Software    *
 * for academic, research and internal business purposes only, without a    *
 * fee.  Licensee may distribute the binary and source code (if released)   *
 * to third parties provided that the copyright notice and this statement   *
 * appears on all copies and that no charge is associated with such         *
 * copies.                                                                  *
 *                                                                          *
 * Licensee may make derivative works.  However, if Licensee distributes    *
 * any derivative work based on or derived from the Software, then          *
 * Licensee will (1) notify NCSA regarding its distribution of the          *
 * derivative work, and (2) clearly notify users that such derivative       *
 * work is a modified version and not the original NCSA Mosaic              *
 * distributed by the UI.                                                   *
 *                                                                          *
 * Any Licensee wishing to make commercial use of the Software should       *
 * contact the UI, c/o NCSA, to negotiate an appropriate license for such   *
 * commercial use.  Commercial use includes (1) integration of all or       *
 * part of the source code into a product for sale or license by or on      *
 * behalf of Licensee to third parties, or (2) distribution of the binary   *
 * code or source code to third parties that need it to utilize a           *
 * commercial product sold or licensed by or on behalf of Licensee.         *
 *                                                                          *
 * UI MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR   *
 * ANY PURPOSE.  IT IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED          *
 * WARRANTY.  THE UI SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY THE    *
 * USERS OF THIS SOFTWARE.                                                  *
 *                                                                          *
 * By using or copying this Software, Licensee agrees to abide by the       *
 * copyright law and all other applicable laws of the U.S. including, but   *
 * not limited to, export control laws, and the terms of this license.      *
 * UI shall have the right to terminate this license immediately by         *
 * written notice upon Licensee's breach of, or non-compliance with, any    *
 * of its terms.  Licensee may be held legally responsible for any          *
 * copyright infringement that is caused or encouraged by Licensee's        *
 * failure to abide by the terms of this license.                           *
 *                                                                          *
 * Comments and questions are welcome and can be sent to                    *
 * mosaic-x@ncsa.uiuc.edu.                                                  *
 ****************************************************************************/

#include "mosaic.h"
#include <ctype.h>

/* Grumble grumble... */
#ifdef __sgi
#define __STDC__
#endif

#include "libwww/HTUtils.h"
#include "libwww/HTString.h"
#include "libwww/tcp.h"
#include "libwww/WWW.h"
#include "libwww/HTTCP.h"
#include "libwww/HTAnchor.h"
#include "libwww/HTParse.h"
#include "libwww/HTAccess.h"
#include "libwww/HTHistory.h"
#include "libwww/HTML.h"
#include "libwww/HText.h"
#include "libwww/HTList.h"
#include "libwww/HTStyle.h"

#define MO_BUFFER_SIZE 8192

/* Bare minimum. */
struct _HText {
  HTParentAnchor *node_anchor;
  char *expandedAddress;
  char *simpleAddress;

  /* This is what we should parse and display; it is *not*
     safe to free. */
  char *htmlSrc;
  /* This is what we should free. */
  char *htmlSrcHead;
  int srcalloc;    /* amount of space allocated */
  int srclen;      /* amount of space used */
};

HText* HTMainText = 0;                  /* Equivalent of main window */
HTParentAnchor* HTMainAnchor = 0;       /* Anchor for HTMainText */
HTStyleSheet *styleSheet;               /* Default or overridden */

int bypass_format_handling = 0;         /* hook to switch off www-lib value-add */
int force_dump_to_file = 0;             /* hook to force dumping binary data
                                           straight to file named by... */
char *force_dump_filename = 0;          /* this filename. */

extern Widget toplevel;

static char *hack_htmlsrc (void)
{
  /* Keep pointer to real head of htmlSrc memory block. */
  HTMainText->htmlSrcHead = HTMainText->htmlSrc;
  
  /* OK, bigtime hack -- again -- sigh, I really gotta stop doing this.
     First, we make sure that <plaintext> isn't repeated, or improperly
     applied to the result of a WAIS query (which has an immediate <title>
     tag). */
  if (HTMainText->htmlSrc && HTMainText->srclen > 30)
    {
      if (!strncmp (HTMainText->htmlSrc, "<plaintext>", 11) ||
          !strncmp (HTMainText->htmlSrc, "<PLAINTEXT>", 11))
        {
          if (!strncmp (HTMainText->htmlSrc + 11, "<plaintext>", 11) ||
              !strncmp (HTMainText->htmlSrc + 11, "<PLAINTEXT>", 11))
            {
              HTMainText->htmlSrc += 11;
            }
          else if (!strncmp (HTMainText->htmlSrc + 11, "\n<plaintext>", 12) ||
                   !strncmp (HTMainText->htmlSrc + 11, "\n<PLAINTEXT>", 12))
            {
              HTMainText->htmlSrc += 12;
            }
          else if (!strncmp (HTMainText->htmlSrc + 11, "\n<title>", 8) ||
                   !strncmp (HTMainText->htmlSrc + 11, "\n<TITLE>", 8))
            {
              HTMainText->htmlSrc += 12;
            }
          /* Catch CERN hytelnet gateway, etc. */
          else if (!strncmp (HTMainText->htmlSrc + 11, "\n<HEAD>", 7) ||
                   !strncmp (HTMainText->htmlSrc + 11, "\n<head>", 7) ||
                   !strncmp (HTMainText->htmlSrc + 11, "\n<HTML>", 7) ||
                   !strncmp (HTMainText->htmlSrc + 11, "\n<html>", 7))
            {
              HTMainText->htmlSrc += 12;
            }
        }
    }
  return HTMainText->htmlSrc;
}

/* Given a url, a flag to indicate if this is the first url
   to be loaded in this context, and a pointer to string that
   should be set to represent the head of the allocated text,
   this returns the head of the displayable text. */
static char *doit (char *url, int first, char **texthead)
{
  char *msg;

  /* Eric -- here's where you should probably hook in. */
#ifdef HAVE_DMF
  {
    char *tag;
    
    tag = HTParse(url, "", PARSE_ACCESS);
    if ((tag != NULL)&&(strcmp(tag, "dmfdb") == 0))
      {
        char *addr;
        char *path;
        char *text;
        
        addr = HTParse(url, "", PARSE_HOST);
        path = HTParse(url, "", PARSE_PATH);
        
        text = DMFAnchor(tag, addr, path);
        if (text != NULL)
          {
            *texthead = text;
            return(text);
          }
      }
    else if ((tag != NULL)&&(strcmp(tag, "dmffield") == 0))
      {
        char *addr;
        char *path;
        char *text;
        
        addr = HTParse(url, "", PARSE_HOST);
        path = HTParse(url, "", PARSE_PATH);
        
        text = DMFField(tag, addr, path);
        if (text != NULL)
          {
            *texthead = text;
            return(text);
          }
      }
    else if ((tag != NULL)&&(strcmp(tag, "dmfquery") == 0))
      {
        char *addr;
        char *path;
        char *text;
        
        addr = HTParse(url, "", PARSE_HOST);
        path = HTParse(url, "", PARSE_PATH);
        
        text = DMFQuery(tag, addr, path);
        if (text != NULL)
          {
            *texthead = text;
            return(text);
          }
      }
    else if ((tag != NULL)&&(strcmp(tag, "dmfinquire") == 0))
      {
        char *addr;
        char *path;
        char *text;
        
        addr = HTParse(url, "", PARSE_HOST);
        path = HTParse(url, "", PARSE_PATH);
        
        text = DMFInquire(tag, addr, path);
        if (text != NULL)
          {
            *texthead = text;
            return(text);
          }
      }
    else if ((tag != NULL)&&(strcmp(tag, "dmfitem") == 0))
      {
        char *addr;
        char *path;
        char *text;
        
        addr = HTParse(url, "", PARSE_HOST);
        path = HTParse(url, "", PARSE_PATH);
        
        text = DMFItem(tag, addr, path);
        if (text != NULL)
          {
            *texthead = text;
            return(text);
          }
        else
          {
            *texthead = NULL;
            return(NULL);
          }
      }
  }
#endif /* HAVE_DMF */

  if (first)
    {
      if (HTLoadAbsolute (url, 0))
        {
          char *txt = hack_htmlsrc ();
          *texthead = HTMainText->htmlSrcHead;
          return txt;
        }
    }
  else
    {
      if (HTLoadRelative (url))
        {
          char *txt = hack_htmlsrc ();
          *texthead = HTMainText->htmlSrcHead;
          return txt;
        }
    }

  msg = (char *)malloc ((strlen (url) + 200) * sizeof (char));
  sprintf (msg, "<H1>ERROR</H1> Requested document (URL %s) could not be accessed.<p>The information server either is not accessible or is refusing to serve the document to you.<p>", url);
  *texthead = msg;
  return msg;
}

char *mo_pull_er_over (char *url, char **texthead)
{
  return doit (url, 0, texthead);
}

char *mo_pull_er_over_first (char *url, char **texthead)
{
  return doit (url, 1, texthead);
}

/* Don't let the WWW library play games with formats. */
/* May want to strip off leading <plaintext>'s here also. */
mo_status mo_pull_er_over_virgin (char *url, char *fnam)
{
  /* This was a mistake, but it's still in here at the moment. */
  bypass_format_handling = 1;
  
  /* Force dump to file. */
  force_dump_to_file = 1;
  force_dump_filename = fnam;

  if (HTLoadRelative (url))
    {
      bypass_format_handling = 0;
      force_dump_to_file = 0;
      return mo_succeed;
    }
  else
    {
      bypass_format_handling = 0;
      force_dump_to_file = 0;
      return mo_fail;
    }  
}

HText *HText_new (HTParentAnchor *anchor)
{
  HText *htObj = (HText *)malloc (sizeof (HText));

  htObj->expandedAddress = NULL;
  htObj->simpleAddress = NULL;
  htObj->htmlSrc = NULL;
  htObj->htmlSrcHead = NULL;
  htObj->srcalloc = 0;
  htObj->srclen = 0;

  htObj->node_anchor = anchor;
  
  HTMainText = htObj;
  HTMainAnchor = anchor;

  return htObj;
}

void HText_free (HText *self)
{
  if (self->htmlSrcHead)
    free (self->htmlSrcHead);
  free (self);
  return;
}

void HText_beginAppend (HText *text)
{
  HTMainText = text;
  return;
}

void HText_endAppend (HText *text)
{
  HText_appendCharacter (text, '\0');
  HTMainText = text;
  return;
}

void HText_setStyle (HText *text, HTStyle *style)
{
  return;
}

static void new_chunk (HText *text)
{
  if (text->srcalloc == 0)
    {
      text->htmlSrc = (char *)malloc (MO_BUFFER_SIZE);
      text->htmlSrc[0] = '\0';
    }
  else
    {
      text->htmlSrc = (char *)realloc
        (text->htmlSrc, text->srcalloc + MO_BUFFER_SIZE);
    }

  text->srcalloc += MO_BUFFER_SIZE;

  return;
}

#ifdef __alpha
void HText_appendCharacter (text, ch)
HText *text;
char ch;
#else
void HText_appendCharacter (HText *text, char ch)
#endif
{
  if (text->srcalloc < text->srclen + 1 + 16)
    new_chunk (text);
  
  text->htmlSrc[text->srclen++] = ch;

  return;
}

void HText_appendText (HText *text, const char *str)
{
  int len;

  if (!str)
    return;

  len = strlen (str);

  if (text->srcalloc < text->srclen + len + 16)
    new_chunk (text);

  bcopy (str, (text->htmlSrc + text->srclen), len);

  text->srclen += len;
  text->htmlSrc[text->srclen] = '\0';

  return;
}

void HText_appendParagraph (HText *text)
{
  /* Boy, talk about a misnamed function. */
  char *str = " <p> \n";

  HText_appendText (text, str);

  return;
}

void HText_beginAnchor (HText *text, HTChildAnchor *anc)
{
  return;
}

void HText_endAnchor (HText * text)
{
  return;
}

void HText_dump (HText *me)
{
  return;
}

HTParentAnchor *HText_nodeAnchor (HText *me)
{
  return me->node_anchor;
}

char *HText_getText (HText *me)
{
  return me->htmlSrc;
}

BOOL HText_select (HText *text)
{
  return 0;
}

BOOL HText_selectAnchor (HText *text, HTChildAnchor *anchor)
{
  return 0;
}

#ifndef L_tmpnam
#define L_tmpnam 32
#endif
char *mo_tmpnam (void)
{
  char *tmp = (char *)malloc (sizeof (char) * L_tmpnam);
  tmpnam (tmp);

  if (!Rdata.tmp_directory)
    {
      /* Fast path. */
      return tmp;
    }
  else
    {
      /* OK, reconstruct to go in the directory of our choice. */
      char *oldtmp = tmp;
      int i;

      /* Start at the back and work our way forward. */
      for (i = strlen(oldtmp)-1; i >= 0; i--)
        {
          if (oldtmp[i] == '/')
            goto found_it;
        }
      
      /* No luck, just punt. */
      return tmp;

    found_it:
      tmp = (char *)malloc (sizeof (char) * (strlen (Rdata.tmp_directory) + 
                                             strlen (&(oldtmp[i])) + 8));
      if (Rdata.tmp_directory[strlen(Rdata.tmp_directory)-1] == '/')
        {
          /* Trailing slash in tmp_directory spec. */
          sprintf (tmp, "%s%s", Rdata.tmp_directory, &(oldtmp[i])+1);
        }
      else
        {
          /* No trailing slash. */
          sprintf (tmp, "%s%s", Rdata.tmp_directory, &(oldtmp[i]));
        }

      /* fprintf (stderr, "Pieced together '%s'\n", tmp); */
      
      free (oldtmp);
      return tmp;
    }
}

/* Grumble grumble... */
#if defined(ultrix) || defined(VMS) || defined(NeXT) || defined(M4310) || defined(vax)
char *strdup (char *str)
{
  char *dup;

  dup = (char *)malloc (strlen (str) + 1);
  dup = strcpy (dup, str);

  return dup;
}
#endif

#if defined(NeXT)
char *getcwd (char *pathname)
{
  return (getwd(pathname));
}
#endif

/* Feedback from the library. */
void application_user_feedback (char *str)
{
  XmxMakeInfoDialog (toplevel, str, "NCSA Mosaic: Application Feedback");
  XmxManageRemanage (Xmx_w);
}

char *mo_get_html_return (char **texthead)
{
  char *txt = hack_htmlsrc ();
  *texthead = HTMainText->htmlSrcHead;
  return txt;
}

/* Simply loop through a string and convert all newlines to spaces. */
char *mo_convert_newlines_to_spaces (char *str)
{
  int i;

  if (!str)
    return NULL;

  for (i = 0; i < strlen (str); i++)
    if (str[i] == '\n')
      str[i] = ' ';

  return str;
}

