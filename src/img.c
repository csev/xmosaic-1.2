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
#include "libhtmlw/HTML.h"
#include "libwww/HTParse.h"

/* Defined in gui.c */
extern char *cached_url;

/* Image resolution function. */
static ImageInfo *Resolve (Widget w, char *src)
{
  int i, cnt;
  unsigned char *data;
  unsigned char *ptr;
  int width, height;
  int Used[256];
  XColor colrs[256];
  ImageInfo *img_data;
  char *txt;
  int foo;
  int len;
  char *fnam;
  int rc;
  extern int Vclass;

  if (src == NULL)
    return(NULL);
  
  /* Here should go hooks for built-in images. */

  /* OK, src is the URL we have to go hunt down.
     First, we go get it. */
  /* Call HTParse!  We can use cached_url here, since
     we set it in do_window_text. */
  src = HTParse(src, cached_url,
                PARSE_ACCESS | PARSE_HOST | PARSE_PATH |
                PARSE_PUNCTUATION);

  /* Go see if we already have the image info hanging around. */
  img_data = mo_fetch_cached_image_info (src);
  if (img_data)
    return (ImageInfo *)img_data;
  
  /* We have to load the image. */
  fnam = mo_tmpnam();
  
  rc = mo_pull_er_over_virgin (src, fnam);
  if (!rc)
    return NULL;

  data = ReadBitmap(fnam, &width, &height, colrs);

  /* Now delete the file. */
  {
    char *cmd = (char *)malloc ((strlen (fnam) + 32) * sizeof (char));
    sprintf (cmd, "/bin/rm -f %s &", fnam);
    system (cmd);
    free (cmd);
  }

  if (data == NULL)
    {
      return(NULL);
    }
  
  img_data = (ImageInfo *)malloc(sizeof(ImageInfo));
  img_data->width = width;
  img_data->height = height;
  for (i=0; i < 256; i++)
    {
      Used[i] = 0;
    }
  cnt = 1;
  ptr = data;
  for (i=0; i < (width * height); i++)
    {
      if (Used[(int)*ptr] == 0)
        {
          Used[(int)*ptr] = cnt;
          cnt++;
        }
      ptr++;
    }
  cnt--;

  /*
   * If the image has too many colors, apply a median cut algorithm to
   * reduce the color usage, and then reprocess it.
   * Don't cut colors for direct mapped visuals like TrueColor.
   */
  if ((cnt > Rdata.colors_per_inlined_image)&&(Vclass != TrueColor))
    {
      MedianCut(data, &width, &height, colrs, cnt, 
                Rdata.colors_per_inlined_image);

      for (i=0; i < 256; i++)
	{
	  Used[i] = 0;
	}
      cnt = 1;
      ptr = data;
      for (i=0; i < (width * height); i++)
	{
	  if (Used[(int)*ptr] == 0)
	    {
	      Used[(int)*ptr] = cnt;
	      cnt++;
	    }
	  ptr++;
	}
      cnt--;
    }

  img_data->num_colors = cnt;
  img_data->reds = (int *)malloc(sizeof(int) * cnt);
  img_data->greens = (int *)malloc(sizeof(int) * cnt);
  img_data->blues = (int *)malloc(sizeof(int) * cnt);
  
  for (i=0; i < 256; i++)
    {
      int indx;
      
      if (Used[i] != 0)
        {
          indx = Used[i] - 1;
          img_data->reds[indx] = colrs[i].red;
          img_data->greens[indx] = colrs[i].green;
          img_data->blues[indx] = colrs[i].blue;
        }
    }
  ptr = data;
  for (i=0; i < (width * height); i++)
    {
      *ptr = (unsigned char)(Used[(int)*ptr] - 1);
      ptr++;
    }
  img_data->image_data = data;
  img_data->image = NULL;

  mo_cache_image_info (src, (void *)img_data);
  
  return(img_data);
}

mo_status mo_register_image_resolution_function (mo_window *win)
{
  XmxSetArg (WbNresolveImageFunction, (long)Resolve);
  XmxSetValues (win->view);
}
