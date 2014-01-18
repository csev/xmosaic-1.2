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
#include <time.h>
#include <pwd.h>
#include <sys/types.h>
#include "libhtmlw/HTML.h"

#include "history.xbm"

/* ------------------------------------------------------------------------ */
/* ----------------------------- HISTORY LIST ----------------------------- */
/* ------------------------------------------------------------------------ */

/* ---------------------------- kill functions ---------------------------- */

/* Free the data contained in an mo_node.  Currently we only free
   the text itself. */
mo_status mo_free_node_data (mo_node *node)
{
  if (node->texthead != NULL)
    free (node->texthead);
  return mo_succeed;
}

/* Kill a single mo_node associated with a given mo_window; it
   the history list exists we delete it from that.  In any case
   we call mo_free_node_data and return. */
mo_status mo_kill_node (mo_window *win, mo_node *node)
{
  if (win->history_list)
    XmListDeletePos (win->history_list, node->position);
  mo_free_node_data (node);

  return mo_succeed;
}

/* Iterate through all descendents of an mo_node, but not the given
   mo_node itself, and kill them.  This is equivalent to calling
   mo_kill_node on each of those nodes, except this is faster since
   all the Motif list entries can be killed at once. */
mo_status mo_kill_node_descendents (mo_window *win, mo_node *node)
{
  mo_node *foo;
  int count = 0;

  if (node == NULL)
    return 0;
  for (foo = node->next; foo != NULL; foo = foo->next)
    {
      mo_free_node_data (foo);
      count++;
    }

  /* Free count number of items from the end of the list... */
  if (win->history_list && count)
    {
      XmListDeleteItemsPos (win->history_list, count, node->position + 1);
    }

  return 0;
}

/* ------------------------ mo_add_node_to_history ------------------------ */

/* Called from mo_record_visit to insert an mo_node into the history
   list of an mo_window. */
mo_status mo_add_node_to_history (mo_window *win, mo_node *node)
{
  /* If there is no current node, this is our first time through. */
  if (win->history == NULL)
    {
      win->history = node;
      node->previous = NULL;
      node->next = NULL;
      node->position = 1;
      win->current_node = node;
    }
  else
    {
      /* Node becomes end of history list. */
      /* Point back at current node. */
      node->previous = win->current_node;
      /* Point forward to nothing. */
      node->next = NULL;
      node->position = node->previous->position + 1;
      /* Kill descendents of current node, since we'll never
         be able to go forward to them again. */
      mo_kill_node_descendents (win, win->current_node);
      /* Current node points forward to this. */
      win->current_node->next = node;
      /* Current node now becomes new node. */
      win->current_node = node;
    }

  if (win->history_list)
    {
      XmListAddItemUnselected 
        (win->history_list, 
         XmxMakeXmstrFromString (Rdata.display_urls_not_titles ? 
                                 node->url : node->title), 
         node->position);
    }

  return mo_succeed;
}

/* ---------------------------- mo_grok_title ----------------------------- */

/* Make up an appropriate title for a document that does not otherwise
   have one associated with it. */
