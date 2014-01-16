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

#include <stdio.h>
#include <ctype.h>
#include "HTMLP.h"
#include "NoImage.xbm"


ImageInfo no_image;

static int allocation_index[256];


/*
 * Free all the colors in the default colormap that we have allocated so far.
 */
void
FreeColors(dsp, colormap)
	Display *dsp;
	Colormap colormap;
{
	int i, j;
	unsigned long pix;

	for (i=0; i<256; i++)
	{
		if (allocation_index[i])
		{
			pix = (unsigned long)i;
			/*
			 * Because X is stupid, we have to Free the color
			 * once for each time we've allocated it.
			 */
			for (j=0; j<allocation_index[i]; j++)
			{
				XFreeColors(dsp, colormap, &pix, 1, 0L);
			}
		}
		allocation_index[i] = 0;
	}
}


/*
 * Free up all the pixmaps allocated for this document.
 */
void
FreeImages(hw)
	HTMLWidget hw;
{
	int page;
	struct ele_rec *eptr;

	for (page = 1; page <= hw->html.page_cnt; page++)
	{
		eptr = hw->html.pages[(page - 1)].elist;

		while (eptr != NULL)
		{
			if ((eptr->type == E_IMAGE)&&(eptr->pic_data != NULL))
			{
				/*
				 * Don't free the no_image default image
				 */
				if ((eptr->pic_data->image != (Pixmap)NULL)&&
				    (eptr->pic_data->image != no_image.image))
				{
					XFreePixmap(XtDisplay(hw),
						eptr->pic_data->image);
					eptr->pic_data->image = (Pixmap)NULL;
				}
			}
			eptr = eptr->next;
		}
	}
}


/*
 * Find the closest color by allocating it, or picking an already allocated
 * color
 */
void
FindColor(dsp, colormap, colr)
	Display *dsp;
	Colormap colormap;
	XColor *colr;
{
	int i, match;
#ifdef MORE_ACCURATE
	double rd, gd, bd, dist, mindist;
#else
	int rd, gd, bd, dist, mindist;
#endif /* MORE_ACCURATE */
	int cindx;
static XColor def_colrs[256];
static int have_colors = 0;
	int NumCells;

	match = XAllocColor(dsp, colormap, colr);
	if (match == 0)
	{
		NumCells = DisplayCells(dsp, DefaultScreen(dsp));
		if (!have_colors)
		{
			for (i=0; i<NumCells; i++)
			{
				def_colrs[i].pixel = i;
			}
			XQueryColors(dsp, colormap, def_colrs, NumCells);
			have_colors = 1;
		}
#ifdef MORE_ACCURATE
		mindist = 196608.0;		/* 256.0 * 256.0 * 3.0 */
		cindx = colr->pixel;
		for (i=0; i<NumCells; i++)
		{
			rd = (def_colrs[i].red - colr->red) / 256.0;
			gd = (def_colrs[i].green - colr->green) / 256.0;
			bd = (def_colrs[i].blue - colr->blue) / 256.0;
			dist = (rd * rd) +
				(gd * gd) +
				(bd * bd);
			if (dist < mindist)
			{
				mindist = dist;
				cindx = def_colrs[i].pixel;
				if (dist == 0.0)
				{
					break;
				}
			}
		}
#else
		mindist = 196608;		/* 256 * 256 * 3 */
		cindx = colr->pixel;
		for (i=0; i<NumCells; i++)
		{
			rd = ((int)(def_colrs[i].red >> 8) -
				(int)(colr->red >> 8));
			gd = ((int)(def_colrs[i].green >> 8) -
				(int)(colr->green >> 8));
			bd = ((int)(def_colrs[i].blue >> 8) -
				(int)(colr->blue >> 8));
			dist = (rd * rd) +
				(gd * gd) +
				(bd * bd);
			if (dist < mindist)
			{
				mindist = dist;
				cindx = def_colrs[i].pixel;
				if (dist == 0)
				{
					break;
				}
			}
		}
#endif /* MORE_ACCURATE */
		colr->pixel = cindx;
		colr->red = def_colrs[cindx].red;
		colr->green = def_colrs[cindx].green;
		colr->blue = def_colrs[cindx].blue;
	}
	else
	{
		/*
		 * Keep a count of how many times we have allocated the
		 * same color, so we can properly free them later.
		 */
		allocation_index[colr->pixel]++;

		/*
		 * If this is a new color, we've actually changed the default
		 * colormap, and may have to re-query it later.
		 */
		if (allocation_index[colr->pixel] == 1)
		{
			have_colors = 0;
		}
	}
}


