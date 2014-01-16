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

/* Initially we just keep a linked list, in order of time,
   of all documents accessed. */

typedef struct entry
{
  int action;
  char *url;
  time_t entry_time;
  struct entry *next;
} entry;

static entry *list = NULL;
static entry *list_end = NULL;

mo_status mo_inst_register_action (int action, char *url)
{
  entry *l;
  time_t t = time (NULL);

  l = (entry *)malloc (sizeof (entry));
  l->action = action;
  l->url = url;
  l->entry_time = t;
  l->next = NULL;

  if (!list)
    {
      list = l;
      list_end = l;
    }
  else
    {
      list_end->next = l;
      list_end = l;
    }

  return mo_succeed;
}

mo_status mo_inst_dump_and_flush_data (void)
{
  entry *l, *foo;
  char *fnam, *cmd;
  FILE *fp;
  char *actions[10];
  
  actions[0] = "view"; /* done */
  actions[1] = "annotate"; /* done */
  actions[2] = "edit_annotation"; /* done */
  actions[3] = "delete_annotation"; /* done */
  actions[4] = "whine"; /* done */
  actions[5] = "print"; /* done */
  actions[6] = "mail"; /* done */
  actions[7] = "save"; /* done */
  actions[8] = "dtm_broadcast"; /* done */
  actions[9] = "audio_annotate"; /* done */

  fnam = mo_tmpnam();
  
  fp = fopen (fnam, "w");
  if (!fp)
    goto oops;
  fprintf (fp, "Mosaic Instrumentation Statistics Format v1.0\n");
  fprintf (fp, "Mosaic for X version %s on %s.\n", MO_VERSION_STRING,
           MO_MACHINE_TYPE);
  l = list;
  while (l != NULL)
    {
      fprintf (fp, "%s %s %s\0", actions[l->action], l->url, ctime (&(l->entry_time)));
      foo = l;
      l = l->next;
      free (foo);
    }
  fclose (fp);

  cmd = (char *)malloc ((64 + strlen (Rdata.mail_command) + 
                         strlen (MO_INSTRUMENTATION_ADDRESS) +
                         strlen (fnam) + 24) * sizeof (char));
  sprintf (cmd, "%s -s \"NCSA Mosaic Instrumentation: v%s on %s.\" %s < %s", 
           Rdata.mail_command,
           MO_VERSION_STRING, MO_MACHINE_TYPE, MO_INSTRUMENTATION_ADDRESS, 
           fnam);
  if ((system (cmd)) != 0)
    {
      fprintf (stderr, "Note: While closing itself down, Mosaic was unable to\n");
      fprintf (stderr, "      mail its instrumentation log to NCSA.  Please\n");
      fprintf (stderr, "      check the value of the X resource mailCommand\n");
      fprintf (stderr, "      and make sure it references a valid mail program.\n");
      fprintf (stderr, "      If you need help, please contact us at\n");
      fprintf (stderr, "      'mosaic-x@ncsa.uiuc.edu'.  Thank you.\n");
    }
  free (cmd);
  
  cmd = (char *)malloc ((strlen (fnam) + 32) * sizeof (char));
  sprintf (cmd, "/bin/rm -f %s &", fnam);
  system (cmd);
  free (fnam);
  free (cmd);
  
 oops:
  return mo_succeed;
}
