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

#ifndef __MOSAIC_H__
#define __MOSAIC_H__

/* --------------------------- SYSTEM INCLUDES ---------------------------- */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#if !defined(VMS) && !defined(NeXT)
#include <unistd.h>
#endif /* not VMS and not NeXT */
#include <stdlib.h>
#include <sys/types.h>

#ifdef __sgi
#include <malloc.h>
#endif

#include "Xmx.h"

/* ------------------------------------------------------------------------ */
/* -------------------------------- MACROS -------------------------------- */
/* ------------------------------------------------------------------------ */

#define MO_MAX(x,y) ((x) > (y) ? (x) : (y))
#define MO_MIN(x,y) ((x) > (y) ? (y) : (x))

#define MO_VERSION_STRING "1.2"
#define MO_HELP_ON_VERSION_DOCUMENT \
  "http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/Docs/help-on-version-1.2.html"
#define MO_DEVELOPER_ADDRESS "mosaic-x@ncsa.uiuc.edu"
#define MO_INSTRUMENTATION_ADDRESS "csmdude@www.ncsa.uiuc.edu"

#if defined(__hpux)
#define MO_MACHINE_TYPE "HP-UX"
#endif
#if defined(__sgi)
#define MO_MACHINE_TYPE "Silicon Graphics"
#endif
#if defined(ultrix)
#define MO_MACHINE_TYPE "DEC Ultrix"
#endif
#if defined(_IBMR2)
#define MO_MACHINE_TYPE "RS/6000 AIX"
#endif
#if defined(sun)
#define MO_MACHINE_TYPE "Sun"
#endif
#if defined(NEXT)
#define MO_MACHINE_TYPE "NeXT BSD"
#endif
#if defined(cray)
#define MO_MACHINE_TYPE "Cray"
#endif
#if defined(VMS)
#define MO_MACHINE_TYPE "VMS"
#endif
#if defined(NeXT)
#define MO_MACHINE_TYPE "NeXT"
#endif
#ifndef MO_MACHINE_TYPE
#define MO_MACHINE_TYPE "Unknown Platform"
#endif

#ifdef __hpux
#define HAVE_AUDIO_ANNOTATIONS
#else
#if defined(__sgi) || defined(sun)
#define HAVE_AUDIO_ANNOTATIONS
#endif /* if */
#endif /* ifdef */

/* Be safe... some URL's get very long. */
#define MO_LINE_LENGTH 2048

/* Use builtin strdup when appropriate. */
#if defined(ultrix) || defined(VMS) || defined(NeXT)
extern char *strdup ();
#endif

extern int can_instrument;

#if defined(sun) && defined(__svr4__)
#define bcopy(a, b, c) memmove(b, a, c)
#define bzero(a, b)    memset(a, 0, b)
#endif

/* ------------------------------------------------------------------------ */
/* ------------------------------ MAIN TYPES ------------------------------ */
/* ------------------------------------------------------------------------ */

/* ------------------------------ mo_window ------------------------------- */

/* mo_window contains everything related to a single Document View
   window, including subwindow details. */