/*
 * Make am image of appropriate depth for display from image data.
 */
XImage *
MakeImage(dsp, data, width, height, depth, img_info)
	Display *dsp;
	unsigned char *data;
	int width, height;
	int depth;
	ImageInfo *img_info;
{
	int linepad, shiftnum;
	int shiftstart, shiftstop, shiftinc;
	int bytesperline;
	int temp;
	int w, h;
	XImage *newimage;
	unsigned char *bit_data, *bitp, *datap;

	switch(depth)
	{
	    case 8:
		bit_data = (unsigned char *)malloc(width * height);
		bcopy(data, bit_data, (width * height));
		bytesperline = width;
		newimage = XCreateImage(dsp,
			DefaultVisual(dsp, DefaultScreen(dsp)),
			depth, ZPixmap, 0, (char *)bit_data,
			width, height, 8, bytesperline);
		break;
	    case 1:
	    case 2:
	    case 4:
		if (BitmapBitOrder(dsp) == LSBFirst)
		{
			shiftstart = 0;
			shiftstop = 8;
			shiftinc = depth;
		}
		else
		{
			shiftstart = 8 - depth;
			shiftstop = -depth;
			shiftinc = -depth;
		}
		linepad = 8 - (width % 8);
		bit_data = (unsigned char *)malloc(((width + linepad) * height)
				+ 1);
		bitp = bit_data;
		datap = data;
		*bitp = 0;
		shiftnum = shiftstart;
		for (h=0; h<height; h++)
		{
			for (w=0; w<width; w++)
			{
				temp = *datap++ << shiftnum;
				*bitp = *bitp | temp;
				shiftnum = shiftnum + shiftinc;
				if (shiftnum == shiftstop)
				{
					shiftnum = shiftstart;
					bitp++;
					*bitp = 0;
				}
			}
			for (w=0; w<linepad; w++)
			{
				shiftnum = shiftnum + shiftinc;
				if (shiftnum == shiftstop)
				{
					shiftnum = shiftstart;
					bitp++;
					*bitp = 0;
				}
			}
		}
		bytesperline = (width + linepad) * depth / 8;
		newimage = XCreateImage(dsp,
			DefaultVisual(dsp, DefaultScreen(dsp)),
			depth, ZPixmap, 0, (char *)bit_data,
			(width + linepad), height, 8, bytesperline);
		break;
	    case 24:
		bit_data = (unsigned char *)malloc(width * height * 4);

		if (BitmapBitOrder(dsp) == MSBFirst)
		{
			bitp = bit_data;
			datap = data;
			for (w = (width * height); w > 0; w--)
			{
				*bitp++ = (unsigned char)0;
				*bitp++ = (unsigned char)
					((img_info->blues[(int)*datap]>>8) &
					0xff);
				*bitp++ = (unsigned char)
					((img_info->greens[(int)*datap]>>8) &
					0xff);
				*bitp++ = (unsigned char)
					((img_info->reds[(int)*datap]>>8) &
					0xff);
				datap++;
			}
		}
		else
		{
			bitp = bit_data;
			datap = data;
			for (w = (width * height); w > 0; w--)
			{
				*bitp++ = (unsigned char)
					((img_info->reds[(int)*datap]>>8) &
					0xff);
				*bitp++ = (unsigned char)
					((img_info->greens[(int)*datap]>>8) &
					0xff);
				*bitp++ = (unsigned char)
					((img_info->blues[(int)*datap]>>8) &
					0xff);
				*bitp++ = (unsigned char)0;
				datap++;
			}
		}

		newimage = XCreateImage(dsp,
			DefaultVisual(dsp, DefaultScreen(dsp)),
			depth, ZPixmap, 0, (char *)bit_data,
			width, height, 32, 0);
		break;
	    default:
		fprintf(stderr, "Don't know how to format image for display of depth %d\n", depth);
		return(NULL);
	}

	return(newimage);
}