static char *mo_grok_alternate_title (char *url, char *ref)
{
  char *title, *foo1, *foo2;

  if (!strncmp (url, "gopher:", 7))
    {
      /* It's a gopher server. */
      /* Do we have a ref? */
      if (ref)
        {
          title = strdup (ref);
          goto done;
        }
      else
        {
          /* Nope, no ref.  Make up a title. */
          foo1 = url + 9;
          foo2 = strstr (foo1, ":");
          /* If there's a trailing colon (always should be.. ??)... */
          if (foo2)
            {
              char *server = (char *) malloc ((foo2 - foo1 + 2));
              
              bcopy (foo1, server, (foo2 - foo1));
              server[(foo2 - foo1)] = '\0';
              
              title = (char *) malloc ((strlen (server) + 32) * sizeof (char));
              sprintf (title, "Gopher server at %s\0", server);
              
              /* OK, we got a title... */
              free (server);

              goto done;
            }
          else
            {
              /* Aw hell... */
              title = strdup ("Gopher server");
              goto done;
            }
        }
    }

  /* If we got here, assume we should use 'ref' if possible
     for the WAIS title. */
  if (!strncmp (url, "wais:", 5) || 
      !strncmp (url, "http://info.cern.ch:8001/", 25) ||
      !strncmp (url, "http://info.cern.ch.:8001/", 26) ||
      !strncmp (url, "http://www.ncsa.uiuc.edu:8001/", 30))
    {
      /* It's a WAIS server. */
      /* Do we have a ref? */
      if (ref)
        {
          title = strdup (ref);
          goto done;
        }
      else
        {
          /* Nope, no ref.  Make up a title. */
          foo1 = url + 7;
          foo2 = strstr (foo1, ":");
          /* If there's a trailing colon (always should be.. ??)... */
          if (foo2)
            {
              char *server = (char *) malloc ((foo2 - foo1 + 2));
              
              bcopy (foo1, server, (foo2 - foo1));
              server[(foo2 - foo1)] = '\0';
              
              title = (char *) malloc ((strlen (server) + 32) * sizeof (char));
              sprintf (title, "WAIS server at %s\0", server);
              
              /* OK, we got a title... */
              free (server);

              goto done;
            }
          else
            {
              /* Aw hell... */
              title = strdup ("WAIS server");
              goto done;
            }
        }
    }

  if (!strncmp (url, "news:", 5))
    {
      /* It's a news source. */
      if (strstr (url, "@"))
        {
          /* It's a news article. */
          foo1 = url + 5;
          
          title = (char *)malloc ((strlen (foo1) + 32) * sizeof (char));
          sprintf (title, "USENET article %s\0", foo1);

          goto done;
        }
      else
        {
          /* It's a newsgroup. */
          foo1 = url + 5;
          
          title = (char *)malloc ((strlen (foo1) + 32) * sizeof (char));
          sprintf (title, "USENET newsgroup %s\0", foo1);

          goto done;
        }
    }

  if (!strncmp (url, "file:", 5))
    {
      /* It's a file. */
      if (!strstr (url, "file://"))
        {
          /* It's a local file. */
          foo1 = url + 5;
          
          title = (char *)malloc ((strlen (foo1) + 32) * sizeof (char));
          sprintf (title, "Local file %s\0", foo1);
          
          goto done;
        }
      else
        {
          /* It's a remote file. */
          foo1 = url + 7;
          
          title = (char *)malloc ((strlen (foo1) + 32) * sizeof (char));
          sprintf (title, "Remote file %s\0", foo1);
          
          goto done;
        }
    }
  
  if (!strncmp (url, "ftp:", 4))
    {
      /* It's a file. */
      if (!strstr (url, "ftp://"))
        {
          /* It's a local file. */
          foo1 = url + 4;
          
          title = (char *)malloc ((strlen (foo1) + 32) * sizeof (char));
          sprintf (title, "Local file %s\0", foo1);
          
          goto done;
        }
      else
        {
          /* It's a remote file. */
          foo1 = url + 6;
          
          title = (char *)malloc ((strlen (foo1) + 32) * sizeof (char));
          sprintf (title, "Remote file %s\0", foo1);
          
          goto done;
        }
    }
  
  /* Punt... */
  title = (char *) malloc ((strlen (url) + 24) * sizeof (char));
  sprintf (title, "Untitled, URL %s\0", url);
  
 done:
  return title;
}

/* Figure out a title for the given URL.  'ref', if it exists,
   was the text used for the anchor that pointed us to this URL;
   it is not required to exist. */
char *mo_grok_title (mo_window *win, char *url, char *ref)
{
  char *title = NULL, *t;

  XtVaGetValues (win->view, WbNtitleText, &title, NULL);
  if (!title)
    t = mo_grok_alternate_title (url, ref);
  else if (!strcmp (title, "Document"))
    t = mo_grok_alternate_title (url, ref);
  else
    t = strdup (title);

  mo_convert_newlines_to_spaces (t);

  return t;
}

/* --------------------------- mo_record_visit ---------------------------- */

/* Called when we visit a new node (as opposed to backing up or
   going forward).  Create an mo_node entry, call mo_grok_title
   to figure out what the title is, and call mo_node_to_history
   to add the new mo_node to both the window's data structures and
   to its Motif history list. */