typedef struct mo_window
{
  int id;
  Widget base;

  /* Subwindows. */
  Widget source_win;
  Widget save_win;
  Widget savebinary_win;  /* for binary transfer mode */
  Widget open_win;
  Widget mail_win;
  Widget mailhot_win;
  Widget edithot_win;
  Widget mailhist_win;
  Widget print_win;
  Widget history_win;
  Widget open_local_win;
  Widget hotlist_win;
  Widget whine_win;
  Widget annotate_win;
  Widget search_win;
#ifdef HAVE_DTM
  Widget dtmout_win;
#endif
#ifdef HAVE_AUDIO_ANNOTATIONS
  Widget audio_annotate_win;
#endif

  XmxMenuRecord *menubar;
  Widget url_text;
  Widget title_text;

  Widget scrolled_win, view;
  Widget bottom_form;
  Widget search_form, search_label, search_text;
  Widget button_rc, back_button, forward_button, save_button,
    clone_button, close_button, reload_button, open_button,
    new_button;

  Widget home_button;

  int last_width;

  struct mo_node *history;
  struct mo_node *current_node;

  int num_hrefs;
  char **hrefs;
  char *target_anchor;

  /* Document source window. */
  Widget source_text;
  Widget source_url_text;
  XmxMenuRecord *format_optmenu;
  int save_format; /* starts at 0 */

  Widget open_text;

  Widget mail_to_text;
  Widget mail_subj_text;
  XmxMenuRecord *mail_fmtmenu;
  int mail_format;

  Widget mailhot_to_text;
  Widget mailhot_subj_text;
  Widget mailhist_to_text;
  Widget mailhist_subj_text;

  Widget edithot_text;
  int edithot_pos;

  Widget print_text;
  XmxMenuRecord *print_fmtmenu;
  int print_format;

  Widget history_list;
  Widget future_list;

  Widget hotlist_list;

  Widget whine_text;
  
  int font_size;
  int pretty;

  int underlines_snarfed;
  int underlines_state;
  /* Default values only, mind you. */
  int underlines;
  int visited_underlines;
  Boolean dashed_underlines;
  Boolean dashed_visited_underlines;

#ifdef HAVE_DTM
  Widget dtmout_text;
#endif /* HAVE_DTM */

#ifdef HAVE_AUDIO_ANNOTATIONS
  Widget audio_start_button;
  Widget audio_stop_button;
  pid_t record_pid;
  char *record_fnam;
#endif

  Widget annotate_author;
  Widget annotate_title;
  Widget annotate_text;
  Widget delete_button;
  Widget include_fsb;
  int annotation_mode;
  int editing_id;

  char *cached_url;

  Widget search_win_text;
  Widget search_caseless_toggle;
  Widget search_backwards_toggle;
  void *search_start;
  void *search_end;

  /* Bitfield. */
  int can_annotate : 1;
  int can_crossref : 1;
  int can_checkout : 1;
  int can_checkin  : 1;
  int window_per_document : 1;
  int instrument_usage : 1;
  int binary_transfer : 1;
  
  struct mo_window *next;

#ifdef GRPAN_PASSWD
  Widget passwd_label;
  Widget annotate_passwd;
  Widget passwd_toggle;
#endif
  XmxMenuRecord *pubpri_menu;
  int pubpri;  /* one of mo_annotation_[public,private] */
  XmxMenuRecord *audio_pubpri_menu;
  int audio_pubpri;  /* one of mo_annotation_[public,private] */
#ifdef NOPE_NOPE_NOPE
  XmxMenuRecord *title_menu;
  int title_opt;  /* mo_document_title or mo_document_url */
  Widget annotate_toggle;
  Widget crossref_toggle;
  Widget checkout_toggle;
  Widget checkin_toggle;
#endif
} mo_window;

/* ------------------------------- mo_node -------------------------------- */

/* mo_node is a component of the linear history list.  A single
   mo_node will never be effective across multiple mo_window's;
   each window has its own linear history list. */
typedef struct mo_node
{
  char *title;
  char *url;
  char *ref;  /* how the node was referred to from a previous anchor,
                 if such an anchor existed. */
  char *text;
  char *texthead;   /* head of the alloc'd text -- this should
                       be freed, NOT text */
  /* Position in the list, starting at 1; last item is
     effectively 0 (according to the XmList widget). */
  int position;

  /* The type of annotation this is (if any) */
  int annotation_type;

  /* Cached position in the document when it was last accessed. */
  /* It's not clear that this is needed anymore -- probably we just
     need docid. */
  int scrpos;
  /* This is returned from HTMLPositionToId. */
  int docid;

  struct mo_node *previous;
  struct mo_node *next;
} mo_node;

/* ------------------------------ mo_hotnode ------------------------------ */

/* mo_hotnode is a single item in a mo_hotlist. */
typedef struct mo_hotnode
{
  char *title; /* cached */
  char *url;
  char *lastdate;
  /* Position in the list; starting at 1... */
  int position;

  struct mo_hotnode *previous;
  struct mo_hotnode *next;
} mo_hotnode;

/* ------------------------------ mo_hotlist ------------------------------ */

/* mo_hotlist is a list of URL's and (cached) titles that can be
   added to and deleted from freely, and stored and maintained across
   sessions. */
typedef struct mo_hotlist
{
  mo_hotnode *nodelist;
  /* Point to last element in nodelist for fast appends. */
  mo_hotnode *nodelist_last;

  /* Filename for storing this hotlist to local disk; example is
     $HOME/.mosaic-hotlist-default. */
  char *filename;

  /* Name for this hotlist.  This is plain english. */
  char *name;

  /* Flag set to indicate whether this hotlist has to be written
     back out to disk at some point. */
  int modified;

  /* In case we want to string together multiple hotlists... */
  struct mo_hotlist *next;
} mo_hotlist;

/* ------------------------------------------------------------------------ */
/* ------------------------------ MISC TYPES ------------------------------ */
/* ------------------------------------------------------------------------ */

typedef enum
{
  mo_fail = 0, mo_succeed
} mo_status;

typedef enum
{
  mo_annotation_public = 0, mo_annotation_workgroup, mo_annotation_private
} mo_pubpri_token;

