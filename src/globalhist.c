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

#include "libhtmlw/HTML.h" /* for ImageInfo */

/* ------------------------------------------------------------------------ */
/* ---------------------------- GLOBAL HISTORY ---------------------------- */
/* ------------------------------------------------------------------------ */

/* We save history list out to a file (~/.mosaic-global-history) and
   reload it on entry.

   Initially the history file format will look like this:

   ncsa-mosaic-history-format-1            [identifying string]
   Global                                  [title]
   url Fri Sep 13 00:00:00 1986            [first word is url;
                                            subsequent words are
                                            last-accessed date (GMT)]
   [1-line sequence for single document repeated as necessary]
   ...
*/

#define NCSA_HISTORY_FORMAT_COOKIE_ONE "ncsa-mosaic-history-format-1"

#define HASH_TABLE_SIZE 200

/* An entry in a hash bucket, containing a URL (in canonical,
   absolute form) and possibly cached info (right now, an ImageInfo
   struct for inlined images). */
typedef struct entry
{
  /* Canonical URL for this document. */
  char *url;
  /* Cached image info, if appropriate. */
  ImageInfo *imginfo;
  struct entry *next;
} entry;

/* A bucket in the hash table; contains a linked list of entries. */
typedef struct bucket
{
  entry *head;
  int count;
} bucket;

static bucket hash_table[HASH_TABLE_SIZE];

/* Given a URL, hash it and return the hash value, mod'd by the size
   of the hash table. */
static int hash_url (char *url)
{
  int len, i, val;

  if (!url)
    return 0;
  len = strlen (url);
  val = 0;
  for (i = 0; i < 10; i++)
    val += url[(i * val + 7) % len];

  return val % HASH_TABLE_SIZE;
}

/* Each bucket in the hash table maintains a count of the number of
   entries contained within; this function dumps that information
   out to stdout. */
static void dump_bucket_counts (void)
{
  int i;

  for (i = 0; i < HASH_TABLE_SIZE; i++)
    fprintf (stdout, "Bucket %03d, count %03d\n", i, hash_table[i].count);
  
  return;
}

/* Presumably url isn't already in the bucket; add it by
   creating a new entry and sticking it at the head of the bucket's
   linked list of entries. */
static void add_url_to_bucket (int buck, char *url)
{
  bucket *bkt = &(hash_table[buck]);
  entry *l;

  l = (entry *)malloc (sizeof (entry));
  l->url = strdup (url);
  l->imginfo = NULL;
  l->next = NULL;
  
  if (bkt->head == NULL)
    bkt->head = l;
  else
    {
      l->next = bkt->head;
      bkt->head = l;
    }

  bkt->count += 1;
}

/* This is the internal predicate that takes a URL, hashes it,
   does a search through the appropriate bucket, and either returns
   1 or 0 depending on whether we've been there. */
static int been_here_before (char *url)
{
  int hash = hash_url (url);
  entry *l;

  if (hash_table[hash].count)
    for (l = hash_table[hash].head; l != NULL; l = l->next)
      {
        if (!strcmp (l->url, url))
          return 1;
      }
  
  return 0;
}


/* 
 * Have we been here before?
 * See if the url is in the hash table yet.
 * If it's not, return mo_fail, else mo_succeed.
 *
 * This is the external interface; just call been_here_before().
 */
mo_status mo_been_here_before_huh_dad (char *url)
{
  if (been_here_before (url))
    return mo_succeed;
  else
    return mo_fail;
}

/* 
 * We're now here.  Take careful note of that fact.
 *
 * Add the current URL to the appropriate bucket if it's not
 * already there.
 */
mo_status mo_here_we_are_son (char *url)
{
  if (!been_here_before (url))
    add_url_to_bucket (hash_url (url), url);
  
  return mo_succeed;
}

/* ------------------------------------------------------------------------ */

/* Given a filename for the global history file, load the contents
   into the global hashtable. */