Pixmap
NoImage(hw)
	HTMLWidget hw;
{
	if (no_image.image == (Pixmap)NULL)
	{
		no_image.image = XCreatePixmapFromBitmapData(XtDisplay(hw),
			XtWindow(hw), NoImage_bits,
			NoImage_width, NoImage_height,
/*
 * Without motif we use our own foreground resource instead of
 * using the manager's
 */
#ifdef MOTIF
                        hw->manager.foreground,
#else
                        hw->html.foreground,
#endif /* MOTIF */
			hw->core.background_pixel,
			DefaultDepthOfScreen(XtScreen(hw)));
	}
	return(no_image.image);
}


ImageInfo *
NoImageData(hw)
	HTMLWidget hw;
{
	no_image.width = NoImage_width;
	no_image.height = NoImage_height;
	no_image.num_colors = 0;
	no_image.reds = NULL;
	no_image.greens = NULL;
	no_image.blues = NULL;
	no_image.image_data = NULL;
	no_image.image = (Pixmap)NULL;

	return(&no_image);
}


Pixmap
InfoToImage(hw, img_info)
	HTMLWidget hw;
	ImageInfo *img_info;
{
	int i, size;
	Pixmap Img;
	XImage *tmpimage;
	XColor tmpcolr;
	int *Mapping;
	unsigned char *tmpdata;
	unsigned char *ptr;
	unsigned char *ptr2;
	int Vclass;
	XVisualInfo vinfo, *vptr;

	/* find the visual class. */
	vinfo.visualid = XVisualIDFromVisual(DefaultVisual(XtDisplay(hw),
		DefaultScreen(XtDisplay(hw))));
	vptr = XGetVisualInfo(XtDisplay(hw), VisualIDMask, &vinfo, &i);
	Vclass = vptr->class;
	XFree((char *)vptr);

	Mapping = (int *)malloc(img_info->num_colors * sizeof(int));

	for (i=0; i < img_info->num_colors; i++)
	{
		tmpcolr.red = img_info->reds[i];
		tmpcolr.green = img_info->greens[i];
		tmpcolr.blue = img_info->blues[i];
		tmpcolr.flags = DoRed|DoGreen|DoBlue;
		if (Vclass == TrueColor)
		{
			Mapping[i] = i;
		}
		else
		{
			FindColor(XtDisplay(hw),
				DefaultColormapOfScreen(XtScreen(hw)),
				&tmpcolr);
			Mapping[i] = tmpcolr.pixel;
		}
	}

	size = img_info->width * img_info->height;
	tmpdata = (unsigned char *)malloc(size);
	if (tmpdata == NULL)
	{
		tmpimage = NULL;
		Img = (Pixmap)NULL;
	}
	else
	{
		ptr = img_info->image_data;
		ptr2 = tmpdata;
		for (i=0; i < size; i++)
		{
			*ptr2++ = (unsigned char)Mapping[(int)*ptr];
			ptr++;
		}

		tmpimage = MakeImage(XtDisplay(hw), tmpdata,
			img_info->width, img_info->height,
			DefaultDepthOfScreen(XtScreen(hw)), img_info);

		Img = XCreatePixmap(XtDisplay(hw), XtWindow(hw),
			img_info->width, img_info->height,
			DefaultDepthOfScreen(XtScreen(hw)));
	}

	if ((tmpimage == NULL)||(Img == (Pixmap)NULL))
	{
		if (tmpimage != NULL)
		{
			XDestroyImage(tmpimage);
		}
		if (Img != (Pixmap)NULL)
		{
			XFreePixmap(XtDisplay(hw), Img);
		}
		img_info->width = NoImage_width;
		img_info->height = NoImage_height;
		Img = NoImage(hw);
	}
	else
	{
		XPutImage(XtDisplay(hw), Img, hw->html.drawGC, tmpimage, 0, 0,
			0, 0, img_info->width, img_info->height);
		XDestroyImage(tmpimage);
	}
	return(Img);
}