mo_status mo_record_visit (mo_window *win, char *url, char *newtext, 
                           char *newtexthead, char *ref)
{
  mo_node *node = (mo_node *)malloc (sizeof (mo_node));
  node->url = url;
  node->text = newtext;
  node->texthead = newtexthead;
  node->ref = ref;
  /* Figure out what the title is... */
  node->title = mo_grok_title (win, url, ref);
  
  /* This will be recalc'd when we leave this node. */
  node->scrpos = -1;
  node->docid = 1;

  mo_add_node_to_history (win, node);

  return mo_succeed;
}

/* ------------------------- navigation functions ------------------------- */

/* Back up a node: grab the current scrollbar values, reset
   win->current_node, and call mo_do_window_node. */
mo_status mo_back_node (mo_window *win)
{
  /* If there is no previous node, choke. */
  if (win->current_node->previous == NULL)
    return mo_fail;

  /* Grab the current scrollbar values for the window and the
     current node. */
  mo_snarf_scrollbar_values (win);

  /* Back up. */
  win->current_node = win->current_node->previous;

  /* Set the text. */
  mo_do_window_node (win, win->current_node);
  
  return mo_succeed;
}

/* Go forward a node: grab the current scrollbar values, reset
   win->current_node, and call mo_do_window_node. */
mo_status mo_forward_node (mo_window *win)
{
  /* If there is no next node, choke. */
  if (win->current_node->next == NULL)
    return mo_fail;

  /* Grab the current scrollbar values for the window and the
     current node. */
  mo_snarf_scrollbar_values (win);

  /* Go forward. */
  win->current_node = win->current_node->next;

  /* Set the text. */
  mo_do_window_node (win, win->current_node);
  
  return mo_succeed;
}

/* Visit an arbitrary position.  This is called when a history
   list entry is double-clicked upon.

   Iterate through the window history; find the mo_node associated
   with the given position.  Reset win->current_node and call
   mo_do_window_node. */
mo_status mo_visit_position (mo_window *win, int pos)
{
  mo_node *node;
  
  for (node = win->history; node != NULL; node = node->next)
    {
      if (node->position == pos)
        {
          /* Grab the current scrollbar values for the window and the
             current node. */
          mo_snarf_scrollbar_values (win);
          
          win->current_node = node;
          mo_do_window_node (win, win->current_node);
          goto done;
        }
    }

  fprintf (stderr, "UH OH BOSS, asked for position %d, ain't got it.\n",
           pos);

 done:
  return mo_succeed;
}

/* Visit an arbitrary position.  This is called when a future
   list entry is double-clicked upon. */
mo_status mo_visit_future_position (mo_window *win, int pos)
{
  /* Let's assume we have win->hrefs and win->num_hrefs. */
  /* If we have 10 items and we click on list position 10 (array position 9),
     then we're legal.  If we click on list position 11 somehow, then we're
     greater than num_hrefs and should be shot. */
  if (pos > win->num_hrefs)
    return mo_fail;

  mo_access_document (win, win->hrefs[pos - 1]);

  return mo_succeed;
}

/* ---------------------------- misc functions ---------------------------- */

mo_status mo_dump_history (mo_window *win)
{
  mo_node *node;
  fprintf (stderr, "----------------- history -------------- \n");
  fprintf (stderr, "HISTORY is 0x%08x\n", win->history);
  for (node = win->history; node != NULL; node = node->next)
    {
      fprintf (stderr, "NODE %d %s\n", node->position, node->url);
      fprintf (stderr, "     TITLE %s\n", node->title);
    }
  fprintf (stderr, "CURRENT NODE %d %s\n", win->current_node->position,
           win->current_node->url);
  fprintf (stderr, "----------------- history -------------- \n");
  return mo_succeed;
}  

/* ------------------------------------------------------------------------ */
/* ----------------------------- HISTORY GUI ------------------------------ */
/* ------------------------------------------------------------------------ */

/* We've just init'd a new history list widget; look at the window's
   history and load 'er up. */
static void mo_load_history_list (mo_window *win, Widget list)
{
  mo_node *node;
  
  for (node = win->history; node != NULL; node = node->next)
    {
      XmListAddItemUnselected 
        (list, 
         XmxMakeXmstrFromString (Rdata.display_urls_not_titles ? 
                                 node->url : node->title), 
         0);
    }
  
  XmListSetBottomPos (list, 0);
  XmListSelectPos (win->history_list, win->current_node->position, False);

  return;
}