static void mo_read_global_history (char *filename)
{
  FILE *fp;
  char line[MO_LINE_LENGTH];
  char *status;
  
  fp = fopen (filename, "r");
  if (!fp)
    goto screwed_no_file;
  
  status = fgets (line, MO_LINE_LENGTH, fp);
  if (!status || !(*line))
    goto screwed_with_file;
  
  /* See if it's our format. */
  if (strncmp (line, NCSA_HISTORY_FORMAT_COOKIE_ONE,
               strlen (NCSA_HISTORY_FORMAT_COOKIE_ONE)))
    goto screwed_with_file;

  /* Go fetch the name on the next line. */
  status = fgets (line, MO_LINE_LENGTH, fp);
  if (!status || !(*line))
    goto screwed_with_file;
  
  /* Start grabbing url's. */
  while (1)
    {
      char *url;
      
      status = fgets (line, MO_LINE_LENGTH, fp);
      if (!status || !(*line))
        goto done;
      
      url = strtok (line, " ");
      if (!url)
        goto screwed_with_file;
      url = strdup (url);

      /* We don't use the last-accessed date... yet. */
      /* lastdate = strtok (NULL, "\n"); blah blah blah... */

      add_url_to_bucket (hash_url (url), url);
    }
  
 screwed_with_file:
 done:
  fclose (fp);
  return;

 screwed_no_file:
  return;
}

/*
 * Called upon program initialization.
 * Sets up the global history from the init file.
 * Also coincidentally initializes history structs.
 * Returns mo_succeed.
 */
static char *cached_global_hist_fname = NULL;

mo_status mo_init_global_history (void)
{
  int i;

  /* Initialize the history structs. */
  for (i = 0; i < HASH_TABLE_SIZE; i++)
    {
      hash_table[i].count = 0;
      hash_table[i].head = 0;
    }

  return mo_succeed;
}

mo_status mo_wipe_global_history (void)
{
  /* Memory leak! @@@ */
  mo_init_global_history ();

  return;
}

mo_status mo_setup_global_history (void)
{
  char *home = getenv ("HOME");
  char *default_filename = Rdata.global_history_file;
  char *filename;

  mo_init_global_history ();

  /* This shouldn't happen. */
  if (!home)
    home = "/tmp";
  
  filename = (char *)malloc 
    ((strlen (home) + strlen (default_filename) + 8) * sizeof (char));
  sprintf (filename, "%s/%s\0", home, default_filename);
  cached_global_hist_fname = filename;

  mo_read_global_history (filename);
  
  return mo_succeed;
}

mo_status mo_write_global_history (void)
{
  FILE *fp;
  int i;
  entry *l;
  time_t foo = time (NULL);
  char *ts = ctime (&foo);

  ts[strlen(ts)-1] = '\0';
 
  fp = fopen (cached_global_hist_fname, "w");
  if (!fp)
    return mo_fail;

  fprintf (fp, "%s\n%s\n", NCSA_HISTORY_FORMAT_COOKIE_ONE, "Global");
  
  for (i = 0; i < HASH_TABLE_SIZE; i++)
    {
      for (l = hash_table[i].head; l != NULL; l = l->next)
        {
          fprintf (fp, "%s %s\n", l->url, ts);
        }
    }
  
  fclose (fp);
  
  return mo_succeed;
}

/* ------------------------- image caching stuff -------------------------- */

void *mo_fetch_cached_image_info (char *url)
{
  int hash = hash_url (url);
  entry *l;

  if (hash_table[hash].count)
    for (l = hash_table[hash].head; l != NULL; l = l->next)
      {
        if (!strcmp (l->url, url))
          {
            return (ImageInfo *)(l->imginfo);
          }
      }
  
  /* If we couldn't find the entry at all, obviously we didn't
     cache the image. */
  return NULL;
}

mo_status mo_cache_image_info (char *url, void *info)
{
  int hash = hash_url (url);
  entry *l;

  /* First, register ourselves if we're not already registered. */
  /* Call external interface since it checks to prevent redundancy. */
  mo_here_we_are_son (url);

  /* Then, find the right entry. */
  if (hash_table[hash].count)
    for (l = hash_table[hash].head; l != NULL; l = l->next)
      {
        if (!strcmp (l->url, url))
          goto found;
      }
  
  return mo_fail;

 found:
  l->imginfo = (ImageInfo *)info;
  return mo_succeed;
}
