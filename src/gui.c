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

#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#if (XtSpecificationRelase == 4)
extern void _XEditResCheckMessages();
#define EDITRES_SUPPORT
#endif

#if (XtSpecificationRelase > 4)
#define EDITRES_SUPPORT
#endif

#ifdef EDITRES_SUPPORT
#include <X11/Xmu/Editres.h>
#endif

#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#include <Xm/List.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>

#include <Xm/Protocols.h>
#include <X11/keysym.h>

#include "libwww/HTParse.h"
#include "libhtmlw/HTML.h"
#include "xresources.h"

#include "xmosaic.xbm"


/*
 * Because Sun cripples the keysym.h file
 */
#ifndef XK_KP_Up
#define XK_KP_Home              0xFF95  /* Keypad Home */
#define XK_KP_Left              0xFF96  /* Keypad Left Arrow */
#define XK_KP_Up                0xFF97  /* Keypad Up Arrow */
#define XK_KP_Right             0xFF98  /* Keypad Right Arrow */
#define XK_KP_Down              0xFF99  /* Keypad Down Arrow */
#define XK_KP_Prior             0xFF9A  /* Keypad Page Up */
#define XK_KP_Next              0xFF9B  /* Keypad Page Down */
#define XK_KP_End               0xFF9C  /* Keypad End */
#endif


/* ------------------------------ variables ------------------------------- */

Display *dsp;
XtAppContext app_context;
Widget toplevel;
Widget view;  /* HORRIBLE HACK @@@@ */
int Vclass;  /* visual class for 24bit support hack */
AppData Rdata;  /* extern'd in mosaic.h */
char *global_xterm_str;  /* required for HTAccess.c now */
char *gif_reader;
char *hdf_reader;
char *jpeg_reader;
char *tiff_reader;
char *audio_reader;
char *aiff_reader;
char *postscript_reader;
char *dvi_reader;
char *mpeg_reader;
char *mime_reader;
char *xwd_reader;
char *sgimovie_reader;
char *evlmovie_reader;
char *rgb_reader;
char *cave_reader;

char *uncompress_program;
char *gunzip_program;

int tweak_gopher_types;

/* This is exported to libwww, like altogether too many other
   variables here. */
int binary_transfer;
/* Now we cache the current window right before doing a binary
   transfer, too.  Sheesh, this is not pretty. */
mo_window *current_win;

char *home_document = NULL;
char *machine;
char *shortmachine;
char *machine_with_domain;

XColor fg_color, bg_color;

/* This is set to the most recent value of global_window_per_document
   in any window, and reflects the user's desires for newly created
   windows.  Maybe. */
static int global_window_per_document;

static Widget exitbox = NULL;
static Cursor busy_cursor;
static int busy = 0;
static Widget *busylist = NULL;
char *cached_url = NULL;
/* Internal flag to indicate when we're deliberately munging the
   current window's document page and don't want document_page_cb
   to screw us up. */
static int in_document_page_munging = 0;

/* Forward declaration of test predicate. */
static int anchor_visited_predicate (Widget, char *);

/* When we first start the application, we call mo_startup()
   after creating the unmapped toplevel widget.  mo_startup()
   either sets the value of this to 1 or 0.  If 0, we don't
   open a starting window. */
int defer_initial_window;

/* This is the result of either posting the startup dialog
   and asking the user for permission to instrument,
   or reading the config file (.mosaicusr). */
int can_instrument;

/* ------------------------------- menubar -------------------------------- */

typedef enum
{
#ifdef HAVE_DTM
  mo_do_dtm,
#endif
  mo_reload_document, mo_document_source, mo_search,
  mo_open_document, mo_open_local_document, mo_save_document,
  mo_mail_document, mo_print_document, 
  mo_new_window, mo_clone_window,
  mo_close_window, mo_exit_program,
  mo_home_document, mo_ncsa_document,
  mo_cern_document, mo_ohiostate_document, mo_slac_document,
  mo_honolulu_document,
  mo_bsdi_document, mo_lysator_document, mo_northwestern, mo_british_columbia,
  mo_anu_document, mo_desy_document, mo_ctc_document,
  mo_indiana_document, mo_cmu_document, mo_ssc_document,
  mo_web_document, mo_hep_document,
  mo_wais_document, mo_gopher_document, mo_gopher_veronica,
  mo_gopher_wiretap,
  mo_gopher_internic,
  mo_ftp_document,
  mo_info_document, mo_datasources_document,
  mo_hyperg_document,
  mo_hytelnet_document,
  mo_techinfo_document, mo_law_document,
  mo_archie_document, mo_x500_document, mo_whois_document,
  mo_finger_document,
  mo_usenet_document,
  mo_njit_document,
  mo_overview_document, mo_servers_document, mo_www_news,
  mo_sunos_manpages, mo_hpux_manpages,
  mo_ultrix_manpages, mo_irix_manpages,
  mo_aix_manpages, mo_pyramid_manpages, mo_x11r5_manpages,
  mo_texinfo_manpages,
  mo_vms_manpages, mo_zippy_manpages, mo_internet_rfcs,
  mo_mosaic_homepage, mo_mosaic_manual, mo_mosaic_demopage,
  mo_siggraph_document,
  mo_back, mo_forward, mo_history_list, 
  mo_clear_global_history,
  mo_hotlist_postit, mo_register_node_in_default_hotlist,
  mo_window_per_document,
  mo_large_fonts, mo_regular_fonts, mo_small_fonts,
  mo_large_helvetica, mo_regular_helvetica, mo_small_helvetica,
  mo_large_newcentury, mo_regular_newcentury, mo_small_newcentury,
  mo_large_lucidabright, mo_regular_lucidabright, mo_small_lucidabright,
  mo_help_about, mo_help_onwindow, mo_help_onversion, mo_help_faq,
  mo_whine, mo_help_html, mo_help_url,
  mo_ncsa_gopher, mo_ctc_gopher, mo_psc_gopher, mo_sdsc_gopher,
  mo_umn_gopher, mo_uiuc_gopher, mo_uiuc_weather, mo_bigcheese_gopher, mo_pmc,
  mo_white_house, mo_cs_techreports,
  mo_bmcr, mo_losalamos, mo_losalamos_web,
  mo_internet_services, mo_ncsa_access, mo_evl,
  mo_vatican, mo_internet_radio, mo_anu_art_history,
  mo_whats_new,
  mo_annotate,
#ifdef HAVE_AUDIO_ANNOTATIONS
  mo_audio_annotate,
#endif
  mo_annotate_edit, mo_annotate_delete,
  mo_crosslink,
  mo_checkout, mo_checkin,
  mo_fancy_selections,
  mo_default_underlines, mo_l1_underlines, mo_l2_underlines, mo_l3_underlines,
  mo_no_underlines, mo_binary_transfer,
  mo_instrument_usage
} mo_token;

/* ----------------------------- Timing stuff ----------------------------- */

#include <sys/types.h>
#include <sys/times.h>
#include <sys/param.h>

/* Time globals. */
static struct tms tbuf;
static int gtime;

void StartClock (void) 
{
  gtime = times (&tbuf);
  
  return;
}

void StopClock ()
{
  int donetime;

  donetime = times(&tbuf);

  fprintf (stderr, "Elapsed time %d\n", donetime - gtime);

  return;
}

/* --------------------------- mo_post_exitbox ---------------------------- */

static XmxCallback (exit_confirm_cb)
{
  if (XmxExtractToken ((int)client_data))
    mo_exit ();
  else
    XtUnmanageChild (w);
  
  return;
}

static void mo_post_exitbox (void)
{
  if (Rdata.confirm_exit)
    {
      if (exitbox == NULL)
        {
          exitbox = XmxMakeQuestionDialog
            (toplevel, "Are you sure you want to exit NCSA Mosaic?",
             "NCSA Mosaic: Exit Confirmation", exit_confirm_cb, 1, 0);
          XtManageChild (exitbox);
        }
      else
        {
          XmxManageRemanage (exitbox);
        }
    }
  else
    {
      /* Don't confirm exit; just zap it. */
      mo_exit ();
    }

  return;
}

/* ----------------------------- WINDOW LIST ------------------------------ */

static mo_window *winlist = NULL;
static int wincount = 0;

/* Return either the first window in the window list or the next
   window after the current window. */
mo_window *mo_next_window (mo_window *win)
{
  if (win == NULL)
    return winlist;
  else
    return win->next;
}

/* Return a window matching a specified uniqid. */
mo_window *mo_fetch_window_by_id (int id)
{
  mo_window *win;

  win = winlist;
  while (win != NULL)
    {
      if (win->id == id)
        goto done;
      win = win->next;
    }

 notfound:
  return NULL;

 done:
  return win;
}

/* Register a window in the window list. */
mo_status mo_add_window_to_list (mo_window *win)
{
  wincount++;

  if (winlist == NULL)
    {
      win->next = NULL;
      winlist = win;
    }
  else
    {
      win->next = winlist;
      winlist = win;
    }

  return mo_succeed;
}

/* Remove a window from the window list. */
mo_status mo_remove_window_from_list (mo_window *win)
{
  mo_window *w = NULL, *prev = NULL;

  while (w = mo_next_window (w))
    {
      if (w == win)
        {
          /* Delete w. */
          if (!prev)
            {
              /* No previous window. */
              winlist = w->next;

              free (w);
              w = NULL;

              wincount--;

              /* Maybe exit. */
              if (!winlist)
                mo_exit ();
            }
          else
            {
              /* Previous window. */
              prev->next = w->next;

              free (w);
              w = NULL;

              wincount--;

              return mo_succeed;
            }
        }
      prev = w;
    }
   
  /* Couldn't find it. */
  return mo_fail;
}

/* ----------------------------- busy cursor ------------------------------ */

mo_status mo_not_busy (void)
{
  mo_window *win = NULL;
  
  if (busy)
    {
      XUndefineCursor (dsp, XtWindow (toplevel));
      while (win = mo_next_window (win))
        {
          XUndefineCursor (dsp, XtWindow (win->base));
          if (win->history_win)
            XUndefineCursor (dsp, XtWindow (win->history_win));
          if (win->hotlist_win)
            XUndefineCursor (dsp, XtWindow (win->hotlist_win));
        }
      
      XFlush (dsp);
      busy = 0;
    }
  
  return mo_succeed;
}

mo_status mo_busy (void)
{
  mo_window *win = NULL;

  if (!busy)
    {
      XDefineCursor (dsp, XtWindow (toplevel), busy_cursor);
      while (win = mo_next_window (win))
      {
        XDefineCursor (dsp, XtWindow (win->base), busy_cursor);
        if (win->history_win)
          XDefineCursor (dsp, XtWindow (win->history_win), busy_cursor);
        if (win->hotlist_win)
          XDefineCursor (dsp, XtWindow (win->hotlist_win), busy_cursor);
      }
      
      XFlush (dsp);
      busy = 1;
    }
  
  return mo_succeed;
}

/* ------------------ mo_set_window_per_document_toggles ------------------ */

static void mo_set_window_per_document_toggle (mo_window *win)
{
  XmxRSetToggleState (win->menubar, mo_window_per_document,
                      win->window_per_document ? XmxSet : XmxNotSet);
  return;
}

static void mo_set_window_per_document_toggles ()
{
  mo_window *win = NULL;

  while (win = mo_next_window (win))
    {
      XmxRSetToggleState (win->menubar, mo_window_per_document,
                          global_window_per_document ? XmxSet : XmxNotSet);
      win->window_per_document = global_window_per_document;
    }
 
  return;
}

/* -------------------- mo_set_fancy_selections_toggle -------------------- */

static void mo_set_fancy_selections_toggle (mo_window *win)
{
  XmxRSetToggleState (win->menubar, mo_fancy_selections,
                      win->pretty ? XmxSet : XmxNotSet);
  return;
}

/* ---------------------- mo_annotate_edit_possible ----------------------- */

static void mo_annotate_edit_possible (mo_window *win)
{
  XmxRSetSensitive (win->menubar, mo_annotate_edit, XmxSensitive);
  XmxRSetSensitive (win->menubar, mo_annotate_delete, XmxSensitive);
  return;
}

static void mo_annotate_edit_impossible (mo_window *win)
{
  XmxRSetSensitive (win->menubar, mo_annotate_edit, XmxNotSensitive);
  XmxRSetSensitive (win->menubar, mo_annotate_delete, XmxNotSensitive);
  return;
}

/* --------------------------- mo_back_possible --------------------------- */

/* This could be cached, but since it shouldn't take too long... */
static void mo_back_possible (mo_window *win)
{
  XmxSetSensitive (win->back_button, XmxSensitive);
  XmxRSetSensitive (win->menubar, mo_back, XmxSensitive);
  return;
}

static void mo_back_impossible (mo_window *win)
{
  XmxSetSensitive (win->back_button, XmxNotSensitive);
  XmxRSetSensitive (win->menubar, mo_back, XmxNotSensitive);
  return;
}

static void mo_forward_possible (mo_window *win)
{
  XmxSetSensitive (win->forward_button, XmxSensitive);
  XmxRSetSensitive (win->menubar, mo_forward, XmxSensitive);
  return;
}

static void mo_forward_impossible (mo_window *win)
{
  XmxSetSensitive (win->forward_button, XmxNotSensitive);
  XmxRSetSensitive (win->menubar, mo_forward, XmxNotSensitive);
  return;
}

static void mo_keyword_search_possible (mo_window *win)
{
  XmxSetSensitive (win->search_label, XmxSensitive);
  XmxSetSensitive (win->search_text, XmxSensitive);
  if (Rdata.warp_pointer_for_index)
    XWarpPointer (dsp, NULL, 
                  XtWindow (win->search_text),
                  0, 0, 0, 0, 30, 15);
  return;
}

static void mo_keyword_search_impossible (mo_window *win)
{
  XmxSetSensitive (win->search_label, XmxNotSensitive);
  XmxSetSensitive (win->search_text, XmxNotSensitive);
  return;
}

/* --------------------------- redisplay_window --------------------------- */

static mo_status mo_redisplay_window (mo_window *win)
{
  char *curl = cached_url;

  cached_url = win->cached_url;

  /* Thanks, Eric!! */
  HTMLRetestAnchors (win->view, anchor_visited_predicate);

  cached_url = curl;

  return mo_succeed;
}

/* ---------------------- mo_set_current_cached_win ----------------------- */