/* This is called to reload all hrefs in the window's future list.
   We assume win->num_hrefs and win->hrefs have already been loaded. */
mo_status mo_load_future_list (mo_window *win, Widget list)
{
  int i;
  XmString *items;

  /* Kill 'em all. */
  XmListDeleteAllItems (list);

  if (!win->num_hrefs || !win->hrefs)
    {
      XtSetSensitive(list, False);
      return mo_fail;
    }
    else
    {
      XtSetSensitive(list, True);
    }

  items = (XmString *)malloc (win->num_hrefs * sizeof (XmString *));
  for (i = 0; i < win->num_hrefs; i++)
    {
      items[i] = XmxMakeXmstrFromString (win->hrefs[i]);
    }
  XmListAddItems (list, items, win->num_hrefs, 1);

  XmListSetPos (list, 1);

  return mo_succeed;
}

/* ----------------------------- mail history ----------------------------- */

static XmxCallback (mailhist_win_cb)
{
  mo_window *win = mo_fetch_window_by_id 
    (XmxExtractUniqid ((int)client_data));
  char *to, *subj, *fnam, *cmd;
  FILE *fp;

  switch (XmxExtractToken ((int)client_data))
    {
    case 0:
      XtUnmanageChild (win->mailhist_win);

      mo_busy ();

      to = XmxTextGetString (win->mailhist_to_text);
      if (!to)
        return;
      if (to[0] == '\0')
        return;

      subj = XmxTextGetString (win->mailhist_subj_text);

      fnam = mo_tmpnam();

      fp = fopen (fnam, "w");
      if (!fp)
        goto oops;

      {
        mo_node *node;
        struct passwd *pw = getpwuid (getuid ());
        char *author;
  
        if (Rdata.default_author_name)
          author = Rdata.default_author_name;
        else
          author = pw->pw_gecos;
        
        fprintf (fp, "<H1>History Path From %s</H1>\n", author);
        fprintf (fp, "<DL>\n");
        for (node = win->history; node != NULL; node = node->next)
          {
            fprintf (fp, "<DT>%s\n<DD><A HREF=\"%s\">%s</A>\n", 
                     node->title, node->url, node->url);
          }
        fprintf (fp, "</DL>\n");
      }
        
      fclose (fp);

      cmd = (char *)malloc 
        ((strlen (Rdata.mail_command) + strlen (subj) + strlen (to)
                             + strlen (fnam) + 24));
      sprintf (cmd, "%s -s \"%s\" %s < %s", 
               Rdata.mail_command, subj, to, fnam);
      if ((system (cmd)) != 0)
        {
          XmxMakeErrorDialog 
            (win->base,
             "Unable to mail window history;\ncheck the value of the X resource\nmailCommand.", 
             "Mail History Error");
          XtManageChild (Xmx_w);
        }
      free (cmd);

    oops:
      free (to);
      free (subj);

      cmd = (char *)malloc ((strlen (fnam) + 32) * sizeof (char));
      sprintf (cmd, "/bin/rm -f %s &", fnam);
      system (cmd);

      free (fnam);
      free (cmd);

      mo_not_busy ();
            
      break;
    case 1:
      XtUnmanageChild (win->mailhist_win);
      /* Do nothing. */
      break;
    case 2:
      mo_open_another_window
        (win, 
         "http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/Docs/help-on-hotlist-view.html", 
         NULL, NULL);
      break;
    }

  return;
}

