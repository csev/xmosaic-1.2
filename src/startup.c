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

/* We look for a file called ~/.mosaicusr.  If it exists, we read it
   and look for this format:

   ncsa-mosaic-usr-format-1
   instrument yes                    [or instrument no]

   If it doesn't exist, we put up the initial splash screen
   and ask the user what to do, what to do. */

#define MO_USR_FILE_NAME ".mosaicusr"
#define NCSA_USR_FORMAT_COOKIE_ONE "ncsa-mosaic-usr-format-1"

#define YAK_YAK_YAK \
"Welcome to NCSA Mosaic for the X Window System.\n\n\
NCSA Mosaic is part of a research program being conducted by the\n\
National Center for Supercomputing Applications at the University of Illinois\n\
at Urbana-Champaign.  We are studying how information is discovered, retrieved,\n\
manipulated, published, and shared on wide-area networks like the Internet.\n\n\
As part of this research, we would like to ask you to allow us to instrument\n\
your use of NCSA Mosaic.  If you agree to participate, each time you use Mosaic\n\
a log of the activities you undertake will be sent via email to NCSA, where it\n\
will be added to a large and growing pool of such information and used as the\n\
basis for our research.\n\n\
All such information will be held in the strictest confidence and never released\n\
to anyone but the principle investigators of the research program (the lead\n\
investigator is Joseph Hardin, hardin@ncsa.uiuc.edu).  Further, you will have the\n\
option of turning off Mosaic's built-in instrumentation at any time, temporarily\n\
or permanently.  We ask you to participate in this study as doing so will allow us\n\
both to fulfill our research goals and to build more and better software for you\n\
to use.\n\n\
Please choose either 'Participate' or 'Don't Participate' below.\n\
We thank you for your support.\n"

extern int defer_initial_window;
extern Widget toplevel;

static Widget dialog_win;

/* ------------------------ mo_post_startup_dialog ------------------------ */

static XmxCallback (dialog_win_cb)
{
  FILE *fp;
  char *home = getenv ("HOME"), *filename;

  switch (XmxExtractToken ((int)client_data))
    {
    case 0:
      /* participate */
      can_instrument = 1;
      break;
    case 1:
      /* don't participate */
      can_instrument = 0;
      break;
    }

  if (!home)
    home = "/tmp";
  
  filename = (char *)malloc (strlen (home) + 
                             strlen (MO_USR_FILE_NAME) + 8);
  sprintf (filename, "%s/%s\0", home, MO_USR_FILE_NAME);
  fp = fopen (filename, "w");
  if (!fp)
    goto never_mind;
  
  fprintf (fp, "%s\n", NCSA_USR_FORMAT_COOKIE_ONE);
  fprintf (fp, "instrument %s\n", can_instrument ? "yes" : "no");
  
  fclose (fp);
  
 never_mind:
  free (filename);
  XtUnmanageChild (dialog_win);
  mo_open_initial_window ();
}

static void mo_post_startup_dialog (void)
{
  Widget dialog_frame, dialog_form;
  Widget dialog_sep, buttons_form;
  Widget label, logo;

  dialog_win = XmxMakeFormDialog
    (toplevel, "NCSA Mosaic: Startup Questionnaire");
  dialog_frame = XmxMakeFrame (dialog_win, XmxShadowOut);

  /* Constraints for dialog_win. */
  XmxSetConstraints 
    (dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
     XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
      
  dialog_form = XmxMakeForm (dialog_frame);

  label = XmxMakeLabel (dialog_form, YAK_YAK_YAK);
  
  dialog_sep = XmxMakeHorizontalSeparator (dialog_form);
  
  buttons_form = XmxMakeFormAndTwoButtons
    (dialog_form, dialog_win_cb, "Participate", "Don't Participate", 0, 1);
  
  XmxSetOffsets (label, 10, 10, 10, 10);
  XmxSetConstraints
    (label, XmATTACH_FORM, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_FORM,
     NULL, NULL, NULL, NULL);
  XmxSetConstraints
    (dialog_sep, XmATTACH_WIDGET, XmATTACH_WIDGET, 
     XmATTACH_FORM, XmATTACH_FORM,
     label, buttons_form, NULL, NULL);
  XmxSetConstraints
    (buttons_form, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_FORM,
     NULL, NULL, NULL, NULL);
  
  XtManageChild (dialog_win);
  
  return;
}

/* ------------------------------ mo_startup ------------------------------ */

mo_status mo_startup (void)
{
  FILE *fp;
  char *home = getenv ("HOME"), *filename;

  if (!home)
    home = "/tmp";

  filename = (char *)malloc (strlen (home) + strlen (MO_USR_FILE_NAME) + 8);
  sprintf (filename, "%s/%s\0", home, MO_USR_FILE_NAME);
  fp = fopen (filename, "r");
  if (!fp)
    goto new_user;

  /* File exists; read it. */
  {
    char line[MO_LINE_LENGTH];
    char *status;

    /* Check for starting line. */
    status = fgets (line, MO_LINE_LENGTH, fp);
    if (!status || !(*line))
      {
        fclose (fp);
        goto new_user;
      }

    if (strncmp (line, NCSA_USR_FORMAT_COOKIE_ONE,
                 strlen (NCSA_USR_FORMAT_COOKIE_ONE)))
      {
        fclose (fp);
        goto new_user;
      }

    /* Check for instrument permission line. */
    status = fgets (line, MO_LINE_LENGTH, fp);
    if (!status || !(*line))
      {
        fclose (fp);
        goto new_user;
      }

    if (strncmp (line, "instrument ", strlen ("instrument ")))
      {
        fclose (fp);
        goto new_user;
      }
    if (!strncmp (line, "instrument yes", strlen ("instrument yes")))
      can_instrument = 1;
    else
      can_instrument = 0;
  }
  
  fclose (fp);
  free (filename);
  defer_initial_window = 0;
  return mo_succeed;
  
 new_user:
  free (filename);
  defer_initial_window = 1;
  mo_post_startup_dialog ();
  
  return 0 ;
}