mo_status mo_set_current_cached_win (mo_window *win)
{
  current_win = win;
  view = win->view;

  return mo_succeed;
}

/* ------------------------------------------------------------------------ */
/* ------------------------- mo_load_window_text -------------------------- */
/* ------------------------- auxiliary functions -------------------------- */
/* ------------------------------------------------------------------------ */

/* -------------------------- mo_set_view_anchor -------------------------- */

/* Set the given scrollbar to view the given anchor.  The scrollbar
   is assumed to be in existence and managed.  The anchor is not assumed
   to exist within the document. */
static mo_status mo_set_view_anchor (mo_window *win, Widget sb, char *anchor)
{
  int x, y, page, current_page;
  int min, max;
  int val, size, inc, pageinc;

  XmScrollBarGetValues (sb, &val, &size, &inc, &pageinc);
  XtVaGetValues 
    (sb, XmNminimum, (long)(&min), XmNmaximum, (long)(&max), 
     WbNcurrentPage, (long)(&current_page), NULL);

  page = HTMLAnchorToPosition (win->view, anchor, &x, &y);
  if (page == -1)
    {
      /* Set the scrollbars to their minimum value. */
      XmScrollBarSetValues (sb, min, size, inc, pageinc, True);
    }
  else
    {
      if (page != current_page)
        {
          in_document_page_munging = 1;
          XtVaSetValues (win->view, WbNcurrentPage, page, NULL);
          in_document_page_munging = 0;

          XmScrollBarGetValues (sb, &val, &size, &inc, &pageinc);
          XtVaGetValues 
            (sb, XmNminimum, (long)(&min), XmNmaximum, (long)(&max), NULL);
        }
      /* Make sure we don't exceed the maximum. */
      val = MO_MIN (y, max - size - 1);
      /* Or fall under the maximum. */
      val = MO_MAX (val, min);
      XmScrollBarSetValues (sb, val, size, inc, pageinc, True);
    }

  return mo_succeed;
}

/* ------------------------ mo_set_page_increments ------------------------ */

/* For a stable (formatted) page with a managed scrollbar,
   do funky things to the page increments to make them reasonable. */
mo_status mo_set_page_increments (mo_window *win, Widget sb)
{
  int page_inc;
  XtVaGetValues (sb, XmNpageIncrement, &page_inc, NULL);
  /* Pick a divisor at random. */
  page_inc /= 24;
  if (page_inc < 1)
    page_inc = 1;
  XtVaSetValues (sb, XmNincrement, page_inc, NULL);

  return mo_succeed;
}

/* ----------------------- mo_set_scrollbar_minimum ----------------------- */

/* For a managed scrollbar, reset its position to its minimum. */
mo_status mo_set_scrollbar_minimum (mo_window *win, Widget sb)
{
  int min, val, size, inc, pageinc;

  XtVaGetValues (sb, XmNminimum, (long)(&min), NULL);
  XmScrollBarGetValues (sb, &val, &size, &inc, &pageinc);

  XmScrollBarSetValues (sb, min, size, inc, pageinc, True);

  return mo_succeed;
}

/* ------------------------- mo_raise_scrollbars -------------------------- */

/* This is called from mo_do_window_text.  It sets up scrollbar
   positions appropriately.
   Also we're called from document_page_cb. */
static mo_status mo_raise_scrollbars (mo_window *win)
{
  Widget sb;

  XtVaGetValues (win->scrolled_win, XmNverticalScrollBar, (long)(&sb), NULL);
  if (sb && XtIsManaged (sb))
    {
      if (win->target_anchor)
        {
          mo_set_view_anchor (win, sb, win->target_anchor);
          win->target_anchor = NULL;
        }
      else
        {
          mo_set_scrollbar_minimum (win, sb);
        }
      mo_set_page_increments (win, sb);
    }

  XtVaGetValues (win->scrolled_win, XmNhorizontalScrollBar, (long)(&sb), NULL);
  if (sb && XtIsManaged (sb))
    {
      mo_set_scrollbar_minimum (win, sb);
    }

  return mo_succeed;
}

/* ---------------------- mo_snarf_scrollbar_values ----------------------- */

/* Store state in the current_node in case we want to return. */
mo_status mo_snarf_scrollbar_values (mo_window *win)
{
  Widget sb = NULL;
  int min, val, size, inc, pageinc;

  /* Make sure we have a node. */
  if (!win->current_node)
    return mo_fail;

  XtVaGetValues (win->scrolled_win, XmNverticalScrollBar, &sb, NULL);
  if (sb && XtIsManaged (sb))
    {
      /* The scrollbar exists and is up; store its position and max. */
      XmScrollBarGetValues (sb, &(win->current_node->scrpos),
                            &size, &inc, &pageinc);
    }
  else
    {
      /* Scrollbar isn't up. */
      win->current_node->scrpos = -1;
    }
  win->current_node->docid = HTMLPositionToId 
    (win->view, 0, MO_MAX (win->current_node->scrpos, 0));

  return mo_succeed;
}


/* Restore scrollbar and document page state, if appropriate. */
static mo_status mo_restore_scrollbar_values (mo_window *win)
{
  Widget sb;
  int min, max, val, size, inc, pageinc;
  int pos, x, y;

  if (!win->current_node)
    return mo_fail;

  /* Nothing to restore... we don't restore document page yet
     because we don't know how many there might be. */
  if (win->current_node->scrpos < 0)
    return mo_succeed;

  pos = HTMLIdToPosition (win->view, win->current_node->docid, &x, &y);
  in_document_page_munging = 1;
  XtVaSetValues (win->view, WbNcurrentPage, pos, NULL);
  in_document_page_munging = 0;

  XtVaGetValues (win->scrolled_win, XmNverticalScrollBar, &sb, NULL);
  if (sb && XtIsManaged (sb))
    {
      XmScrollBarGetValues (sb, &val, &size, &inc, &pageinc);
      XtVaGetValues 
        (sb, XmNminimum, (long)(&min), XmNmaximum, (long)(&max), NULL);

      /* Special case for scrollbar at very top of window. */
      if (win->current_node->scrpos == min)
        y = min;

      val = MO_MIN (y, max - size - 1);
      val = MO_MAX (val, min);
      XmScrollBarSetValues (sb, val, size, inc, pageinc, True);

      mo_set_page_increments (win, sb);
    }
  
  /* Set the horizontal scrollbar to its origin, if it exists. */
  XtVaGetValues (win->scrolled_win, XmNhorizontalScrollBar, (long)(&sb), NULL);
  if (sb && XtIsManaged (sb))
    mo_set_scrollbar_minimum (win, sb);

  return mo_succeed;
}

/* ---------------------- mo_reset_document_headers ----------------------- */

static mo_status mo_reset_document_headers (mo_window *win)
{
  XmxTextSetString (win->title_text, win->current_node->title);
  XmxTextSetString (win->url_text, win->current_node->url);

  return mo_succeed;
}

/* -------------------------- mo_do_window_text --------------------------- */

static void set_text (Widget w, char *txt, char *ans)
{
  if (Rdata.annotations_on_top)
    HTMLSetText (w, txt, ans ? ans : "\0", "\0");
  else
    HTMLSetText (w, txt, "\0", ans ? ans : "\0");
}

/* Handle guts of mo_load_window_text and mo_load_window_text_first. */
/* If register_visit is true, then we record this visit with the
   history list.  Else we pull the title out explicitly anyway,
   since it may have changed since last time. */
static mo_status mo_do_window_text (mo_window *win, char *url, char *txt,
                                    char *txthead,
                                    int register_visit, char *ref)
{
  char *line;
  char *ans;
  Boolean isindex;

  /* cached_url HAS to be set here, since Resolve
     counts on it.  This sucks. */
  mo_here_we_are_son (url);
  cached_url = url;
  win->cached_url = url;

  if (can_instrument && win->instrument_usage)
    mo_inst_register_action (mo_inst_view, url);

  {
    /* Since mo_fetch_annotation_links uses the communications code,
       we need to play games with binary_transfer. */
    int tmp = binary_transfer;
    binary_transfer = 0;
    ans = mo_fetch_annotation_links (url, Rdata.annotations_on_top);
    binary_transfer = tmp;
  }
    
  {
    Widget w = win->scrolled_win;
    Window rwin;
    int vx, vy;
    unsigned int vwidth, vheight, vborder, vdepth;
    unsigned int orig_width, new_width, new_height;
    Widget sb;
    Dimension mw, spacing;

    new_width = win->last_width;

    /* Really we only want to force a resize if we previously
       had a scrollbar. */
    XtVaGetValues (w, XmNhorizontalScrollBar, &sb, NULL);

    if ((sb) && (XtIsManaged(sb)))
      {
        XGetGeometry(dsp, XtWindow(win->view), 
                     &rwin, &vx, &vy,
                     &vwidth, &vheight, &vborder, &vdepth);
        win->last_width = new_width;
        XtUnmanageChild(win->view);
        set_text (win->view, txt, ans);
        
        XtResizeWidget(win->view, MO_MAX((signed int)new_width - 10, 10), 
                       vheight, vborder);
        XtManageChild(win->view);
      }
    else
      {
        set_text (win->view, txt, ans);
      }
  }

  if (register_visit || win->target_anchor)
    {
      /* Go to the first page, then raise the scrollbars. */
      in_document_page_munging = 1;
      XtVaSetValues (win->view, WbNcurrentPage, 1, NULL);
      in_document_page_munging = 0;
      mo_raise_scrollbars (win);
    }
  else
    mo_restore_scrollbar_values (win);

  /* Every time we view the document, we reset the search_start
     struct so searches will start at the beginning of the document. */
  ((ElementRef *)win->search_start)->id = 0;

  /* CURRENT_NODE ISN'T SET UNTIL HERE. */
  /* Now that WbNtext has been set, we can pull out WbNtitle. */
  /* First, check and see if we have a URL.  If not, we probably
     are only jumping around inside a document. */
  if (url && *url)
    {
      if (register_visit)
        mo_record_visit (win, url, txt, txthead, ref);
      else
        /* At the very least we want to pull out the new title,
           if one exists. */
        win->current_node->title = mo_grok_title (win, url, ref);
    }
  
  mo_reset_document_headers (win);

  if (win->history_list)
    {
      XmListSelectPos (win->history_list, win->current_node->position, False);
      XmListSetBottomPos (win->history_list, win->current_node->position);
    }

  /* Update source text if necessary. */
  if (win->source_text && XtIsManaged(win->source_text))
    {
      XmxTextSetString (win->source_text, win->current_node->text);
      XmxTextSetString (win->source_url_text, win->current_node->url);
    }

  if (win->current_node->previous != NULL)
    mo_back_possible (win);
  else
    mo_back_impossible (win);
  
  if (win->current_node->next != NULL)
    mo_forward_possible (win);
  else
    mo_forward_impossible (win);

  /* If isIndex, then search possible, else search impossible. */
  XtVaGetValues (win->view, WbNisIndex, &isindex, NULL);
  if (isindex)
    mo_keyword_search_possible (win);
  else
    mo_keyword_search_impossible (win);

  if (mo_is_editable_annotation (win, win->current_node->text))
    mo_annotate_edit_possible (win);
  else
    mo_annotate_edit_impossible (win);

  /* Free existing hrefs, if any exist. */
  if (win->num_hrefs)
    {
      int i;
      for (i = 0; i < win->num_hrefs; i++)
        free (win->hrefs[i]);
      free (win->hrefs);
    }
  /* Go fetch new hrefs. */
  win->hrefs = HTMLGetHRefs (win->view, &(win->num_hrefs));
  /* Load up the future list, if it exists. */
  if (win->history_win)
    mo_load_future_list (win, win->future_list);

  mo_not_busy ();

  return mo_succeed;
}

/* -------------------------- mo_do_window_node --------------------------- */

/* Given a window and a node, load the window's text
   with the node's contents. */
mo_status mo_do_window_node (mo_window *win, mo_node *node)
{
  /* Turn on busy cursor; will be turned off at completion of
     mo_do_window_text(). */
  mo_busy ();
  mo_set_current_cached_win (win);
  return mo_do_window_text (win, node->url, 
                            node->text, node->texthead,
                            FALSE, node->ref);
}

/* ------------------------ mo_reload_window_text ------------------------- */

mo_status mo_reload_window_text (mo_window *win)
{
  mo_busy ();

  if (win->current_node->texthead)
    free (win->current_node->texthead);

  /* Set binary_transfer as per current window. */
  binary_transfer = win->binary_transfer;
  mo_set_current_cached_win (win);
  win->current_node->text = mo_pull_er_over (win->current_node->url, 
                                             &win->current_node->texthead);

  mo_snarf_scrollbar_values (win);
  mo_do_window_node (win, win->current_node);
  return mo_succeed;
}

/* ------------------------- mo_load_window_text -------------------------- */

static char *mo_munge_url (char *url)
{
  /* Now, it can happen that we will go from
     file://ftp.ncsa.uiuc.edu/ to file://ftp.ncsa.uiuc.edu here.
     So let's tack on a trailing slash if necessary.
     This is a hack, but what the hell? */
  if (!strncmp (url, "file://", 7))
    {
      char *foo = url + 7;
      if (!strstr (foo, "/"))
        {
          char *bar = (char *)malloc 
            ((strlen (url) + 4) * sizeof (char));
          sprintf (bar, "%s/\0", url);
          url = bar;
        }
    }
  return url;
}

/* Given a window and a raw URL, load the window.  Also keep
   the window's data structures up to date, and the history
   list, and the document source displays. */