typedef enum
{
  mo_inst_view = 0, mo_inst_annotate, mo_inst_edit_annotation,
  mo_inst_delete_annotation, mo_inst_whine,
  mo_inst_print, mo_inst_mail, mo_inst_save, mo_inst_dtm_broadcast,
  mo_inst_audio_annotate, mo_inst_addtohotlist, mo_inst_rmvfromhotlist,
  mo_inst_mail_hotlist, mo_inst_mail_historylist, mo_inst_instoff,
  mo_inst_inston, mo_inst_new_window, mo_inst_clone_window,
  mo_inst_close_window
} mo_inst_action;

typedef struct
{
  Boolean window_per_document;          
  Boolean global_window_per_document;   
  Boolean warp_pointer_for_index;       
  Boolean use_global_history;           
  Boolean static_interface;             
  Boolean display_urls_not_titles;      
  int default_width;                    
  int default_height;                   
  char *home_document;                  
  Boolean confirm_exit;
  char *mail_command;
  char *print_command;
  char *xterm_command;
  char *global_history_file;
  char *default_hotlist_file;
  char *private_annotation_directory;
  Boolean default_fancy_selections;
  char *annotation_server;
  char *default_author_name;
  Boolean annotations_on_top;

  char *gif_viewer_command;
  char *hdf_viewer_command;
  char *jpeg_viewer_command;
  char *tiff_viewer_command;
  char *audio_player_command;
  char *aiff_player_command;
  char *dvi_viewer_command;
  char *mpeg_viewer_command;
  char *mime_viewer_command;
  char *xwd_viewer_command;
  char *sgimovie_viewer_command;
  char *evlmovie_viewer_command;
  char *rgb_viewer_command;
  char *postscript_viewer_command;

  int colors_per_inlined_image;
  Boolean instrument_usage;
  Boolean track_visited_anchors;

  char *uncompress_command;
  char *gunzip_command;

  char *record_command_location;
  char *record_command;

  char *cave_viewer_command;

  char *tmp_directory;
  Boolean catch_prior_and_next;

  char *full_hostname;

  Boolean reverse_inlined_bitmap_colors;

  Boolean confirm_delete_annotation;
  Boolean tweak_gopher_types;

  /* Whether or not unrecognized files are transferred over
     as binary and dumped into local files, or not. */
  Boolean binary_transfer_mode;

  /* If True, we can't call gethostbyname to find out who we are. */
  Boolean gethostbyname_is_evil;

  Boolean auto_place_windows;
  Boolean initial_window_iconic;

#ifdef __sgi
  Boolean debugging_malloc;
#endif
} AppData, *AppDataPtr;

extern AppData Rdata;

/* ------------------------------------------------------------------------ */
/* ------------------------------ PROTOTYPES ------------------------------ */
/* ------------------------------------------------------------------------ */

/* annotate.c */
extern mo_status mo_post_annotate_win 
  (mo_window *win, int, int, char *, char *, char *, char *);
extern char *mo_fetch_annotation_links (char *, int);
extern mo_status mo_is_editable_annotation (mo_window *, char *);
extern mo_status mo_delete_annotation (mo_window *, int);
extern mo_status mo_delete_group_annotation (mo_window *, char *);

#ifdef HAVE_AUDIO_ANNOTATIONS
/* audan.c */
extern mo_status mo_audio_capable (void);
extern mo_status mo_post_audio_annotate_win (mo_window *);
#endif

/* globalhist.c */
extern mo_status mo_been_here_before_huh_dad (char *);
extern mo_status mo_here_we_are_son (char *);
extern mo_status mo_init_global_history (void);
extern mo_status mo_wipe_global_history (void);
extern mo_status mo_setup_global_history (void);
extern mo_status mo_write_global_history (void);
extern void *mo_fetch_cached_image_info (char *);
extern mo_status mo_cache_image_info (char *, void *);

/* grpan.c */
extern char *mo_fetch_grpan_links (char *url);
extern mo_status mo_is_editable_grpan (char *);
extern mo_status mo_audio_grpan (char *url, char *title, char *author,
                               char *data, int len);
extern mo_status mo_new_grpan (char *url, char *title, char *author,
                               char *text);
extern mo_status mo_modify_grpan (char *url, char *title, char *author,
                               char *text);
extern mo_status mo_delete_grpan (char *url);
extern mo_status mo_grok_grpan_pieces 
  (char *, char *, char **, char **, char **, int *, char **);

/* grpan-www.c */
extern char *grpan_doit (char *, char *, char *, int, char **);