static mo_status mo_post_mailhist_win (mo_window *win)
{
  /* This shouldn't happen. */
  if (!win->history_win)
    return mo_fail;

  if (!win->mailhist_win)
    {
      Widget dialog_frame;
      Widget dialog_sep, buttons_form;
      Widget mailhist_form, to_label, subj_label;
      
      /* Create it for the first time. */
      XmxSetUniqid (win->id);
      win->mailhist_win = XmxMakeFormDialog 
        (win->history_win, "NCSA Mosaic: Mail Window History");
      dialog_frame = XmxMakeFrame (win->mailhist_win, XmxShadowOut);

      /* Constraints for base. */
      XmxSetConstraints 
        (dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
         XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
      
      /* Main form. */
      mailhist_form = XmxMakeForm (dialog_frame);
      
      to_label = XmxMakeLabel (mailhist_form, "Mail To: ");
      XmxSetArg (XmNwidth, 335);
      win->mailhist_to_text = XmxMakeTextField (mailhist_form);
      
      subj_label = XmxMakeLabel (mailhist_form, "Subject: ");
      win->mailhist_subj_text = XmxMakeTextField (mailhist_form);

      dialog_sep = XmxMakeHorizontalSeparator (mailhist_form);
      
      buttons_form = XmxMakeFormAndThreeButtons
        (mailhist_form, mailhist_win_cb, "Mail", "Dismiss", "Help...", 0, 1, 2);

      /* Constraints for mailhist_form. */
      XmxSetOffsets (to_label, 14, 0, 10, 0);
      XmxSetConstraints
        (to_label, XmATTACH_FORM, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_NONE,
         NULL, NULL, NULL, NULL);
      XmxSetOffsets (win->mailhist_to_text, 10, 0, 5, 10);
      XmxSetConstraints
        (win->mailhist_to_text, XmATTACH_FORM, XmATTACH_NONE, XmATTACH_WIDGET,
         XmATTACH_FORM, NULL, NULL, to_label, NULL);

      XmxSetOffsets (subj_label, 14, 0, 10, 0);
      XmxSetConstraints
        (subj_label, XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_FORM, 
         XmATTACH_NONE,
         win->mailhist_to_text, NULL, NULL, NULL);
      XmxSetOffsets (win->mailhist_subj_text, 10, 0, 5, 10);
      XmxSetConstraints
        (win->mailhist_subj_text, XmATTACH_WIDGET, XmATTACH_NONE, 
         XmATTACH_WIDGET,
         XmATTACH_FORM, win->mailhist_to_text, NULL, subj_label, NULL);

      XmxSetArg (XmNtopOffset, 10);
      XmxSetConstraints 
        (dialog_sep, XmATTACH_WIDGET, XmATTACH_WIDGET, XmATTACH_FORM, 
         XmATTACH_FORM,
         win->mailhist_subj_text, buttons_form, NULL, NULL);
      XmxSetConstraints 
        (buttons_form, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_FORM, 
         XmATTACH_FORM,
         NULL, NULL, NULL, NULL);
    }
  
  XtManageChild (win->mailhist_win);
  
  return mo_succeed;
}

/* ---------------------------- history_win_cb ---------------------------- */

static XmxCallback (history_win_cb)
{
  mo_window *win = mo_fetch_window_by_id (XmxExtractUniqid ((int)client_data));

  switch (XmxExtractToken ((int)client_data))
    {
    case 0:
      XtUnmanageChild (win->history_win);
      /* Dismissed -- do nothing. */
      break;
    case 1:
      mo_post_mailhist_win (win);
      break;
    case 2:
      mo_open_another_window
        (win, 
         "http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/Docs/docview-menubar-navigate.html#history",
         NULL, NULL);
      break;
    }

  return;
}

static XmxCallback (history_list_cb)
{
  mo_window *win = mo_fetch_window_by_id (XmxExtractUniqid ((int)client_data));
  XmListCallbackStruct *cs = (XmListCallbackStruct *)call_data;
  
  mo_visit_position (win, cs->item_position);

  return;
}

static XmxCallback (future_list_cb)
{
  mo_window *win = mo_fetch_window_by_id (XmxExtractUniqid ((int)client_data));
  XmListCallbackStruct *cs = (XmListCallbackStruct *)call_data;
  
  mo_visit_future_position (win, cs->item_position);

  return;
}

mo_status mo_post_history_win (mo_window *win)
{
  if (!win->history_win)
    {
      Widget dialog_frame;
      Widget dialog_sep, buttons_form;
      Widget history_label;
      Widget history_form;
      Widget future_label;
      Widget logo;
      XtTranslations listTable;
      static char listTranslations[] =
	"~Shift ~Ctrl ~Meta ~Alt <Btn2Down>: ListKbdSelectAll() ListBeginSelect() \n\
	 ~Shift ~Ctrl ~Meta ~Alt <Btn2Up>:   ListEndSelect() ListKbdActivate()";

      listTable = XtParseTranslationTable(listTranslations);
      
      /* Create it for the first time. */
      XmxSetUniqid (win->id);
      win->history_win = XmxMakeFormDialog 
        (win->base, "NCSA Mosaic: Window History");
      dialog_frame = XmxMakeFrame (win->history_win, XmxShadowOut);

      /* Constraints for base. */
      XmxSetConstraints 
        (dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
         XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
      
      /* Main form. */
      history_form = XmxMakeForm (dialog_frame);

      /* This time, label is going to be taller than logo. */
      XmxSetArg (XmNalignment, XmALIGNMENT_BEGINNING);
      history_label = XmxMakeLabel (history_form, "You can double-click on any entry\nin either of the following lists to jump\nto that document.\n    \nWhere you've been:");

      logo = XmxMakeNamedLabel (history_form, NULL, "logo");
      XmxApplyBitmapToLabelWidget
        (logo, history_bits, history_width, history_height);
      
      /* History list itself. */
      XmxSetArg (XmNresizable, False);
      XmxSetArg (XmNscrollBarDisplayPolicy, XmSTATIC);
      XmxSetArg (XmNlistSizePolicy, XmCONSTANT);
      XmxSetArg (XmNwidth, 380);
      XmxSetArg (XmNheight, 184);
      win->history_list = XmxMakeScrolledList 
        (history_form, history_list_cb, 0);
      XtAugmentTranslations (win->history_list, listTable);

      future_label = XmxMakeLabel (history_form, "Where you can go:");

      XmxSetArg (XmNresizable, False);
      XmxSetArg (XmNscrollBarDisplayPolicy, XmSTATIC);
      XmxSetArg (XmNlistSizePolicy, XmCONSTANT);
      XmxSetArg (XmNwidth, 380);
      XmxSetArg (XmNheight, 184);
      win->future_list = XmxMakeScrolledList 
        (history_form, future_list_cb, 0);
      XtAugmentTranslations (win->future_list, listTable);

      dialog_sep = XmxMakeHorizontalSeparator (history_form);
      
      buttons_form = XmxMakeFormAndThreeButtons
        (history_form, history_win_cb, "Dismiss", "Mail To...", "Help...", 
         0, 1, 2);

      /* Constraints for history_form. */
      XmxSetOffsets (history_label, 8, 0, 10, 10);
      XmxSetConstraints
        (history_label, XmATTACH_FORM, XmATTACH_NONE, XmATTACH_FORM,
         XmATTACH_NONE, NULL, NULL, NULL, NULL);
      /* Logo: top to form, bottom to nothing, left to nothing,
         right to form. */
      XmxSetOffsets (logo, 8, 0, 0, 4);
      XmxSetConstraints
        (logo, XmATTACH_FORM, XmATTACH_NONE, XmATTACH_NONE, XmATTACH_FORM,
         NULL, NULL, NULL, NULL);
      /* History list is stretchable. */
      XmxSetOffsets (XtParent (win->history_list), 0, 10, 10, 10);
      XmxSetConstraints
        (XtParent (win->history_list), 
         XmATTACH_WIDGET, XmATTACH_WIDGET, XmATTACH_FORM, XmATTACH_FORM, 
         history_label, future_label, NULL, NULL);
      XmxSetOffsets (future_label, 10, 0, 10, 10);
      XmxSetConstraints
        (future_label, XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM,
         XmATTACH_NONE, NULL, XtParent (win->future_list), NULL, NULL);
      XmxSetOffsets (XtParent (win->future_list), 0, 10, 10, 10);
      XmxSetConstraints
        (XtParent (win->future_list), 
         XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM, XmATTACH_FORM, 
         NULL, dialog_sep, NULL, NULL);
      XmxSetArg (XmNtopOffset, 10);
      XmxSetConstraints 
        (dialog_sep, 
         XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM, XmATTACH_FORM,
         NULL, buttons_form, NULL, NULL);
      XmxSetConstraints 
        (buttons_form, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_FORM, 
         XmATTACH_FORM,
         NULL, NULL, NULL, NULL);

      /* Go get the history up to this point set up... */
      mo_load_history_list (win, win->history_list);
      mo_load_future_list (win, win->future_list);
    }

  XtManageChild (win->history_win);
  
  return mo_succeed;
}