/* Also munge the url to expand it to a full address. */
mo_status mo_load_window_text (mo_window *win, char *url, char *ref)
{
  char *newtext, *newtexthead;
  int loading_flag = 1;  /* assume we're loading over the net */

  win->target_anchor = HTParse (url, "", PARSE_ANCHOR);

  /* If we're just referencing an anchor inside a document,
     do the right thing. */
  if (url && *url == '#')
    {
      mo_busy ();
      newtext = win->current_node->text;
      newtexthead = win->current_node->texthead;
      loading_flag = 0;
      url = HTParse(url, win->current_node->url,
                    PARSE_ACCESS | PARSE_HOST | PARSE_PATH |
                    PARSE_PUNCTUATION);
    }
  else
    {
      /* Get a full address for this URL. */
      /* Under some circumstances we may not have a current node yet
         and may wish to just run with it... so check for that. */
      if (win->current_node && win->current_node->url)
        {
          url = HTParse(url, win->current_node->url,
                        PARSE_ACCESS | PARSE_HOST | PARSE_PATH |
                        PARSE_PUNCTUATION);
          url = mo_munge_url (url);
        }
      mo_busy ();
      /* Set binary_transfer as per current window. */
      binary_transfer = win->binary_transfer;
      mo_set_current_cached_win (win);
      newtext = mo_pull_er_over (url, &newtexthead);
    }

  /* Now, if it's a telnet session, there should be no need
     to do anything other than the mo_pull_er_over bit.
     Also check for override in text itself. */
  if (newtext && 
      strncmp (url, "telnet:", 7) && strncmp (url, "tn3270:", 7) &&
      strncmp (url, "rlogin:", 7) &&
      strncmp (newtext, "<mosaic-access-override>", 24))
    {
      /* Not a telnet session and not an override: */

      /* Grab the current scrollbar values for the window and the
         current node. */
      mo_snarf_scrollbar_values (win);

      /* Set the window text. */
      mo_do_window_text (win, url, newtext, newtexthead, loading_flag, ref);
    }
  else
    {
      /* Telnet session, or override, or no newtext (probably from DMF):
         We still want a global history entry but NOT a 
         window history entry. */
      mo_here_we_are_son (url);
      /* And we want to instrument. */
      if (can_instrument && win->instrument_usage)
        mo_inst_register_action (mo_inst_view, url);
      /* ... and we want to redisplay the current window to
         get the effect of the history entry today, not tomorrow. */
      mo_redisplay_window (win);
      /* We're not busy anymore... */
      mo_not_busy ();
    }
  
  return mo_succeed;
}

/* ---------------------- mo_load_window_text_first ----------------------- */

/* Given a window and a raw URL, load the window for the first time.
   This involves using mo_pull_er_over_first; other than that
   everything is the same as mo_load_window_text. */
static mo_status mo_load_window_text_first (mo_window *win, char *url, 
                                            char *ref,
                                            char *refurl)
{
  char *newtext, *newtexthead;
  
  /* We may have had a target anchor set for us in
     mo_open_another_window, so don't step on that. */
  if (!win->target_anchor)
    win->target_anchor = HTParse (url, "", PARSE_ANCHOR);

  /* Try to screen out reference to anchor, if any...
     HTParse seems to be busted. */
  {
    /* This coredumps on Suns if I don't make a copy of the
       string, grumble grumble... */
    char *newurl = strdup (url), *ptr;
    for (ptr = newurl; *ptr; ptr++)
      if (*ptr == '#')
        *ptr = '\0';
    url = newurl;
  }

  /* Get a full address for this URL. */
  /* Under some circumstances we may not have a current node yet
     and may wish to just run with it... so check for that. */
  if (refurl)
    {
      url = HTParse(url, refurl,
                    PARSE_ACCESS | PARSE_HOST | PARSE_PATH |
                    PARSE_PUNCTUATION);
      url = mo_munge_url (url);
    }
  
  mo_busy ();

  /* Set binary_transfer as per current window. */
  binary_transfer = win->binary_transfer;
  mo_set_current_cached_win (win);
  newtext = mo_pull_er_over_first (url, &newtexthead);

  mo_do_window_text (win, url, newtext, newtexthead, TRUE, ref);

  return mo_succeed;
}

/* ----------------------- mo_duplicate_window_text ----------------------- */

static mo_status mo_duplicate_window_text (mo_window *oldw, mo_window *neww)
{
  /* We can get away with just cloning text here and forgetting
     about texthead, obviously, since we're making a new copy. */
  char *newtext = strdup (oldw->current_node->text);

  mo_do_window_text 
    (neww, strdup (oldw->current_node->url), 
     newtext, newtext, TRUE, 
     oldw->current_node->ref ? strdup (oldw->current_node->ref) : NULL);

  return mo_succeed;
}

/* -------------------------- mo_access_document -------------------------- */

/* This function is designed to be called for pre-set document
   choices with full URL's already specified.  It either loads
   the window text or opens a new window, whichever is appropriate. */
mo_status mo_access_document (mo_window *win, char *url)
{
  mo_busy ();

  mo_set_current_cached_win (win);

  if (!win->window_per_document)
    mo_load_window_text (win, url, NULL);
  else
    mo_open_another_window (win, url, NULL, NULL);

  return mo_succeed;
}

/* ---------------------------- mo_set_fonts ---------------------------- */

static long wrapFont (char *name)
{
  XFontStruct *font = XLoadQueryFont (dsp, name);
  if (font == NULL)
    {
      fprintf (stderr, "Could not open font '%s'\n", name);
      font = XLoadQueryFont (dsp, "fixed");
    }
  return ((long)font);
}