/* gui.c */
extern mo_window *mo_next_window (mo_window *);
extern mo_window *mo_fetch_window_by_id (int);
extern mo_status mo_add_window_to_list (mo_window *);
extern mo_status mo_busy (void);
extern mo_status mo_not_busy (void);
extern mo_status mo_set_page_increments (mo_window *, Widget);
extern mo_status mo_set_scrollbar_minimum (mo_window *, Widget);
extern mo_status mo_snarf_scrollbar_values (mo_window *);
extern mo_status mo_load_window_text (mo_window *, char *, char *);
extern mo_status mo_do_window_node (mo_window *, mo_node *);
extern mo_status mo_reload_window_text (mo_window *);
extern mo_status mo_access_document (mo_window *, char *);
extern mo_status mo_delete_window (mo_window *);
extern mo_window *mo_open_window (Widget, char *, mo_window *);
extern mo_window *mo_duplicate_window (mo_window *);
extern mo_window *mo_open_another_window (mo_window *, char *, char *, char *);
extern mo_status mo_open_initial_window (void);
extern void mo_do_gui (int, char **);
extern void mo_recover_from_interrupt (void);

#ifdef HAVE_DMF
extern void dmffield_anchor_cb (char *, char *, mo_window *, int);
#endif

/* gui2.c */
extern mo_status mo_post_save_window (mo_window *);
/* called from libwww */
extern void rename_binary_file (char *);
extern mo_status mo_post_open_local_window (mo_window *);
extern mo_status mo_post_open_window (mo_window *);
extern mo_status mo_post_dtmout_window (mo_window *);
extern mo_status mo_post_mail_window (mo_window *);
extern mo_status mo_post_print_window (mo_window *);
extern mo_status mo_post_source_window (mo_window *);
extern mo_status mo_post_search_window (mo_window *);

/* history.c */
extern mo_status mo_free_node_data (mo_node *);
extern mo_status mo_kill_node (mo_window *, mo_node *);
extern mo_status mo_kill_node_descendents (mo_window *, mo_node *);
extern mo_status mo_add_node_to_history (mo_window *, mo_node *);
extern char *mo_grok_title (mo_window *, char *, char *);
extern mo_status mo_record_visit (mo_window *, char *, char *, 
                                  char *, char *);
extern mo_status mo_back_node (mo_window *win);
extern mo_status mo_forward_node (mo_window *win);
extern mo_status mo_visit_position (mo_window *win, int pos);
extern mo_status mo_load_future_list (mo_window *, Widget);
extern mo_status mo_dump_history (mo_window *win);
extern mo_status mo_post_history_win (mo_window *win);

/* hotlist.c */
extern mo_hotlist *mo_read_hotlist (char *);
extern mo_hotlist *mo_new_hotlist (char *, char *);
extern mo_status mo_write_hotlist (mo_hotlist *);
extern mo_status mo_dump_hotlist (mo_hotlist *);
extern mo_status mo_setup_default_hotlist (void);
extern mo_status mo_write_default_hotlist (void);
extern mo_status mo_post_hotlist_win (mo_window *win);
extern mo_status mo_add_node_to_default_hotlist (mo_node *node);

/* img.c */
extern mo_status mo_register_image_resolution_function (mo_window *);

/* inst.c */
extern mo_status mo_inst_register_action (int, char *);
extern mo_status mo_inst_dump_and_flush_data (void);

/* main.c */
extern void mo_exit (void);

/* mo-www.c */
extern char *mo_pull_er_over (char *, char **);
extern char *mo_pull_er_over_first (char *, char **);
extern mo_status mo_pull_er_over_virgin (char *, char *);
extern char *mo_tmpnam (void);
extern char *mo_get_html_return (char **);
extern char *mo_convert_newlines_to_spaces (char *);

#ifdef HAVE_DMF
/* mo-dmf.c */
extern char *DMFAnchor (char *, char *, char *);
extern char *DMFField (char *, char *, char *);
extern char *DMFQuery (char *, char *, char *);
extern char *DMFInquire (char *, char *, char *);
extern char *DMFItem (char *, char *, char *);
extern void dmf_field_dialog (char *, char *, mo_window *, int);
extern void PrepareDMFDialog (Widget);
extern void PrepareDMFSaveDialog (Widget);
#endif

#ifdef HAVE_DTM
/* mo-dtm.c */
extern mo_status mo_dtm_send_text (mo_window *, char *, char *, char *);
#endif

/* pan.c */
extern mo_status mo_setup_pan_list (void);
extern mo_status mo_write_pan_list (void);
extern mo_status mo_new_pan (char *, char *, char *, char *);
extern char *mo_fetch_pan_links (char *url, int);
extern mo_status mo_delete_pan (int);
extern mo_status mo_modify_pan (int, char *, char *, char *);
extern mo_status mo_is_editable_pan (char *);
extern mo_status mo_grok_pan_pieces 
  (char *, char *, char **, char **, char **, int *, char **);
extern int mo_next_pan_id (void);

/* picread.c */
extern unsigned char *ReadBitmap (char *, int *, int *, XColor *);

/* startup.c */
extern mo_status mo_startup (void);

/* whine.c */
extern mo_status mo_post_whine_win (mo_window *win);

/* ----------------------------- END OF FILE ------------------------------ */

#endif /* not __MOSAIC_H__ */
