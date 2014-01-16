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

void mo_exit (void)
{
  mo_write_default_hotlist ();
  if (Rdata.use_global_history)
    mo_write_global_history ();
  mo_write_pan_list ();

  if (can_instrument && Rdata.instrument_usage)
    mo_inst_dump_and_flush_data ();

  exit (0);
}

/* Extern to mo_do_gui. */
#ifdef _HPUX_SOURCE
int Interrupt (void)
#else /* not HP */
void Interrupt (void)
#endif
{
  /* Apparently this doesn't work on the Sun. */
  signal (SIGINT, Interrupt);
  mo_recover_from_interrupt ();
  /* NOTREACHED */
}

#ifndef VMS
#ifdef _HPUX_SOURCE
int ProcessExternalDirective (void)
#else /* not HP */
void ProcessExternalDirective (void)
#endif
{
  char filename[64];
  char line[MO_LINE_LENGTH], *status, *directive, *url;
  FILE *fp;
  extern void mo_process_external_directive (char *directive, char *url);

  signal (SIGUSR1, SIG_IGN);

  /* Construct filename from our pid. */
  sprintf (filename, "/tmp/xmosaic.%d", getpid ());

  fp = fopen (filename, "r");
  if (!fp)
    goto done;

  status = fgets (line, MO_LINE_LENGTH, fp);
  if (!status || !(*line))
    goto done;
  directive = strdup (line);

  status = fgets (line, MO_LINE_LENGTH, fp);
  if (!status || !(*line))
    {
      free (directive);
      goto done;
    }
  url = strdup (line);

  mo_process_external_directive (directive, url);

  free (directive);
  free (url);

 done:
  signal (SIGUSR1, ProcessExternalDirective);
  return;
}  
#endif

static void RealFatal (void)
{
  signal (SIGBUS, 0);
  signal (SIGSEGV, 0);
  signal (SIGILL, 0);
  abort ();
}

#ifdef _HPUX_SOURCE
static int FatalProblem(int sig, int code, struct sigcontext *scp,
                        char *addr)
#else
static void FatalProblem(int sig, int code, struct sigcontext *scp,
                         char *addr)
#endif
{
  fprintf (stderr, "\nCongratulations, you have found a bug in\n");
  fprintf (stderr, "NCSA Mosaic %s on %s.\n\n", MO_VERSION_STRING, 
           MO_MACHINE_TYPE);
  fprintf (stderr, "If a core file was generated in your directory,\n");
  fprintf (stderr, "please run 'dbx xmosaic' (or 'dbx /path/xmosaic' if the\n\
xmosaic executable is not in your current directory)\n\
and then type:\n");
  fprintf (stderr, "  dbx> where\n");
  fprintf (stderr, "and mail the results, and a description of what you were doing at the time,\n");
  fprintf (stderr, "to %s.  We thank you for your support.\n\n", 
           MO_DEVELOPER_ADDRESS);
  fprintf (stderr, "...exiting NCSA Mosaic now.\n\n");

  RealFatal ();
}

main (int argc, char **argv)
{
  signal (SIGBUS, FatalProblem);
  signal (SIGSEGV, FatalProblem);
  signal (SIGILL, FatalProblem);

  /* SIGINT and SIGUSR1 signals set in mo_do_gui now. */

  mo_do_gui (argc, argv);
}