static mo_status mo_set_fonts (mo_window *win, int size)
{
  switch (size)
    {
    case mo_large_fonts:
      XmxSetArg (XtNfont, wrapFont("-adobe-times-medium-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont("-adobe-times-medium-i-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont("-adobe-times-bold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont("-adobe-courier-medium-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont("-adobe-times-bold-r-normal-*-25-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont("-adobe-times-bold-r-normal-*-24-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont("-adobe-times-bold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont("-adobe-times-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont("-adobe-times-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont("-adobe-times-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont("-adobe-times-medium-i-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont("-adobe-courier-medium-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetValues (win->view);
      break;
    case mo_regular_fonts:
      XmxSetArg (XtNfont, wrapFont("-adobe-times-medium-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont("-adobe-times-medium-i-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont("-adobe-times-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont("-adobe-courier-medium-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont("-adobe-times-bold-r-normal-*-24-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont("-adobe-times-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont("-adobe-times-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont("-adobe-times-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont("-adobe-times-bold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont("-adobe-times-bold-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont("-adobe-times-medium-i-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont("-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetValues (win->view);
      break;
    case mo_small_fonts:
      XmxSetArg (XtNfont, wrapFont("-adobe-times-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont("-adobe-times-medium-i-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont("-adobe-times-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont("-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont("-adobe-times-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont("-adobe-times-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont("-adobe-times-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont("-adobe-times-bold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont("-adobe-times-bold-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont("-adobe-times-bold-r-normal-*-8-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont("-adobe-times-medium-i-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont("-adobe-courier-medium-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetValues (win->view);
      break;
    case mo_large_helvetica:
      XmxSetArg (XtNfont, wrapFont("-adobe-helvetica-medium-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont("-adobe-helvetica-medium-o-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont("-adobe-helvetica-bold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont("-adobe-courier-medium-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont("-adobe-helvetica-bold-r-normal-*-25-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont("-adobe-helvetica-bold-r-normal-*-24-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont("-adobe-helvetica-bold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont("-adobe-helvetica-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont("-adobe-helvetica-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont("-adobe-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont("-adobe-helvetica-medium-o-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont("-adobe-courier-medium-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetValues (win->view);
      break;
    case mo_regular_helvetica:
      XmxSetArg (XtNfont, wrapFont("-adobe-helvetica-medium-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont("-adobe-helvetica-medium-o-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont("-adobe-helvetica-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont("-adobe-courier-medium-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont("-adobe-helvetica-bold-r-normal-*-24-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont("-adobe-helvetica-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont("-adobe-helvetica-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont("-adobe-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont("-adobe-helvetica-bold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont("-adobe-helvetica-bold-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont("-adobe-helvetica-medium-o-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont("-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetValues (win->view);
      break;
    case mo_small_helvetica:
      XmxSetArg (XtNfont, wrapFont("-adobe-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont("-adobe-helvetica-medium-o-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont("-adobe-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont("-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont("-adobe-helvetica-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont("-adobe-helvetica-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont("-adobe-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont("-adobe-helvetica-bold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont("-adobe-helvetica-bold-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont("-adobe-helvetica-bold-r-normal-*-8-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont("-adobe-helvetica-medium-o-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont("-adobe-courier-medium-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetValues (win->view);
      break;
    case mo_large_newcentury:
      XmxSetArg (XtNfont, wrapFont("-adobe-new century schoolbook-medium-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont("-adobe-new century schoolbook-medium-i-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont("-adobe-courier-medium-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-25-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-24-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont("-adobe-new century schoolbook-medium-i-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont("-adobe-courier-medium-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetValues (win->view);
      break;
    case mo_small_newcentury:
      XmxSetArg (XtNfont, wrapFont("-adobe-new century schoolbook-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont("-adobe-new century schoolbook-medium-i-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont("-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-8-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont("-adobe-new century schoolbook-medium-i-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont("-adobe-courier-medium-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetValues (win->view);
      break;
    case mo_regular_newcentury:
      XmxSetArg (XtNfont, wrapFont("-adobe-new century schoolbook-medium-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont("-adobe-new century schoolbook-medium-i-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont("-adobe-courier-medium-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-24-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont("-adobe-new century schoolbook-bold-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont("-adobe-new century schoolbook-medium-i-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont("-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetValues (win->view);
      break;
    case mo_large_lucidabright:
      XmxSetArg (XtNfont, wrapFont("-b&h-lucidabright-medium-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont("-b&h-lucidabright-medium-i-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont("-b&h-lucidabright-demibold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont("-b&h-lucidatypewriter-medium-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-25-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-24-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont("-b&h-lucidabright-medium-i-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont("-b&h-lucidatypewriter-medium-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetValues (win->view);
      break;
    case mo_regular_lucidabright:
      XmxSetArg (XtNfont, wrapFont("-b&h-lucidabright-medium-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont("-b&h-lucidabright-medium-i-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont("-b&h-lucidabright-demibold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont("-b&h-lucidatypewriter-medium-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-24-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont("-b&h-lucidabright-medium-i-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont("-b&h-lucidatypewriter-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetValues (win->view);
      break;
    case mo_small_lucidabright:
      XmxSetArg (XtNfont, wrapFont("-b&h-lucidabright-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont("-b&h-lucidabright-medium-i-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont("-b&h-lucidabright-demibold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont("-b&h-lucidatypewriter-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-11-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont("-b&h-lucidabright-demibold-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont("-b&h-lucidabright-medium-i-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont("-b&h-lucidatypewriter-medium-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetValues (win->view);
      break;
    }

  XmxRSetToggleState (win->menubar, win->font_size, XmxNotSet);
  XmxRSetToggleState (win->menubar, size, XmxSet);

  win->font_size = size;

  {
    Widget sb;
    XtVaGetValues (win->scrolled_win, XmNverticalScrollBar, &sb, NULL);
    if (sb && XtIsManaged(sb))
      mo_set_page_increments (win, sb);
  }

  return mo_succeed;
}

/* -------------------------- mo_set_underlines --------------------------- */

static mo_status mo_set_underlines (mo_window *win, int choice)
{
  if (!win->underlines_snarfed)
    {
      XtVaGetValues (win->view,
                     WbNanchorUnderlines, &(win->underlines),
                     WbNvisitedAnchorUnderlines, &(win->visited_underlines),
                     WbNdashedAnchorUnderlines, &(win->dashed_underlines),
                     WbNdashedVisitedAnchorUnderlines, 
                     &(win->dashed_visited_underlines),
                     NULL);
      win->underlines_snarfed = 1;
    }

  switch (choice)
    {
    case mo_default_underlines:
      XmxSetArg (WbNanchorUnderlines, win->underlines);
      XmxSetArg (WbNvisitedAnchorUnderlines, win->visited_underlines);
      XmxSetArg (WbNdashedAnchorUnderlines, win->dashed_underlines);
      XmxSetArg (WbNdashedVisitedAnchorUnderlines, 
                 win->dashed_visited_underlines);
      XmxSetValues (win->view);
      break;
    case mo_l1_underlines:
      XmxSetArg (WbNanchorUnderlines, 1);
      XmxSetArg (WbNvisitedAnchorUnderlines, 1);
      XmxSetArg (WbNdashedAnchorUnderlines, False);
      XmxSetArg (WbNdashedVisitedAnchorUnderlines, True);
      XmxSetValues (win->view);
      break;
    case mo_l2_underlines:
      XmxSetArg (WbNanchorUnderlines, 1);
      XmxSetArg (WbNvisitedAnchorUnderlines, 1);
      XmxSetArg (WbNdashedAnchorUnderlines, False);
      XmxSetArg (WbNdashedVisitedAnchorUnderlines, False);
      XmxSetValues (win->view);
      break;
    case mo_l3_underlines:
      XmxSetArg (WbNanchorUnderlines, 2);
      XmxSetArg (WbNvisitedAnchorUnderlines, 1);
      XmxSetArg (WbNdashedAnchorUnderlines, False);
      XmxSetArg (WbNdashedVisitedAnchorUnderlines, False);
      XmxSetValues (win->view);
      break;
    case mo_no_underlines:
      XmxSetArg (WbNanchorUnderlines, 0);
      XmxSetArg (WbNvisitedAnchorUnderlines, 0);
      XmxSetArg (WbNdashedAnchorUnderlines, False);
      XmxSetArg (WbNdashedVisitedAnchorUnderlines, False);
      XmxSetValues (win->view);
      break;
    }

  XmxRSetToggleState (win->menubar, win->underlines_state, XmxNotSet);
  XmxRSetToggleState (win->menubar, choice, XmxSet);
  win->underlines_state = choice;
  
  return mo_succeed;
}

/* --------------------------- exit_confirm_cb ---------------------------- */

static XmxCallback (clear_history_confirm_cb)
{
  mo_window *win = mo_fetch_window_by_id (XmxExtractUniqid ((int)client_data));

  if (XmxExtractToken ((int)client_data))
    {
      mo_window *w = NULL;
      mo_wipe_global_history ();

      while (w = mo_next_window (w))
        mo_redisplay_window (w);
    }
  else
    XtUnmanageChild (w);
  
  return;
}

/* ----------------------- mo_do_delete_annotation ------------------------ */

/* Presumably we're on an annotation. */
static mo_status mo_do_delete_annotation (mo_window *win)
{
  char *author, *title, *text, *fname;
  int id;

  if (win->current_node->annotation_type == mo_annotation_private)
    {
      mo_grok_pan_pieces (win->current_node->url,
                          win->current_node->text,
                          &title, &author, &text, 
                          &id, &fname);
      
      mo_delete_annotation (win, id);
    }
  else if (win->current_node->annotation_type == mo_annotation_workgroup)
    {
      mo_delete_group_annotation (win, win->current_node->url);
    }

  return mo_succeed;
}

static XmxCallback (delete_annotation_confirm_cb)
{
  mo_window *win = mo_fetch_window_by_id (XmxExtractUniqid ((int)client_data));

  if (!mo_is_editable_annotation (win, win->current_node->text))
    return;
  
  if (XmxExtractToken ((int)client_data))
    mo_do_delete_annotation (win);
  
  return;
}

/* ------------------------------ menubar_cb ------------------------------ */

static XmxCallback (menubar_cb)
{
  mo_window *win = mo_fetch_window_by_id (XmxExtractUniqid ((int)client_data));
  int i = XmxExtractToken ((int)client_data);

  switch (i)
    {
#ifdef HAVE_DTM
    case mo_do_dtm:
      mo_post_dtmout_window (win);
      break;
#endif
    case mo_reload_document:
      mo_reload_window_text (win);
      break;
    case mo_document_source:
      mo_post_source_window (win);
      break;
    case mo_search:
      mo_post_search_window (win);
      break;
    case mo_open_document:
      mo_post_open_window (win);
      break;
    case mo_open_local_document:
      mo_post_open_local_window (win);
      break;
    case mo_save_document:
      mo_post_save_window (win);
      break;
    case mo_mail_document:
      mo_post_mail_window (win);
      break;
    case mo_print_document:
      mo_post_print_window (win);
      break;
    case mo_new_window:
      mo_open_another_window (win, home_document, NULL, NULL);
      break;
    case mo_clone_window:
      mo_duplicate_window (win);
      break;
    case mo_close_window:
      mo_delete_window (win);
      break;
    case mo_exit_program:
      mo_post_exitbox ();
      break;
    case mo_home_document:
      mo_load_window_text (win, home_document, NULL);
      break;
    case mo_ncsa_document:
      mo_access_document (win, "http://www.ncsa.uiuc.edu/General/NCSAHome.html");
      break;
    case mo_cern_document:
      mo_access_document (win, "http://info.cern.ch/");
      break;
    case mo_bsdi_document:
      mo_access_document (win, "http://www.bsdi.com/");
      break;
    case mo_ctc_document:
      mo_access_document (win, "http://www.tc.cornell.edu:80/ctc.html");
      break;
    case mo_anu_document:
      mo_access_document (win, "http://life.anu.edu.au:80/");
      break;
    case mo_desy_document:
      mo_access_document (win, "http://info.desy.de:80/");
      break;
    case mo_lysator_document:
      mo_access_document (win, "http://www.lysator.liu.se:7500/");
      break;
    case mo_british_columbia:
      mo_access_document (win, "http://www.cs.ubc.ca/");
      break;
    case mo_northwestern:
      mo_access_document (win, "http://www.acns.nwu.edu/");
      break;
    case mo_ohiostate_document:
      mo_access_document (win, "http://www.cis.ohio-state.edu:80/hypertext/information/information.html");
      break;
    case mo_indiana_document:
      mo_access_document (win, "http://cs.indiana.edu/home-page.html");
      break;
    case mo_cmu_document:
      mo_access_document (win, "http://www.cs.cmu.edu:8001/Web/FrontDoor.html");
      break;
    case mo_ssc_document:
      mo_access_document (win, "http://www.ssc.gov/SSC.html");
      break;
    case mo_slac_document:
      mo_access_document (win, "http://slacvm.slac.stanford.edu./FIND/SLAC.HTML");
      break;
    case mo_honolulu_document:
      mo_access_document (win, "http://pulua.hcc.hawaii.edu/");
      break;
    case mo_siggraph_document:
      mo_access_document (win, "http://iicm.tu-graz.ac.at:80/CSIGGRAPHbib");
      break;
    case mo_pmc:
      mo_access_document (win, "gopher://una.hh.lib.umich.edu:70/11/journals/pmc");
      break;
    case mo_cs_techreports:
      mo_access_document (win, "http://cs.indiana.edu/cstr/search");
      break;
    case mo_white_house:
      mo_access_document (win, "http://www.ncsa.uiuc.edu:8001/sunsite.unc.edu:210/%2Fhome3%2Fwais%2FWhite-House-Papers?");
      break;
    case mo_bmcr:
      mo_access_document (win, "gopher://gopher.lib.Virginia.EDU:70/11/alpha/bmcr");
      break;
    case mo_anu_art_history:
      mo_access_document (win, "http://www.ncsa.uiuc.edu/SDG/Experimental/anu-art-history/home.html");
      break;
    case mo_evl:
      mo_access_document (win, "http://www.ncsa.uiuc.edu/EVL/docs/Welcome.html");
      break;
    case mo_losalamos:
      mo_access_document (win, "gopher://mentor.lanl.gov:70/1");
      break;
    case mo_losalamos_web:
      mo_access_document (win, "http://mentor.lanl.gov:80/Welcome.html");
      break;
    case mo_ncsa_access:
      mo_access_document (win, "http://www.ncsa.uiuc.edu/Pubs/access/accessDir.html");
      break;
    case mo_internet_services:
      mo_access_document (win, "http://slacvx.slac.stanford.edu:80/misc/internet-services.html");
      break;
    case mo_internet_radio:
      mo_access_document (win, "http://www.ncsa.uiuc.edu/radio/radio.html");
      break;
    case mo_vatican:
      mo_access_document (win, "http://www.ncsa.uiuc.edu/SDG/Experimental/vatican.exhibit/Vatican.exhibit.html");
      break;
    case mo_njit_document:
      mo_access_document (win, "http://eies2.njit.edu:80/");
      break;
    case mo_overview_document:
      mo_access_document (win, "http://info.cern.ch/hypertext/WWW/LineMode/Defaults/default.html");
      break;
    case mo_archie_document:
      mo_access_document (win, "http://hoohoo.ncsa.uiuc.edu:1235/archie/index");
      break;
    case mo_finger_document:
      mo_access_document (win, "http://cs.indiana.edu/finger/gateway");
      break;
    case mo_x500_document:
      mo_access_document (win, "gopher://umich.edu:7777/");
      break;
    case mo_whois_document:
      mo_access_document (win, "gopher://gopher.tc.umn.edu:70/11/Phone%20Books/WHOIS%20Searches");
      break;
    case mo_usenet_document:
      mo_access_document (win, "http://info.cern.ch/hypertext/DataSources/News/Groups/Overview.html");
      break;
    case mo_datasources_document:
      mo_access_document (win, "http://info.cern.ch/hypertext/DataSources/ByAccess.html");
      break;
    case mo_web_document:
      mo_access_document (win, "http://info.cern.ch/hypertext/WWW/TheProject.html");
      break;
    case mo_hep_document:
      mo_access_document (win, "http://info.cern.ch/hypertext/DataSources/bySubject/Physics/HEP.html");
      break;
    case mo_wais_document:
      mo_access_document (win, "http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/Interfaces/wais/wais-interface.html");
      break;
    case mo_gopher_document:
      mo_access_document (win, "gopher://gopher.micro.umn.edu:70/11/Other%20Gopher%20and%20Information%20Servers");
      break;
    case mo_ftp_document:
      mo_access_document (win, "http://hoohoo.ncsa.uiuc.edu:80/ftp-interface.html");
      break;
    case mo_gopher_veronica:
      mo_access_document (win, "gopher://veronica.scs.unr.edu:70/11/veronica");
      break;
    case mo_gopher_wiretap:
      mo_access_document (win, "gopher://wiretap.spies.com/");
      break;
    case mo_gopher_internic:
      mo_access_document (win, "gopher://is.internic.net:70/11/infosource");
      break;
    case mo_hyperg_document:
      mo_access_document (win, "http://iicm.tu-graz.ac.at:80/ROOT");
      break;
    case mo_hytelnet_document:
      mo_access_document (win, "http://info.cern.ch:8002/w3start.txt");
      break;
    case mo_techinfo_document:
      mo_access_document (win, "http://info.cern.ch:8004/");
      break;
    case mo_law_document:
      mo_access_document (win, "http://fatty.law.cornell.edu:80/usr2/wwwtext/lii.table.html");
      break;
    case mo_info_document:
      mo_access_document (win, "http://info.cern.ch/hypertext/DataSources/bySubject/Overview.html");
      break;
    case mo_servers_document:
      mo_access_document (win, "http://info.cern.ch/hypertext/DataSources/WWW/Servers.html");
      break;
    case mo_www_news:
      mo_access_document (win, "http://info.cern.ch/hypertext/WWW/News/9305.html");
      break;
      /* MAN PAGES */
    case mo_sunos_manpages:
      mo_access_document (win, "http://www.cis.ohio-state.edu:80/man/sunos.top.html");
      break;
    case mo_hpux_manpages:
      mo_access_document (win, "http://www.cis.ohio-state.edu:80/man/hpux.top.html");
      break;
    case mo_ultrix_manpages:
      mo_access_document (win, "http://www.cis.ohio-state.edu:80/man/ultrix.top.html");
      break;
    case mo_pyramid_manpages:
      mo_access_document (win, "http://www.cis.ohio-state.edu:80/man/pyramid.top.html");
      break;
    case mo_x11r5_manpages:
      mo_access_document (win, "http://www.cis.ohio-state.edu:80/man/x11.top.html");
      break;
    case mo_texinfo_manpages:
      mo_access_document (win, "http://www.cis.ohio-state.edu:85/info/dir");
      break;
    case mo_vms_manpages:
      mo_access_document (win, "http://slacvx.slac.stanford.edu:80/HELP");
      break;
    case mo_internet_rfcs:
      mo_access_document (win, "http://www.cis.ohio-state.edu:80/hypertext/information/rfc.html");
      break;
    case mo_zippy_manpages:
      mo_access_document (win, "http://www.cis.ohio-state.edu:84/");
      break;
    case mo_irix_manpages:
      mo_access_document (win, "http://wintermute.ncsa.uiuc.edu:1235/man/index");
      break;
    case mo_aix_manpages:
      mo_access_document (win, "http://hoohoo.ncsa.uiuc.edu:1235/man/index");
      break;
    case mo_mosaic_homepage:
      mo_access_document
        (win, "http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/NCSAMosaicHome.html");
      break;
    case mo_mosaic_demopage:
      mo_access_document
        (win, "http://www.ncsa.uiuc.edu/demoweb/demo.html");
      break;
    case mo_mosaic_manual:
      mo_access_document
        (win, "http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/Docs/mosaic-docs.html");
      break;
      /* GOPHER SERVERS */
    case mo_ncsa_gopher:
      mo_access_document (win, "gopher://gopher.ncsa.uiuc.edu:70/1");
      break;
    case mo_ctc_gopher:
      mo_access_document (win, "gopher://gopher.tc.cornell.edu:70/1");
      break;
    case mo_psc_gopher:
      mo_access_document (win, "gopher://gopher.psc.edu:70/1");
      break;
    case mo_sdsc_gopher:
      mo_access_document (win, "gopher://gopher.sdsc.edu:70/1");
      break;
    case mo_umn_gopher:
      mo_access_document (win, "gopher://gopher.micro.umn.edu:70/1");
      break;
    case mo_uiuc_gopher:
      mo_access_document (win, "gopher://gopher.uiuc.edu:70/1");
      break;
    case mo_uiuc_weather:
      mo_access_document (win, "gopher://wx.atmos.uiuc.edu:70/1");
      break;
    case mo_bigcheese_gopher:
      mo_access_document (win, "gopher://bigcheese.math.scarolina.edu:70/1");
      break;

    case mo_back:
      mo_back_node (win);
      break;
    case mo_forward:
      mo_forward_node (win);
      break;
    case mo_history_list:
      mo_post_history_win (win);
      break;
    case mo_clear_global_history:
      XmxSetUniqid (win->id);
      XmxMakeQuestionDialog
        (win->base, "Are you sure you want to clear the global history?",
         "NCSA Mosaic: Clear Global History", clear_history_confirm_cb, 1, 0);
      XtManageChild (Xmx_w);
      break;
    case mo_hotlist_postit:
      mo_post_hotlist_win (win);
      break;
    case mo_register_node_in_default_hotlist:
      mo_add_node_to_default_hotlist (win->current_node);
      mo_write_default_hotlist ();
      break;
    case mo_window_per_document:
      /* The current window reverses status no matter what. */
      win->window_per_document = 1 - win->window_per_document;
      if (Rdata.global_window_per_document)
        {
          /* Set the global value. */
          global_window_per_document = win->window_per_document;
          /* Go set all of the individual windows' toggles.
             This will also set win->window_per_document for each win. */
          mo_set_window_per_document_toggles ();
        }
      else
        {
          /* No global value.  Only set this window's toggle. */
          mo_set_window_per_document_toggle (win);
        }
      break;
    case mo_fancy_selections:
      win->pretty = 1 - win->pretty;
      mo_set_fancy_selections_toggle (win);
      HTMLClearSelection (win->view);
      XmxSetArg (WbNfancySelections, win->pretty ? True : False);
      XmxSetValues (win->view);
      break;
    case mo_instrument_usage:
      win->instrument_usage =
        (win->instrument_usage ? 0 : 1);
      break;
    case mo_binary_transfer:
      win->binary_transfer =
        (win->binary_transfer ? 0 : 1);
      break;
    case mo_large_fonts:
    case mo_regular_fonts:
    case mo_small_fonts:
    case mo_large_helvetica:
    case mo_regular_helvetica:
    case mo_small_helvetica:
    case mo_large_newcentury:
    case mo_regular_newcentury:
    case mo_small_newcentury:
    case mo_large_lucidabright:
    case mo_regular_lucidabright:
    case mo_small_lucidabright:
      mo_set_fonts (win, i);
      break;
    case mo_default_underlines:
    case mo_l1_underlines:
    case mo_l2_underlines:
    case mo_l3_underlines:
    case mo_no_underlines:
      mo_set_underlines (win, i);
      break;
    case mo_help_about:
      mo_open_another_window
        (win, "http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/Docs/help-about.html",
         NULL, NULL);
      break;
    case mo_whats_new:
      mo_open_another_window
        (win, "http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/Docs/whats-new.html",
         NULL, NULL);
      break;
    case mo_help_onwindow:
      mo_open_another_window
        (win, "http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/Docs/help-on-docview-window.html",
         NULL, NULL);
      break;
    case mo_help_onversion:
      mo_open_another_window
        (win, MO_HELP_ON_VERSION_DOCUMENT,
         NULL, NULL);
      break;
    case mo_help_faq:
      mo_open_another_window (win, "http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/Docs/mosaic-faq.html", NULL, NULL);
      break;
    case mo_help_html:
      mo_open_another_window (win, "http://www.ncsa.uiuc.edu/General/Internet/WWW/HTMLPrimer.html", 
                              NULL, NULL);
      break;
    case mo_help_url:
      mo_open_another_window (win, "http://www.ncsa.uiuc.edu/demoweb/url-primer.html", 
                              NULL, NULL);
      break;
    case mo_whine:
      mo_post_whine_win (win);
      break;
    case mo_annotate:
      mo_post_annotate_win (win, 0, 0, NULL, NULL, NULL, NULL);
      break;
#ifdef HAVE_AUDIO_ANNOTATIONS
    case mo_audio_annotate:
      mo_post_audio_annotate_win (win);
      break;
#endif
    case mo_annotate_edit:
      /* OK, let's be smart.
         If we get here, we know we're viewing an editable
         annotation.
         We also know the filename (just strip the leading
         file: off the URL).
         We also know the ID, by virtue of the filename
         (just look for PAN-#.html. */
      {
        char *author, *title, *text, *fname;
        int id;

	if (win->current_node->annotation_type == mo_annotation_private)
	{
            mo_grok_pan_pieces (win->current_node->url,
                            win->current_node->text,
                            &title, &author, &text, 
                            &id, &fname);

	    mo_post_annotate_win (win, 1, id, title, author, text, fname);
	}
	else if (win->current_node->annotation_type == mo_annotation_workgroup)
	{
            mo_grok_grpan_pieces (win->current_node->url,
                            win->current_node->text,
                            &title, &author, &text, 
                            &id, &fname);
	    mo_post_annotate_win (win, 1, id, title, author, text, fname);
	}
      }
      break;
    case mo_annotate_delete:
      if (Rdata.confirm_delete_annotation)
        {
          XmxSetUniqid (win->id);
          XmxMakeQuestionDialog
            (win->base, "Are you sure you want to delete this annotation?",
             "NCSA Mosaic: Delete Annotation", delete_annotation_confirm_cb, 1, 0);
          XtManageChild (Xmx_w);
        }
      else
        mo_do_delete_annotation (win);
      break;
    case mo_crosslink:
    case mo_checkout:
    case mo_checkin:
      XmxMakeErrorDialog (win->base, 
                          "Sorry, this function isn't working yet.", 
                          "Just Can't Do It");
      XtManageChild (Xmx_w);
      break;
    }

  return;
}

/* ------------------------------------------------------------------------ */
/* ---------------------------- mo_open_window ---------------------------- */
/* ------------------------------------------------------------------------ */

static XmxMenubarStruct file_menuspec[] =
{
  { "New Window",         'N', menubar_cb, mo_new_window },
  { "Clone Window",       'C', menubar_cb, mo_clone_window },
  { "----" },
  { "Open...",            'O', menubar_cb, mo_open_document },
  { "Open Local...",      'L', menubar_cb, mo_open_local_document },
  { "----" },
  { "Save As...",         'A', menubar_cb, mo_save_document },
  { "Mail To...",         'M', menubar_cb, mo_mail_document },
  { "Print...",           'P', menubar_cb, mo_print_document },
#ifdef HAVE_DTM
  { "DTM Broadcast...",   'B', menubar_cb, mo_do_dtm },
#endif /* HAVE_DTM */
  { "----" },
  { "Search...",          'S', menubar_cb, mo_search },
  { "Reload",             'R', menubar_cb, mo_reload_document },
  { "Document Source...", 'D', menubar_cb, mo_document_source },
  { "----" },
  { "Close Window",       'W', menubar_cb, mo_close_window },
  { "Exit Program...",    'E', menubar_cb, mo_exit_program },
  { NULL },
};

static XmxMenubarStruct manp_menuspec[] =
{
  { "NCSA Mosaic Manual",     'M', menubar_cb, mo_mosaic_manual },
  { "----" },
  { "SunOS 4.1.1 Man Pages",  'S', menubar_cb, mo_sunos_manpages },
  { "HP/UX 8.0.7 Man Pages",  'H', menubar_cb, mo_hpux_manpages },
  { "Ultrix 4.2a Man Pages",  'U', menubar_cb, mo_ultrix_manpages },
  { "IRIX 4.0 Man Pages",     'I', menubar_cb, mo_irix_manpages },
  { "AIX 3.2 Man Pages",      'A', menubar_cb, mo_aix_manpages },
  { "Pyramid OSx Man Pages",  'P', menubar_cb, mo_pyramid_manpages },
  { "----" },
  { "GNU Texinfo Manuals",    'G', menubar_cb, mo_texinfo_manpages },
  { "X11R5 Man Pages",        'X', menubar_cb, mo_x11r5_manpages },
  { "VMS Help System",        'V', menubar_cb, mo_vms_manpages },
  { "Internet RFCs",          'R', menubar_cb, mo_internet_rfcs },
  { "----" },
  { "Zippy The Pinhead",      'Z', menubar_cb, mo_zippy_manpages },
  { NULL },
};

static XmxMenubarStruct goph_menuspec[] =
{
  { "Gopherspace Overview",   'G', menubar_cb, mo_gopher_document },
  { "Veronica Interface",     'V', menubar_cb, mo_gopher_veronica },
  { "----" },
  { "NCSA Gopher",            'N', menubar_cb, mo_ncsa_gopher },
  { "CTC Gopher",             'C', menubar_cb, mo_ctc_gopher },
  { "PSC Gopher",             'P', menubar_cb, mo_psc_gopher },
  { "SDSC Gopher",            'S', menubar_cb, mo_sdsc_gopher },
  { "----" },
  { "Original (UMN) Gopher",  'O', menubar_cb, mo_umn_gopher },
  { "UIUC Gopher",            'U', menubar_cb, mo_uiuc_gopher },
  { "UIUC Weather Machine",   'W', menubar_cb, mo_uiuc_weather },
  { "USC Math Gopher",        'M', menubar_cb, mo_bigcheese_gopher },
  { "Wiretap Archives",       'A', menubar_cb, mo_gopher_wiretap },
  { NULL },
};

static XmxMenubarStruct otherwwws_menuspec[] =
{
  { "Data Sources By Service",'D', menubar_cb, mo_datasources_document },
  { "Information By Subject", 'I', menubar_cb, mo_info_document },
  { "Web News",               'N', menubar_cb, mo_www_news },
  { "Web Server Directory",   'S', menubar_cb, mo_servers_document },
  { NULL },
};

static XmxMenubarStruct otherints_menuspec[] =
{
  { "Archie Interface",       'A', menubar_cb, mo_archie_document },
  { "Hyper-G Interface",      'G', menubar_cb, mo_hyperg_document },
  { "Techinfo Interface",     'T', menubar_cb, mo_techinfo_document },
  { "X.500 Interface",        'X', menubar_cb, mo_x500_document },
  { "Whois Interface",        'W', menubar_cb, mo_whois_document },
  { NULL },
};

static XmxMenubarStruct otherhome_menuspec[] =
{
  { "ANU Bioinformatics",     'A', menubar_cb, mo_anu_document },
  { "British Columbia",       'r', menubar_cb, mo_british_columbia },
  { "BSDI Home Page",         'B', menubar_cb, mo_bsdi_document },
  { "Carnegie Mellon",        'M', menubar_cb, mo_cmu_document },
  { "CERN Home Page",         'E', menubar_cb, mo_cern_document },
  { "Cornell Law School",     'C', menubar_cb, mo_law_document },
  { "Cornell Theory Center",  'T', menubar_cb, mo_ctc_document },
  { "DESY Home Page",         'D', menubar_cb, mo_desy_document },
  { "Honolulu Home Page",     'H', menubar_cb, mo_honolulu_document },
  { "Indiana Home Page",      'I', menubar_cb, mo_indiana_document },
  /* { "Lysator ACS, Sweden",    'L', menubar_cb, mo_lysator_document }, */
  { "Northwestern Home Page", 'N', menubar_cb, mo_northwestern },
  { "Ohio State Home Page",   'O', menubar_cb, mo_ohiostate_document },
  { "SSC Home Page",          'S', menubar_cb, mo_ssc_document },
  { NULL },
};

static XmxMenubarStruct otherdocs_menuspec[] =
{
  { "ANU Art History Exhibit",             'A', menubar_cb, mo_anu_art_history },
  { "Bryn Mawr Classical Review",          'B', menubar_cb, mo_bmcr },
  { "Electronic Visualization Lab",        'E', menubar_cb, mo_evl },
  { "Internet Services List",              'S', menubar_cb, mo_internet_services },
  { "Internet Talk Radio",                 'T', menubar_cb, mo_internet_radio },
  { "Library of Congress Vatican Exhibit", 'V', menubar_cb, mo_vatican },
  { "Los Alamos Physics Papers (Gopher)",  'G', menubar_cb, mo_losalamos },
  { "Los Alamos Physics Papers (Web)",     'L', menubar_cb, mo_losalamos_web },
  { "NCSA Access Magazine",                'N', menubar_cb, mo_ncsa_access },
  { "Postmodern Culture",                  'P', menubar_cb, mo_pmc },
  { "Unified CS Tech Reports",             'U', menubar_cb, mo_cs_techreports },
  { "White House Papers",                  'W', menubar_cb, mo_white_house },
  { NULL },
};

static XmxMenubarStruct docs_menuspec[] =
{
  { "Web Overview",           'W', menubar_cb, mo_overview_document },
  { "Web Project",            'P', menubar_cb, mo_web_document },
  { "Other Web Documents",    'O', NULL, NULL, otherwwws_menuspec },
  { "----" },
  { "InterNIC Info Source",   'I', menubar_cb, mo_gopher_internic },
  { "----" },
  { "NCSA Mosaic Home Page",  'M', menubar_cb, mo_mosaic_homepage },
  { "NCSA Mosaic Demo Page",  'D', menubar_cb, mo_mosaic_demopage },
  { "NCSA Home Page",         'N', menubar_cb, mo_ncsa_document },
  { "Other Home Pages",       'H', NULL, NULL, otherhome_menuspec },
  { "----" },
  { "WAIS Interface",         'A', menubar_cb, mo_wais_document },
  { "Gopher Interface",       'G', NULL, NULL, goph_menuspec },
  { "FTP Interface",          'F', menubar_cb, mo_ftp_document },
  { "Usenet Interface",       'U', menubar_cb, mo_usenet_document },
  { "Finger Interface",       'g', menubar_cb, mo_finger_document },
  { "HyTelnet Interface",     'T', menubar_cb, mo_hytelnet_document },
  { "TeXinfo Interface",      'X', menubar_cb, mo_texinfo_manpages },
  { "Other Interfaces",       'e', NULL, NULL, otherints_menuspec },
  { "----" },
  { "Other Documents",        'r', NULL, NULL, otherdocs_menuspec },
  { NULL },
};

static XmxMenubarStruct navi_menuspec[] =
{
  { "Back",                    'B', menubar_cb, mo_back },
  { "Forward",                 'F', menubar_cb, mo_forward },
  { "----" },   
  { "Open...",                 'O', menubar_cb, mo_open_document },
  { "Open Local...",           'L', menubar_cb, mo_open_local_document },
  { "Home Document",           'D', menubar_cb, mo_home_document },
  { "----" },   
  { "Window History...",       'W', menubar_cb, mo_history_list },
  { "Clear Global History...",     'C', menubar_cb, mo_clear_global_history },
  { "----" },
  { "Hotlist...",              'H', menubar_cb, mo_hotlist_postit },
  { "Add Document To Hotlist", 'A', menubar_cb, mo_register_node_in_default_hotlist },
  { NULL },
};

static XmxMenubarStruct fnts_menuspec[] =
{
  { "<Large Times Fonts",          'L', menubar_cb, mo_large_fonts },
  { "<Large Helvetica Fonts",      'H', menubar_cb, mo_large_helvetica },
  { "<Large New Century Fonts",    'N', menubar_cb, mo_large_newcentury },
  { "<Large Lucida Bright Fonts",  'B', menubar_cb, mo_large_lucidabright },
  { "----" },
  { "<Small Times Fonts",          'S', menubar_cb, mo_small_fonts },
  { "<Small Helvetica Fonts",      'e', menubar_cb, mo_small_helvetica },
  { "<Small New Century Fonts",    'C', menubar_cb, mo_small_newcentury },
  { "<Small Lucida Bright Fonts",  'F', menubar_cb, mo_small_lucidabright },
  { NULL },
};

static XmxMenubarStruct undr_menuspec[] =
{
  { "<Default Underlines",         'D', menubar_cb, mo_default_underlines },
  { "<Light Underlines",           'L', menubar_cb, mo_l1_underlines },
  { "<Medium Underlines",          'M', menubar_cb, mo_l2_underlines },
  { "<Heavy Underlines",           'H', menubar_cb, mo_l3_underlines },
  { "<No Underlines",              'N', menubar_cb, mo_no_underlines },
  { NULL },
};

static XmxMenubarStruct opts_menuspec[] =
{
  { "#Window Per Document",        'W', menubar_cb, mo_window_per_document },
  { "#Fancy Selections",           'F', menubar_cb, mo_fancy_selections },
  { "#Binary Transfer Mode",       'B', menubar_cb, mo_binary_transfer },
#ifdef INSTRUMENT
  { "#Instrument Usage",           'I', menubar_cb, mo_instrument_usage },
#endif
  { "----" },
  { "<Times (Default) Fonts",      'T', menubar_cb, mo_regular_fonts },
  { "<Helvetica Fonts",            'H', menubar_cb, mo_regular_helvetica },
  { "<New Century Fonts",          'N', menubar_cb, mo_regular_newcentury },
  { "<Lucida Bright Fonts",        'L', menubar_cb, mo_regular_lucidabright },
  { "More Fonts",                  'M', NULL, NULL, fnts_menuspec },
  { "----" },
  { "Anchor Underlines",           'A', NULL, NULL, undr_menuspec }, 
  { NULL },
};

static XmxMenubarStruct help_menuspec[] =
{
  { "About...",      'A', menubar_cb, mo_help_about },
  { "What's New...", 'W', menubar_cb, mo_whats_new },
  { "----" },
  { "On Version...", 'V', menubar_cb, mo_help_onversion },
  { "On Window...",  'O', menubar_cb, mo_help_onwindow },
  { "On FAQ...",     'F', menubar_cb, mo_help_faq },
  { "----" },
  { "On HTML...",    'H', menubar_cb, mo_help_html },
  { "On URLs...",    'U', menubar_cb, mo_help_url },
  { "----" },
  { "Mail Developers...",   'M', menubar_cb, mo_whine },
  { NULL },
};

static XmxMenubarStruct anno_menuspec[] =
{
  { "Annotate...",              'A', menubar_cb, mo_annotate },
#ifdef HAVE_AUDIO_ANNOTATIONS
  { "Audio Annotate...",        'u', menubar_cb, mo_audio_annotate },
#endif
  { "----" },
  { "Edit This Annotation...",  'E', menubar_cb, mo_annotate_edit },
  { "Delete This Annotation...",'D', menubar_cb, mo_annotate_delete },
  { NULL },
};

static XmxMenubarStruct menuspec[] =
{
  { "File",      'F', NULL, NULL, file_menuspec },
  { "Navigate",  'N', NULL, NULL, navi_menuspec },
  { "Options",   'O', NULL, NULL, opts_menuspec },
  { "Annotate",  'A', NULL, NULL, anno_menuspec },
  { "Documents", 'D', NULL, NULL, docs_menuspec },
  { "Manuals",   'M', NULL, NULL, manp_menuspec },
  { "Help",      'H', NULL, NULL, help_menuspec },
  { NULL },
};

/* ------------------------------------------------------------------------ */

static XmxCallback (anchor_cb)
{
  char *href, *reftext;
  char *access;
  mo_window *win = mo_fetch_window_by_id (XmxExtractUniqid ((int)client_data));
  XButtonReleasedEvent *event = 
    (XButtonReleasedEvent *)(((WbAnchorCallbackData *)call_data)->event);
  int force_newwin = (event->button == Button2 ? 1 : 0);

  href = strdup (((WbAnchorCallbackData *)call_data)->href);
  reftext = strdup (((WbAnchorCallbackData *)call_data)->text);

  mo_convert_newlines_to_spaces (href);

#ifdef HAVE_DMF
  access = HTParse (href, win->current_node->url, PARSE_ACCESS);
  if (access && strncmp(access, "dmffield", 8) == 0)
  {
    dmf_field_dialog(href, reftext, win, force_newwin);
    return;
  }
#endif /* HAVE_DMF */

  if (!win->window_per_document && !force_newwin)
    mo_load_window_text (win, href, reftext);
  else
    {
      char *target = HTParse (href, "", PARSE_ANCHOR);
      href = HTParse(href, win->current_node->url, 
                    PARSE_ACCESS | PARSE_HOST | PARSE_PATH |
                    PARSE_PUNCTUATION);
      if (strncmp (href, "telnet:", 7) && strncmp (href, "tn3270:", 7) &&
          strncmp (href, "rlogin:", 7))
        {
          /* Not a telnet anchor. */

          /* Open the window (generating a new cached_url). */
          mo_open_another_window (win, href, reftext, target);

          /* Now redisplay this window. */
          mo_redisplay_window (win);
        }
      else
        /* Just do mo_load_window_text go get the xterm forked off. */
        mo_load_window_text (win, href, reftext);
    }

  return;
}

#ifdef HAVE_DMF
void dmffield_anchor_cb(char *href, char *reftext, mo_window *win, int force_newwin)
{
  if (!win->window_per_document && !force_newwin)
    mo_load_window_text (win, href, reftext);
  else
    {
      href = HTParse(href, win->current_node->url, 
                    PARSE_ACCESS | PARSE_HOST | PARSE_PATH |
                    PARSE_PUNCTUATION);
      if (strncmp (href, "telnet:", 7) && strncmp (href, "tn3270:", 7) &&
          strncmp (href, "rlogin:", 7))
        {
          /* Not a telnet anchor. */

          /* Open the window (generating a new cached_url). */
          mo_open_another_window (win, href, reftext, NULL);

          /* Now redisplay the goddamn window. */
          mo_redisplay_window (win);
        }
      else
        /* Just do mo_load_window_text go get the xterm forked off. */
        mo_load_window_text (win, href, reftext);
    }

  return;
}
#endif /* HAVE_DMF */

static XmxCallback (document_page_cb)
{
  mo_window *win = mo_fetch_window_by_id (XmxExtractUniqid ((int)client_data));

  /* Reset the scrollbars when a document page is accessed. */
  if (!in_document_page_munging)
    {
      mo_raise_scrollbars (win);
    }

  return;
}

/* This is the equivalent of Eric's StructureProc. */
static XmxEventHandler (structure_handler)
{
  mo_window *win;
  Window rwin;
  int vx, vy;
  unsigned int vwidth, vheight, vborder, vdepth;
  unsigned int orig_width, new_width, new_height;
  Widget sb;
  Dimension mw, spacing;
  
  win = mo_fetch_window_by_id (XmxExtractUniqid ((int)client_data));

  XGetGeometry(dsp, XtWindow(w), &rwin, &vx, &vy, &vwidth, &vheight,
               &vborder, &vdepth);
  
  orig_width = vwidth;
  new_width = vwidth;
  new_height = vheight;
  
  sb = 0;
  XtVaGetValues (w, XmNscrolledWindowMarginWidth, &mw,
                 XmNspacing, &spacing,
                 XmNverticalScrollBar, &sb,
                 NULL);
  
  new_width = new_width - (2 * mw);

  if ((sb) && (XtIsManaged(sb)))
    {
      XGetGeometry(dsp, XtWindow(sb), &rwin, &vx, &vy,
                   &vwidth, &vheight, &vborder, &vdepth);
      new_width = new_width - vwidth - (2 * vborder) - spacing;
    }
  
  if (new_width != win->last_width)
    {
      XGetGeometry(dsp, XtWindow(win->view), &rwin, &vx, &vy,
                   &vwidth, &vheight, &vborder, &vdepth);
      win->last_width = new_width;
      XtUnmanageChild(win->view);
      XtResizeWidget(win->view, MO_MAX((signed int)new_width - 10, 10), 
                     vheight, vborder);
      XtManageChild(win->view);
    }

  if ((sb) && (XtIsManaged(sb)))
    {
      mo_set_page_increments (win, sb);
    }

  return;
}

/* Called when a search keyword is entered. */
static XmxCallback (searchtext_cb)
{
  mo_window *win = mo_fetch_window_by_id (XmxExtractUniqid ((int)client_data));
  char *keyword = XmxTextGetString (w);
  char *url = (char *)malloc ((strlen (win->current_node->url) +
                               strlen (keyword) +
                               10) * sizeof (char));
  char *oldurl = strdup (win->current_node->url);
  int i;

  /* Convert spaces to plus's: see
     http://info.cern.ch/hypertext/WWW/Addressing/Search.html */
  for (i = 0; i < strlen (keyword); i++)
    if (keyword[i] == ' ')
      keyword[i] = '+';

  /* Clip out previous query, if any, by calling strtok. */
  strtok (oldurl, "?");
  
  if (win->current_node->url[strlen(oldurl)-1] == '?')
    /* Question mark is already there... */
    sprintf (url, "%s%s\0", oldurl, keyword);
  else
    sprintf (url, "%s?%s\0", oldurl, keyword);

  if (!win->window_per_document)
    mo_load_window_text (win, url, NULL);
  else
    mo_open_another_window (win, url, NULL, NULL);

  XmxTextSetString (w, "\0");

  return;
}

/* ---------------------------- mo_fill_window ---------------------------- */

static int anchor_visited_predicate (Widget w, char *href)
{
  if (!Rdata.track_visited_anchors || !href)
    return 0;

  href = HTParse(href, cached_url,
                 PARSE_ACCESS | PARSE_HOST | PARSE_PATH |
                 PARSE_PUNCTUATION);

  return (mo_been_here_before_huh_dad (href) == mo_succeed ? 1 : 0);
}

static XmxEventHandler (mo_view_keypress_handler)
{
  mo_window *win = mo_fetch_window_by_id (XmxExtractUniqid ((int)client_data));
  int _bufsize = 3, _count;
  char _buffer[3];
  KeySym _key;
  XComposeStatus _cs;

  if (!win)
    return;

  /* Go get ascii translation. */
  _count = XLookupString (&(event->xkey), _buffer, _bufsize, 
                          &_key, &_cs);
  
  /* Insert trailing Nil. */
  _buffer[_count] = '\0';

  /* Page up. */
  if ((Rdata.catch_prior_and_next && _key == XK_Prior) || 
      _key == XK_KP_Prior ||
      _key == XK_BackSpace || _key == XK_Delete)
    {
      Widget sb;
      String params[1];
      
      params[0] = "0";

      XtVaGetValues (win->scrolled_win, XmNverticalScrollBar, 
                     (long)(&sb), NULL);
      if (sb && XtIsManaged (sb))
        {
          XtCallActionProc (sb, "PageUpOrLeft", event, params, 1);
        }
    }

  /* Page down. */
  if ((Rdata.catch_prior_and_next && _key == XK_Next) || _key == XK_KP_Next ||
      _key == XK_Return || _key == XK_Tab ||
      _key == XK_space)
    {
      Widget sb;
      String params[1];
      
      params[0] = "0";
      
      XtVaGetValues (win->scrolled_win, XmNverticalScrollBar, 
                     (long)(&sb), NULL);
      if (sb && XtIsManaged (sb))
        {
          XtCallActionProc (sb, "PageDownOrRight", event, params, 1);
        }
    }
  
  if (_key == XK_Down || _key == XK_KP_Down)
    {
      Widget sb;
      String params[1];
      
      params[0] = "0";
      
      XtVaGetValues (win->scrolled_win, XmNverticalScrollBar, 
                     (long)(&sb), NULL);
      if (sb && XtIsManaged (sb))
        {
          XtCallActionProc (sb, "IncrementDownOrRight", event, params, 1);
        }
    }
  
  if (_key == XK_Right || _key == XK_KP_Right)
    {
      Widget sb;
      String params[1];
      
      params[0] = "1";
      
      XtVaGetValues (win->scrolled_win, XmNhorizontalScrollBar, 
                     (long)(&sb), NULL);
      if (sb && XtIsManaged (sb))
        {
          XtCallActionProc (sb, "IncrementDownOrRight", event, params, 1);
        }
    }
  
  if (_key == XK_Up || _key == XK_KP_Up)
    {
      Widget sb;
      String params[1];
      
      params[0] = "0";
      
      XtVaGetValues (win->scrolled_win, XmNverticalScrollBar, 
                     (long)(&sb), NULL);
      if (sb && XtIsManaged (sb))
        {
          XtCallActionProc (sb, "IncrementUpOrLeft", event, params, 1);
        }
    }
  
  if (_key == XK_Left || _key == XK_KP_Left)
    {
      Widget sb;
      String params[1];
      
      params[0] = "1";
      
      XtVaGetValues (win->scrolled_win, XmNhorizontalScrollBar, 
                     (long)(&sb), NULL);
      if (sb && XtIsManaged (sb))
        {
          XtCallActionProc (sb, "IncrementUpOrLeft", event, params, 1);
        }
    }
  
  /* Annotate. */
  if (_key == XK_A || _key == XK_a)
    mo_post_annotate_win (win, 0, 0, NULL, NULL, NULL, NULL);

  /* Back. */
  if (_key == XK_B || _key == XK_b)
    mo_back_node (win);
  
  /* Clone. */
  if (_key == XK_C || _key == XK_c)
    mo_duplicate_window (win);

  /* Document source. */
  if (_key == XK_D || _key == XK_d)
    mo_post_source_window (win);
    
  /* Forward. */
  if (_key == XK_F || _key == XK_f)
    mo_forward_node (win);
  
  /* History window. */
  if (_key == XK_H || _key == XK_h)
    mo_post_history_win (win);

  /* Open local. */
  if (_key == XK_L || _key == XK_l)
    mo_post_open_local_window (win);
  
  /* Mail to. */
  if (_key == XK_M || _key == XK_m)
    mo_post_mail_window (win);

  /* New window. */
  if (_key == XK_N || _key == XK_n)
    mo_open_another_window (win, home_document, NULL, NULL);
  
  /* Open. */
  if (_key == XK_O || _key == XK_o)
    mo_post_open_window (win);

  /* Print. */
  if (_key == XK_P || _key == XK_p)
    mo_post_print_window (win);

  /* Reload. */
  if (_key == XK_R || _key == XK_r)
    mo_reload_window_text (win);
   
  /* Search. */
  if (_key == XK_S || _key == XK_s)
    mo_post_search_window (win);
  
  if (_key == XK_Escape)
    mo_delete_window (win);
  
  return;
}

/* Fill in all the little GUI bits and pieces in an existing but empty
   mo_window struct. */
static void mo_fill_window (mo_window *win)
{
  Widget form;
  Widget logo, frame, rc;
  Widget title_label, url_label;

  form = XmxMakeForm (win->base);

  /* Create the menubar. */
  win->menubar = XmxRMakeMenubar (form, menuspec);

  if (win->window_per_document)
    XmxRSetToggleState (win->menubar, mo_window_per_document, XmxSet);
  XmxRSetToggleState (win->menubar, win->font_size, XmxSet);

  win->instrument_usage = Rdata.instrument_usage ? 1 : 0;
  if (!can_instrument)
    win->instrument_usage = 0;
#ifdef INSTRUMENT
  if (can_instrument)
    {
      XmxRSetToggleState (win->menubar, mo_instrument_usage,
                          (win->instrument_usage ? XmxSet : XmxNotSet));
    }
  else
    {
      XmxRSetSensitive (win->menubar, mo_instrument_usage,
                        XmxNotSensitive);
    }
#endif
  win->binary_transfer = Rdata.binary_transfer_mode ? 1 : 0;
  XmxRSetToggleState (win->menubar, mo_binary_transfer,
                      (win->binary_transfer ? XmxSet : XmxNotSet));
  
  XmxRSetSensitive (win->menubar, mo_annotate, XmxSensitive);
  XmxRSetSensitive (win->menubar, mo_annotate_edit, XmxNotSensitive);
  XmxRSetSensitive (win->menubar, mo_annotate_delete, XmxNotSensitive);

#ifdef HAVE_AUDIO_ANNOTATIONS
  /* If we're not audio capable, don't provide the menubar entry. */
  if (!mo_audio_capable ())
    XmxRSetSensitive (win->menubar, mo_audio_annotate, XmxNotSensitive);
#endif

  title_label = XmxMakeLabel (form, "Document Title:");
  XmxSetArg (XmNcursorPositionVisible, False);
  XmxSetArg (XmNeditable, False);
  win->title_text = XmxMakeTextField (form);

  url_label = XmxMakeLabel (form, "Document URL:");
  XmxSetArg (XmNcursorPositionVisible, False);
  XmxSetArg (XmNeditable, False);
  win->url_text = XmxMakeTextField (form);

  logo = XmxMakeNamedLabel (form, NULL, "logo");
  XmxApplyBitmapToLabelWidget
    (logo, xmosaic_bits, xmosaic_width, xmosaic_height);

  /* Create a scrolled window. */
  XmxSetArg (XmNscrolledWindowMarginWidth, 5);
  XmxSetArg (XmNscrolledWindowMarginHeight, 5);
  XmxSetArg (XmNscrollBarDisplayPolicy, XmAS_NEEDED);
  XmxSetArg (XmNscrollingPolicy, XmAUTOMATIC);
  win->scrolled_win = 
    XmCreateScrolledWindow (form, "scrolledwin", Xmx_wargs, Xmx_n);
  XtManageChild (win->scrolled_win);
  XmxAddEventHandler(win->scrolled_win, StructureNotifyMask,
                     structure_handler, 0);
  XmxAddEventHandler
    (win->scrolled_win, KeyPressMask, mo_view_keypress_handler, 0);
  Xmx_n = 0;

  /* For some reason the highlightThickness isn't following
     the resources for these scrollbars.  Kill 'em. */
  {
    Widget vsb, hsb;

    XmxSetArg (XmNverticalScrollBar, (long)(&vsb));
    XmxSetArg (XmNhorizontalScrollBar, (long)(&hsb));
    XtGetValues (win->scrolled_win, Xmx_wargs, Xmx_n);
    Xmx_n = 0;

    XmxSetArg (XmNhighlightThickness, 0);
    XmxSetValues (vsb);
    XmxSetArg (XmNhighlightThickness, 0);
    XmxSetValues (hsb);
  }

  XmxSetArg (WbNtext, NULL);
  win->last_width = 0;
  XmxSetArg (XmNresizePolicy, XmRESIZE_ANY);
  XmxSetArg (WbNautoSize, True);
  XmxSetArg (WbNpreviouslyVisitedTestFunction, (long)anchor_visited_predicate);
  XmxSetArg (WbNfancySelections, win->pretty ? True : False);
  win->view = XtCreateWidget ("view", htmlWidgetClass,
                              win->scrolled_win, Xmx_wargs, Xmx_n);
  XtManageChild (win->view);
  mo_register_image_resolution_function (win);
  XmxAddCallback (win->view, WbNanchorCallback, anchor_cb, 0);
  XmxAddCallback (win->view, WbNdocumentPageCallback, document_page_cb, 0);

  XmxAddEventHandler
    (win->view, KeyPressMask, mo_view_keypress_handler, 0);
  /* Need to catch the parent for when the child is too small */
  XmxAddEventHandler
    (XtParent(win->view), KeyPressMask, mo_view_keypress_handler, 0);

  XmxSetArg (XmNworkWindow, (long)(win->view));
  XmxSetValues (win->scrolled_win);

  XmxSetArg (XmNresizePolicy, XmRESIZE_ANY);
  XmxSetArg (XmNresizable, True);
  win->bottom_form = XmxMakeForm (form);
  
  /* Children of win->bottom_form. */
  {
    win->search_form = XmxMakeForm (win->bottom_form);

    /* Children of win->search_form. */
    {
      win->search_label = XmxMakeLabel (win->search_form, "Search Keyword: ");
      win->search_text = XmxMakeTextField (win->search_form);
      XmxAddCallbackToText (win->search_text, searchtext_cb, 0);

      XmxSetOffsets (win->search_label, 4, 0, 0, 0);
      XmxSetConstraints
        (win->search_label, XmATTACH_FORM, XmATTACH_NONE, XmATTACH_FORM,
         XmATTACH_NONE, NULL, NULL, NULL, NULL);
      XmxSetConstraints
        (win->search_text, XmATTACH_FORM, XmATTACH_NONE, XmATTACH_WIDGET,
         XmATTACH_FORM, NULL, NULL, win->search_label, NULL);
    }

    XmxSetArg (XmNmarginWidth, 0);
    XmxSetArg (XmNmarginHeight, 0);
    XmxSetArg (XmNpacking, XmPACK_TIGHT);
    XmxSetArg (XmNresizable, True);
    win->button_rc = XmxMakeHorizontalRowColumn (win->bottom_form);

    /* Children of win->button_rc. */
    {
      win->back_button = XmxMakePushButton 
        (win->button_rc, "Back", menubar_cb, mo_back);
      win->forward_button = XmxMakePushButton 
        (win->button_rc, "Forward", menubar_cb, mo_forward);
      win->home_button = XmxMakePushButton
        (win->button_rc, "Home", menubar_cb, mo_home_document);
      win->reload_button = XmxMakePushButton
        (win->button_rc, "Reload", menubar_cb, mo_reload_document);
      win->open_button = XmxMakePushButton
        (win->button_rc, "Open...", menubar_cb, mo_open_document);
      win->save_button = XmxMakePushButton 
        (win->button_rc, "Save As...", menubar_cb, mo_save_document);
      win->clone_button = XmxMakePushButton 
        (win->button_rc, "Clone", menubar_cb, mo_clone_window);
      win->new_button = XmxMakePushButton
        (win->button_rc, "New Window", menubar_cb, mo_new_window);
      win->close_button = XmxMakePushButton 
        (win->button_rc, "Close Window", menubar_cb, mo_close_window);
    }

    /* Constraints for win->bottom_form. */
    XmxSetOffsets (win->search_form, 3, 0, 10, 10);
    XmxSetConstraints 
      (win->search_form, XmATTACH_FORM, XmATTACH_NONE, XmATTACH_FORM,
       XmATTACH_FORM, NULL, NULL, NULL, NULL);
    XmxSetOffsets (win->button_rc, 10, 5, 10, 10);
    XmxSetConstraints
      (win->button_rc, XmATTACH_WIDGET, XmATTACH_FORM, XmATTACH_FORM,
       XmATTACH_FORM, win->search_form, NULL, NULL, NULL);
  }
  
  /* Constraints for form. */
  XmxSetConstraints 
    (win->menubar->base, XmATTACH_FORM, XmATTACH_NONE, XmATTACH_FORM,
     XmATTACH_FORM, NULL, NULL, NULL, NULL);

  /* Top to menubar, bottom to nothing,
     left to form, right to nothing. */
  XmxSetOffsets (title_label, 12, 0, 10, 0);
  XmxSetConstraints
    (title_label, XmATTACH_WIDGET, XmATTACH_NONE,
     XmATTACH_FORM, XmATTACH_NONE, win->menubar->base,
     NULL, NULL, NULL);
  /* Top to menubar, bottom to nothing,
     Left to title label, right to logo. */
  XmxSetOffsets (win->title_text, 8, 1, 9, 8);
  XmxSetConstraints
    (win->title_text, XmATTACH_WIDGET, XmATTACH_NONE,
     XmATTACH_WIDGET, XmATTACH_WIDGET, win->menubar->base,
     NULL, title_label, logo);
  /* Top to title text, bottom to nothing,
     left to form, right to nothing. */
  XmxSetOffsets (url_label, 12, 0, 10, 0);
  XmxSetConstraints
    (url_label, XmATTACH_WIDGET, XmATTACH_NONE,
     XmATTACH_FORM, XmATTACH_NONE, win->title_text,
     NULL, NULL, NULL);
  /* Top to menubar, bottom to nothing,
     Left to url label, right to logo. */
  XmxSetOffsets (win->url_text, 8, 1, 8, 8);
  XmxSetConstraints
    (win->url_text, XmATTACH_WIDGET, XmATTACH_NONE,
     XmATTACH_WIDGET, XmATTACH_WIDGET, win->title_text,
     NULL, url_label, logo);
  /* Top to menubar, bottom to nothing,
     left to nothing, right to form. */
  XmxSetOffsets (logo, 7, 0, 0, 8);
  XmxSetConstraints
    (logo, XmATTACH_WIDGET, XmATTACH_NONE,
     XmATTACH_NONE, XmATTACH_FORM, win->menubar->base,
     NULL, NULL, NULL);
  /* Top to logo, bottom to bottom form,
     Left to form, right to form. */
  XmxSetOffsets (win->scrolled_win, 2, 2, 2, 2);
  XmxSetConstraints
    (win->scrolled_win, XmATTACH_WIDGET, XmATTACH_WIDGET,
     XmATTACH_FORM, XmATTACH_FORM, logo, win->bottom_form,
     NULL, NULL);

  XmxSetOffsets (win->bottom_form, 10, 2, 2, 2);
  XmxSetConstraints
    (win->bottom_form, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_FORM,
     XmATTACH_FORM, NULL, NULL, NULL, NULL);

  /* Can't go back or forward if we haven't gone anywhere
     yet... */
  mo_back_impossible (win);
  mo_forward_impossible (win);

  return;
}

/* -------------------------- mo_delete_window -------------------------- */

#define POPDOWN(x) \
  if (win->x) XtUnmanageChild (win->x)

mo_status mo_delete_window (mo_window *win)
{
  mo_node *node, *foo;

  if (!win)
    return mo_fail;

  node = win->history;

  POPDOWN (source_win);
  POPDOWN (save_win);
  POPDOWN (savebinary_win);
  POPDOWN (open_win);
  POPDOWN (mail_win);
  POPDOWN (mailhot_win);
  POPDOWN (edithot_win);
  POPDOWN (mailhist_win);
  POPDOWN (print_win);
  POPDOWN (history_win);
  POPDOWN (open_local_win);
  POPDOWN (hotlist_win);
  POPDOWN (whine_win);
  POPDOWN (annotate_win);
  POPDOWN (search_win);
#ifdef HAVE_DTM
  POPDOWN (dtmout_win);
#endif
#ifdef HAVE_AUDIO_ANNOTATIONS
  POPDOWN (audio_annotate_win);
#endif
  XtPopdown (win->base);

  if (node)
    {
      for (foo = node->next; foo != NULL; foo = foo->next)
        mo_free_node_data (foo);
      mo_free_node_data (node);
    }

  /* This will free the win structure (but none of its elements
     individually) and exit if this is the last window in the list. */
  mo_remove_window_from_list (win);

  /* Go get another current_win. */
  mo_set_current_cached_win (mo_next_window (NULL));

  return mo_succeed;
}

/* ---------------------------- mo_open_window ---------------------------- */

/* Actually make the mo_window struct and fill up the GUI. */
/* We now pass in a parent window.  This can be NULL. */
static mo_window *mo_open_window_internal (Widget base, mo_window *parent)
{
  mo_window *win;

  win = (mo_window *)malloc (sizeof (mo_window));
  win->id = XmxMakeNewUniqid ();
  XmxSetUniqid (win->id);

  win->base = base;

  win->source_win = 0;
  win->save_win = 0;
  win->savebinary_win = 0;
  win->open_win = win->open_text = win->open_local_win = 0;
  win->mail_win = win->mailhot_win = win->edithot_win = win->mailhist_win = 0;
  win->print_win = 0;
  win->history_win = win->history_list = 0;
  win->hotlist_win = win->hotlist_list = 0;
  win->whine_win = win->whine_text = 0;
  win->annotate_win = 0;
  win->search_win = win->search_win_text = 0;
#ifdef HAVE_DTM
  win->dtmout_win = win->dtmout_text = 0;
#endif
#ifdef HAVE_AUDIO_ANNOTATIONS
  win->audio_annotate_win = 0;
#endif

  win->history = NULL;
  win->current_node = 0;
  win->source_text = 0;
  win->format_optmenu = 0;
  win->save_format = 0;
  /* If no parent, inherit global value (which will be default value
     even if we're not using the global value).  Else, use the parent
     value (which will be what we want in either case). */
  if (!parent)
    win->window_per_document = global_window_per_document;
  else
    win->window_per_document = parent->window_per_document;
  if (!parent)
    win->font_size = mo_regular_fonts;
  else
    win->font_size = parent->font_size;

  win->underlines_snarfed = 0;
  if (!parent)
    win->underlines_state = mo_default_underlines;
  else
    win->underlines_state = parent->underlines_state;

  win->pretty = Rdata.default_fancy_selections;

  win->mail_format = 0;

#ifdef HAVE_AUDIO_ANNOTATIONS
  win->record_fnam = 0;
  win->record_pid = 0;
#endif
  
  win->print_text = 0;
  win->print_format = 0;

  win->num_hrefs = 0;
  win->hrefs = 0;

  win->can_annotate = win->can_crossref = win->can_checkout = 
    win->can_checkin = 0;
  win->target_anchor = 0;
  /* Create search_start and search_end. */
  win->search_start = (void *)malloc (sizeof (ElementRef));
  win->search_end = (void *)malloc (sizeof (ElementRef));
  
  /* Install all the GUI bits & pieces. */
  mo_fill_window (win);

  /* Register win with internal window list. */
  mo_add_window_to_list (win);

  /* Pop the window up. */
  XtPopup (win->base, XtGrabNone);
  XFlush (dsp);
  XSync (dsp, False);

  /* Set the font size. */
  if (win->font_size != mo_regular_fonts)
    mo_set_fonts (win, win->font_size);

  /* Set the underline state. */
  mo_set_underlines (win, win->underlines_state);
  
  /* Set the fancy selections toggle to the starting value. */
  mo_set_fancy_selections_toggle (win);

  return win;
}

/* ------------------------------ delete_cb ------------------------------- */

static XmxCallback (delete_cb)
{
  mo_window *win = (mo_window *)client_data;
  mo_delete_window (win);
  return;
}

/* ---------------------------- mo_make_window ---------------------------- */

/* This calls mo_open_window_internal after creating the popup shell
   and doing a few other things. */
static mo_window *mo_make_window (Widget parent, mo_window *parentw)
{
  Widget base;
  mo_window *win;
  Atom WM_DELETE_WINDOW;

  XmxSetArg (XmNtitle, (long)"NCSA Mosaic: Document View");
  XmxSetArg (XmNiconName, (long)"Mosaic");
  base = XtCreatePopupShell ("shell", topLevelShellWidgetClass,
                             toplevel, Xmx_wargs, Xmx_n);
  Xmx_n = 0;

#ifdef EDITRES_SUPPORT  
  XtAddEventHandler(base, (EventMask) 0, TRUE,
                    (XtEventHandler) _XEditResCheckMessages, NULL);
#endif

  /* We're not passing in a form anymore... */
  win = mo_open_window_internal (base, parentw);

  WM_DELETE_WINDOW = XmInternAtom(dsp, "WM_DELETE_WINDOW", False);
  XmAddWMProtocolCallback(base, WM_DELETE_WINDOW, delete_cb, (XtPointer)win);
  
  return win;
}

/* ------------------- mo_open_another_window_internal -------------------- */

/* We make sure the new window sits near but not on top of the
   existing window...  we do this by setting XmNx and XmNy,
   then calling mo_open_window. */
static mo_window *mo_open_another_window_internal (mo_window *win)
{
  Dimension oldx, oldy;
  Dimension scrx = WidthOfScreen (XtScreen (win->base));
  Dimension scry = HeightOfScreen (XtScreen (win->base));
  Dimension x, y;
  Dimension width, height;
  mo_window *newwin;

  XtVaGetValues (win->base, XmNx, &oldx, XmNy, &oldy, 
                 XmNwidth, &width, XmNheight, &height, NULL);
  
  /* Ideally we open down and over 40 pixels... is this possible? */
  x = oldx + 40; y = oldy + 40;
  /* If not, deal with it... */
  while (x + width > scrx)
    x -= 80;
  while (y + height > scry)
    y -= 80;
  
  XmxSetArg (XmNdefaultPosition, False);
  if (Rdata.auto_place_windows)
    {
      char geom[20];
      sprintf (geom, "+%d+%d\0", x, y);
      XmxSetArg (XmNgeometry, (long)geom);
    }
  XmxSetArg (XmNwidth, width);
  XmxSetArg (XmNheight, height);

  newwin = mo_make_window (toplevel, win);
  mo_set_current_cached_win (newwin);
  return newwin;
}

/* ---------------------------- mo_open_window ---------------------------- */

/* Width and height do not get set herein. */
mo_window *mo_open_window (Widget parent, char *url, mo_window *parentw)
{
  mo_window *win;

  win = mo_make_window (parent, parentw);

  /* This is for picread.c -- HORRIBLE HACK @@@@ */
  mo_set_current_cached_win (win);

  mo_load_window_text_first (win, url, NULL, NULL);

  return win;
}

/* ------------------------- mo_duplicate_window -------------------------- */

/* Given a window, duplicate it as intelligently as possible. */
mo_window *mo_duplicate_window (mo_window *win)
{
  mo_window *neww = mo_open_another_window_internal (win);
  
  mo_duplicate_window_text (win, neww);

  return neww;
}

/* ------------------------ mo_open_another_window ------------------------ */

/* We make sure the new window sits near but not on top of the
   existing window...  we do this by setting XmNx and XmNy,
   then calling mo_open_window. */
mo_window *mo_open_another_window (mo_window *win, char *url, char *ref,
                                   char *target_anchor)
{
  mo_window *neww;

  /* Check for reference to telnet.  Never open another window
     if reference to telnet exists; instead, call mo_load_window_text,
     which knows how to manage current window's access to telnet. */
  if (!strncmp (url, "telnet:", 7) || !strncmp (url, "tn3270:", 7) ||
      !strncmp (url, "rlogin:", 7))
    {
      mo_load_window_text (win, url, NULL);
      return NULL;
    }

  neww = mo_open_another_window_internal (win);
  /* Set it here; hope it gets handled in mo_load_window_text_first. */
  neww->target_anchor = target_anchor;
  mo_load_window_text_first (neww, url, ref, 
                             win->current_node ? 
                             win->current_node->url : NULL);

  return neww;
}

/* ------------------------------ mo_do_gui ------------------------------- */

char **gargv;
int gargc;

/* from main.c */
#ifdef _HPUX_SOURCE
extern int Interrupt (void);
#else
extern void Interrupt (void);
#endif

#ifndef VMS
#ifdef _HPUX_SOURCE
extern int ProcessExternalDirective (void);
#else
extern void ProcessExternalDirective (void);
#endif
#endif

static XmxCallback (fire_er_up)
{
  char *home_opt;
  mo_window *win;

  /* Pick up default or overridden value out of X resources. */
  home_document = Rdata.home_document;

  /* Value of environment variable WWW_HOME overrides that. */
  if ((home_opt = getenv ("WWW_HOME")) != NULL)
    home_document = home_opt;

  /* Value of argv[1], if it exists, overrides that.
     (All other command-line flags will be picked up by
     X resource mechanism.) */
  if (gargc > 1 && gargv[1] && *gargv[1])
    home_document = gargv[1];

  /* Check for proper home document URL construction. */
  if (!strstr (home_document, ":"))
    {
      /* Apparently trying to load a local file, without file:
         prefix.... */
      char *cwd = getcwd (NULL, 128);
      char *url = (char *)malloc ((strlen (home_document) + 
                                   strlen (cwd) +
                                   strlen (shortmachine) + 8));
      if (home_document[0] == '/')
        sprintf (url, "file://%s%s\0", 
                 shortmachine, 
                 home_document);
      else
        sprintf (url, "file://%s%s/%s\0",
                 shortmachine,
                 cwd,
                 home_document);

      home_document = url;
    }

  XmxSetArg (XmNwidth, Rdata.default_width);
  XmxSetArg (XmNheight, Rdata.default_height);
  if (Rdata.initial_window_iconic)
    {
      XmxSetArg (XmNiconic, True);
    }
  win = mo_open_window (toplevel, home_document, NULL);

  return;
}

/* ------------------------ mo_open_initial_window ------------------------ */

mo_status mo_open_initial_window (void)
{
  /* Set a timer that will actually cause the window to open. */
  XtAppAddTimeOut (app_context, 10, (XtTimerCallbackProc)fire_er_up, True);
  return mo_succeed;
}

/* --------------------------- mo_error_handler --------------------------- */

static int mo_error_handler(Display *dsp, XErrorEvent *event)
{
  char buf[128];

  XUngrabPointer(dsp, CurrentTime);   /* in case error occurred in Grab */

  /* BadAlloc errors (on a XCreatePixmap() call)
     and BadAccess errors on XFreeColors are 'ignoreable' errors */
  if (event->error_code == BadAlloc ||
      (event->error_code == BadAccess && event->request_code == 88)) 
    return 0;
  else 
    {
      /* All other errors are 'fatal'. */
      XGetErrorText (dsp, event->error_code, buf, 128);
      fprintf (stderr, "X Error: %s\n", buf);
      fprintf (stderr, "  Major Opcode:  %d\n", event->request_code);

      /* Try to close down gracefully. */
      mo_exit ();
    }
}

/* ------------------------------ mo_do_gui ------------------------------- */

void mo_do_gui (int argc, char **argv)
{
#ifdef MONO_DEFAULT
  int use_color = 0;
#else
  int use_color = 1;
#endif
  int no_defaults = 0;

  int i;

  for (i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "-mono"))
        use_color = 0;
    }
  for (i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "-color"))
        use_color = 1;
    }
  for (i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "-nd"))
        no_defaults = 1;
    }
  
  /* Motif setup. */
  XmxStartup ();
  XmxSetArg (XmNmappedWhenManaged, False);
  if (no_defaults)
    {
      toplevel = XtAppInitialize 
        (&app_context, "XMosaic", options, XtNumber (options),
         &argc, argv, NULL, Xmx_wargs, Xmx_n);
    }
  else
    {
      if (use_color)
        {
          toplevel = XtAppInitialize 
            (&app_context, "XMosaic", options, XtNumber (options),
             &argc, argv, color_resources, Xmx_wargs, Xmx_n);
        }
      else
        {
          toplevel = XtAppInitialize 
            (&app_context, "XMosaic", options, XtNumber (options),
             &argc, argv, mono_resources, Xmx_wargs, Xmx_n);
        }
    }
  
  /* Needed for picread.c, right now. */
  dsp = XtDisplay (toplevel);
  {
    XVisualInfo vinfo, *vptr;
    int cnt;
    
    vinfo.visualid = 
      XVisualIDFromVisual
        (DefaultVisual (dsp,
                        DefaultScreen (dsp)));
    vptr = 
      XGetVisualInfo (dsp, VisualIDMask, &vinfo, &cnt);
    Vclass = vptr->class;
    XFree((char *)vptr);
  }
  
  XtVaGetApplicationResources (toplevel, (XtPointer)&Rdata, resources,
                               XtNumber (resources), NULL);
  
  XSetErrorHandler (mo_error_handler);

#ifdef __sgi
  /* Turn on debugging malloc if necessary. */
  if (Rdata.debugging_malloc)
    mallopt (M_DEBUG, 1);
#endif

  global_xterm_str = Rdata.xterm_command;
  gif_reader = Rdata.gif_viewer_command;
  hdf_reader = Rdata.hdf_viewer_command;
  jpeg_reader = Rdata.jpeg_viewer_command;
  tiff_reader = Rdata.tiff_viewer_command;
  audio_reader = Rdata.audio_player_command;
  aiff_reader = Rdata.aiff_player_command;
  postscript_reader = Rdata.postscript_viewer_command;
  dvi_reader = Rdata.dvi_viewer_command;
  mpeg_reader = Rdata.mpeg_viewer_command;
  mime_reader = Rdata.mime_viewer_command;
  xwd_reader = Rdata.xwd_viewer_command;
  sgimovie_reader = Rdata.sgimovie_viewer_command;
  evlmovie_reader = Rdata.evlmovie_viewer_command;
  rgb_reader = Rdata.rgb_viewer_command;
  cave_reader = Rdata.cave_viewer_command;

  uncompress_program = Rdata.uncompress_command;
  gunzip_program = Rdata.gunzip_command;
  tweak_gopher_types = Rdata.tweak_gopher_types;

  /* First get the hostname. */
  machine = (char *)malloc (sizeof (char) * 64);
  gethostname (machine, 64);
  
  /* Then make a copy of the hostname for shortmachine.
     Don't even ask. */
  shortmachine = strdup (machine);
  
  /* Then find out the full name, if possible. */
  if (Rdata.full_hostname)
    machine = Rdata.full_hostname;
  else if (!Rdata.gethostbyname_is_evil)
    {
      struct hostent *phe;
      
      phe = gethostbyname (machine);
      if (phe && phe->h_name)
        machine = strdup (phe->h_name);
    }
  /* (Otherwise machine just remains whatever gethostname returned.) */
  
  machine_with_domain = (strlen (machine) > strlen (shortmachine) ?
                         machine : shortmachine);

  /* Set up default value for window_per_document (for at least
     the first window).  Subsequent windows will inherit from their
     previous window; all windows will share the same value if
     Rdata.global_window_per_document is true (this is currently the
     case). */
  global_window_per_document = Rdata.window_per_document;

  /* If there's no tmp directory assigned by the X resource, then
     look at TMPDIR. */
  if (!Rdata.tmp_directory)
    {
      Rdata.tmp_directory = getenv ("TMPDIR");
      /* It can still be NULL when we leave here -- then we'll just
         let tmpnam() do what it does best. */
    }

  mo_setup_default_hotlist ();
  if (Rdata.use_global_history)
    mo_setup_global_history ();
  else
    mo_init_global_history ();
  mo_setup_pan_list ();

  {
    Widget foo = XmxMakeForm (toplevel);
    XmxSetArg
      (XmNlabelString,
       (long)XmxMakeXmstrFromString
       ("NCSA Mosaic"));
    Xmx_w = XtCreateManagedWidget ("blargh", xmLabelGadgetClass,
                                   foo, Xmx_wargs, Xmx_n);
    Xmx_n = 0;
  }

  busy_cursor = XCreateFontCursor (dsp, XC_watch);

  XtRealizeWidget (toplevel);

  gargv = argv;
  gargc = argc;

  signal (SIGINT, Interrupt);
#ifndef VMS
  signal (SIGUSR1, ProcessExternalDirective);
#endif

#ifdef INSTRUMENT
  mo_startup ();
#else
  defer_initial_window = 0;
#endif

  if (!defer_initial_window)
    mo_open_initial_window ();
  
  XtAppMainLoop (app_context);
}

/* ---------------------- mo_recover_from_interrupt ----------------------- */

void mo_recover_from_interrupt (void)
{
  mo_not_busy ();
  XtAppMainLoop (app_context);
}

/* -------------------- mo_process_external_directive --------------------- */

#define FORCE_REFRESH(win) \
{ \
    /* Force it to !#%!@#%@#$%@!#%! refresh. */ \
    XClearWindow (dsp, XtWindow (win->base)); \
    XClearWindow (dsp, XtWindow (win->view)); \
    XmUpdateDisplay (win->base); \
    XmUpdateDisplay (win->view); \
}

#define CLIP_TRAILING_NEWLINE(url) \
  if (url[strlen (url) - 1] == '\n') \
    url[strlen (url) - 1] = '\0';

void mo_process_external_directive (char *directive, char *url)
{
  /* Process a directive that we received externally. */
  mo_window *win = current_win;

  /* Make sure we have a window. */
  if (!win)
    win = mo_next_window (NULL);

  if (!strncmp (directive, "goto", 4))
    {
      CLIP_TRAILING_NEWLINE(url);

      /* Call access_document: this will cause a new window
         to open if the user has chosen window per document;
         maybe that's a bad idea. */
      mo_access_document (win, url);

      FORCE_REFRESH(win);
    }

  if (!strncmp (directive, "newwin", 6))
    {
      CLIP_TRAILING_NEWLINE(url);

      /* Force a new window to open. */ 
      mo_open_another_window (win, url, NULL, NULL);

      FORCE_REFRESH(win);
    }

  return;
}
