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
#include "HTMLP.h"


#define	PAGE_HEIGHT_MIN		100
#define	PAGE_HEIGHT_MAX		30000
#define	MARGIN_DEFAULT		20
#define	CLICK_TIME		500
#define	SELECT_THRESHOLD	3
#define	MAX_UNDERLINES		3

#ifndef ABS
#define ABS(x)  (((x) > 0) ? (x) : ((x) * -1))
#endif


extern int FormatAll();
extern int DocumentWidth();
extern void PlaceLine();
extern void TextRefresh();
extern void ImageRefresh();
extern void LinefeedRefresh();
extern void RefreshTextRange();
extern void FreeColors();
extern void FreeImages();
extern int SwapElements();
extern int ElementLessThan();
extern char *ParseTextToString();
extern char *ParseTextToPrettyString();
extern char *ParseTextToPSString();
extern struct mark_up *HTMLParse();
extern struct ele_rec *LocateElement();
extern struct ele_rec **MakeLineList();
extern void FreeHRefs();
extern struct ref_rec *AddHRef();
extern struct ref_rec *FindHRef();


static void		SelectStart();
static void		ExtendStart();
static void		ExtendAdjust();
static void		ExtendEnd();
static Boolean		ConvertSelection();
static void		LoseSelection();
static void		SelectionDone();

#ifdef _NO_PROTO

static void		_HTMLInput() ;
static void             Initialize() ;
static void             Redisplay() ;
static void             Resize() ;
static Boolean          SetValues() ;
static void		RecolorInternalHRefs() ;

#else /* _NO_PROTO */

static void		_HTMLInput(HTMLWidget hw, XEvent *event,
				String *params, Cardinal *num_params);
static void             Initialize(HTMLWidget request, HTMLWidget new);
static void             Redisplay(HTMLWidget hw, XEvent *event, Region region);
static void             Resize(HTMLWidget hw);
static Boolean          SetValues(HTMLWidget current, HTMLWidget request,
				HTMLWidget new);
static void		RecolorInternalHRefs(HTMLWidget hw, char *href);
#endif /* _NO_PROTO */


/*
 * Default translations
 * Selection of text, and activate anchors.
 * If motif, add manager translations.
 */
#ifdef MOTIF
static char defaultTranslations[] =
" \
<Btn1Down>:	select-start() ManagerGadgetArm()\n\
<Btn1Motion>:	extend-adjust() ManagerGadgetButtonMotion()\n\
<Btn1Up>:	extend-end(PRIMARY, CUT_BUFFER0) ManagerGadgetActivate()\n\
<Btn2Down>:	select-start()\n\
<Btn2Motion>:	extend-adjust()\n\
<Btn2Up>:	extend-end(PRIMARY, CUT_BUFFER0)\n\
<Btn3Down>:	extend-start()\n\
<Btn3Motion>:	extend-adjust()\n\
<Btn3Up>:	extend-end(PRIMARY, CUT_BUFFER0) \
";
#else
static char defaultTranslations[] =
" \
<Btn1Down>:	select-start() \n\
<Btn1Motion>:	extend-adjust() \n\
<Btn1Up>:	extend-end(PRIMARY, CUT_BUFFER0) \n\
<Btn2Down>:	select-start() \n\
<Btn2Motion>:	extend-adjust() \n\
<Btn2Up>:	extend-end(PRIMARY, CUT_BUFFER0) \n\
<Btn3Down>:	extend-start()\n\
<Btn3Motion>:	extend-adjust()\n\
<Btn3Up>:	extend-end(PRIMARY, CUT_BUFFER0) \
";
#endif /* MOTIF */


static XtActionsRec actionsList[] =
{
   { "select-start",    (XtActionProc) SelectStart },
   { "extend-start",    (XtActionProc) ExtendStart },
   { "extend-adjust",   (XtActionProc) ExtendAdjust },
   { "extend-end",      (XtActionProc) ExtendEnd },
   { "HTMLInput",	(XtActionProc) _HTMLInput },

#ifdef MOTIF
   { "Arm",      (XtActionProc) _XmGadgetArm },         /* Motif 1.0 */
   { "Activate", (XtActionProc) _XmGadgetActivate },    /* Motif 1.0 */
   { "Enter",    (XtActionProc) _XmManagerEnter },      /* Motif 1.0 */
   { "FocusIn",  (XtActionProc) _XmManagerFocusIn },    /* Motif 1.0 */
   { "Help",     (XtActionProc) _XmManagerHelp },       /* Motif 1.0 */
#endif /* MOTIF */
};



/*
 *  Resource definitions for HTML widget
 */

static XtResource resources[] =
{
  /* Without Motif we need to override the borderWidth to 0 (from 1). */
#ifndef MOTIF
        {       XtNborderWidth,
                XtCBorderWidth, XtRDimension, sizeof (Dimension),
                XtOffset (HTMLWidget, core.border_width),
                XtRImmediate, (XtPointer) 0
        },
#endif

	{	WbNmarginWidth,
		WbCMarginWidth, XtRDimension, sizeof (Dimension),
		XtOffset (HTMLWidget, html.margin_width),
		XtRImmediate, (caddr_t) MARGIN_DEFAULT
	},

	{	WbNmarginHeight,
		WbCMarginHeight, XtRDimension, sizeof (Dimension),
		XtOffset (HTMLWidget, html.margin_height),
		XtRImmediate, (caddr_t) MARGIN_DEFAULT
	},

	{	WbNpageHeight,
		WbCPageHeight, XtRDimension, sizeof (Dimension),
		XtOffset (HTMLWidget, html.page_height),
		XtRImmediate, (caddr_t) PAGE_HEIGHT_MAX
	},

	{	WbNcurrentPage,
		WbCCurrentPage, XtRInt, sizeof (int),
		XtOffset (HTMLWidget, html.current_page),
		XtRString, "1"
	},

	{	WbNanchorCallback,
		XtCCallback, XtRCallback, sizeof (XtCallbackList),
		XtOffset (HTMLWidget, html.anchor_callback),
		XtRImmediate, (caddr_t) NULL
	},

	{	WbNdocumentPageCallback,
		XtCCallback, XtRCallback, sizeof (XtCallbackList),
		XtOffset (HTMLWidget, html.document_page_callback),
		XtRImmediate, (caddr_t) NULL
	},

	{	WbNtext,
		WbCText, XtRString, sizeof (char *),
		XtOffset (HTMLWidget, html.raw_text),
		XtRString, (char *) NULL
	},

	{	WbNheaderText,
		WbCHeaderText, XtRString, sizeof (char *),
		XtOffset (HTMLWidget, html.header_text),
		XtRString, (char *) NULL
	},

	{	WbNfooterText,
		WbCFooterText, XtRString, sizeof (char *),
		XtOffset (HTMLWidget, html.footer_text),
		XtRString, (char *) NULL
	},

	{	WbNtitleText,
		WbCTitleText, XtRString, sizeof (char *),
		XtOffset (HTMLWidget, html.title),
		XtRString, (char *) NULL
	},

/*
 * Without motif we need our own foreground resource instead of
 * using the manager's
 */
#ifndef MOTIF
	{	XtNforeground,
		XtCForeground, XtRPixel, sizeof (Pixel),
		XtOffset (HTMLWidget, html.foreground),
		XtRString, "Black"
	},
#endif

	{	WbNanchorUnderlines,
		WbCAnchorUnderlines, XtRInt, sizeof (int),
		XtOffset (HTMLWidget, html.num_anchor_underlines),
		XtRString, "0"
	},

	{	WbNvisitedAnchorUnderlines,
		WbCVisitedAnchorUnderlines, XtRInt, sizeof (int),
		XtOffset (HTMLWidget, html.num_visitedAnchor_underlines),
		XtRString, "0"
	},

	{	WbNdashedAnchorUnderlines,
		WbCDashedAnchorUnderlines, XtRBoolean, sizeof (Boolean),
		XtOffset (HTMLWidget, html.dashed_anchor_lines),
		XtRString, "False"
	},

	{	WbNdashedVisitedAnchorUnderlines,
		WbCDashedVisitedAnchorUnderlines, XtRBoolean, sizeof (Boolean),
		XtOffset (HTMLWidget, html.dashed_visitedAnchor_lines),
		XtRString, "False"
	},

	{	WbNanchorColor,
		XtCForeground, XtRPixel, sizeof (Pixel),
		XtOffset (HTMLWidget, html.anchor_fg),
		XtRString, "blue2"
	},

	{	WbNvisitedAnchorColor,
		XtCForeground, XtRPixel, sizeof (Pixel),
		XtOffset (HTMLWidget, html.visitedAnchor_fg),
		XtRString, "purple4"
	},

	{	WbNactiveAnchorFG,
		XtCBackground, XtRPixel, sizeof (Pixel),
		XtOffset (HTMLWidget, html.activeAnchor_fg),
		XtRString, "Red"
	},

	{	WbNactiveAnchorBG,
		XtCForeground, XtRPixel, sizeof (Pixel),
		XtOffset (HTMLWidget, html.activeAnchor_bg),
		XtRString, "White"
	},

	{	WbNpercentVerticalSpace,
		WbCPercentVerticalSpace, XtRInt, sizeof (int),
		XtOffset (HTMLWidget, html.percent_vert_space),
		XtRString, "90"
	},

	{	WbNimageBorders,
		WbCImageBorders, XtRBoolean, sizeof (Boolean),
		XtOffset (HTMLWidget, html.border_images),
		XtRString, "False"
	},

	{	WbNfancySelections,
		WbCFancySelections, XtRBoolean, sizeof (Boolean),
		XtOffset (HTMLWidget, html.fancy_selections),
		XtRString, "False"
	},

	{	WbNautoSize,
		WbCAutoSize, XtRBoolean, sizeof (Boolean),
		XtOffset (HTMLWidget, html.auto_size),
		XtRString, "False"
	},

	{	WbNisIndex,
		WbCIsIndex, XtRBoolean, sizeof (Boolean),
		XtOffset (HTMLWidget, html.is_index),
		XtRString, "False"
	},

	{	XtNfont,
		XtCFont, XtRFontStruct, sizeof (XFontStruct *),
		XtOffset (HTMLWidget, html.font),
		XtRString, "-adobe-times-medium-r-normal-*-14-*-*-*-*-*-*-*"
	},

	{	WbNitalicFont,
		WbCItalicFont, XtRFontStruct, sizeof (XFontStruct *),
		XtOffset (HTMLWidget, html.italic_font),
		XtRString, "-adobe-times-medium-i-normal-*-14-*-*-*-*-*-*-*"
	},

	{	WbNboldFont,
		WbCBoldFont, XtRFontStruct, sizeof (XFontStruct *),
		XtOffset (HTMLWidget, html.bold_font),
		XtRString, "-adobe-times-bold-r-normal-*-14-*-*-*-*-*-*-*"
	},

	{	WbNfixedFont,
		WbCFixedFont, XtRFontStruct, sizeof (XFontStruct *),
		XtOffset (HTMLWidget, html.fixed_font),
		XtRString, "-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*"
	},

	{	WbNheader1Font,
		WbCHeader1Font, XtRFontStruct, sizeof (XFontStruct *),
		XtOffset (HTMLWidget, html.header1_font),
		XtRString, "-adobe-times-bold-r-normal-*-24-*-*-*-*-*-*-*"
	},

	{	WbNheader2Font,
		WbCHeader2Font, XtRFontStruct, sizeof (XFontStruct *),
		XtOffset (HTMLWidget, html.header2_font),
		XtRString, "-adobe-times-bold-r-normal-*-18-*-*-*-*-*-*-*"
	},

	{	WbNheader3Font,
		WbCHeader3Font, XtRFontStruct, sizeof (XFontStruct *),
		XtOffset (HTMLWidget, html.header3_font),
		XtRString, "-adobe-times-bold-r-normal-*-17-*-*-*-*-*-*-*"
	},

	{	WbNheader4Font,
		WbCHeader4Font, XtRFontStruct, sizeof (XFontStruct *),
		XtOffset (HTMLWidget, html.header4_font),
		XtRString, "-adobe-times-bold-r-normal-*-14-*-*-*-*-*-*-*"
	},

	{	WbNheader5Font,
		WbCHeader5Font, XtRFontStruct, sizeof (XFontStruct *),
		XtOffset (HTMLWidget, html.header5_font),
		XtRString, "-adobe-times-bold-r-normal-*-12-*-*-*-*-*-*-*"
	},

	{	WbNheader6Font,
		WbCHeader6Font, XtRFontStruct, sizeof (XFontStruct *),
		XtOffset (HTMLWidget, html.header6_font),
		XtRString, "-adobe-times-bold-r-normal-*-10-*-*-*-*-*-*-*"
	},

	{	WbNaddressFont,
		WbCAddressFont, XtRFontStruct, sizeof (XFontStruct *),
		XtOffset (HTMLWidget, html.address_font),
		XtRString, "-adobe-times-medium-i-normal-*-14-*-*-*-*-*-*-*"
	},

	{	WbNplainFont,
		WbCPlainFont, XtRFontStruct, sizeof (XFontStruct *),
		XtOffset (HTMLWidget, html.plain_font),
		XtRString, "-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*"
	},

	{	WbNlistingFont,
		WbCListingFont, XtRFontStruct, sizeof (XFontStruct *),
		XtOffset (HTMLWidget, html.listing_font),
		XtRString, "-adobe-courier-medium-r-normal-*-12-*-*-*-*-*-*-*"
	},
                  
        {       WbNpreviouslyVisitedTestFunction,
                WbCPreviouslyVisitedTestFunction, XtRPointer, 
                sizeof (XtPointer),
                XtOffset (HTMLWidget, html.previously_visited_test),
                XtRImmediate, (caddr_t) NULL
        },
                  
        {       WbNresolveImageFunction,
                WbCResolveImageFunction, XtRPointer, 
                sizeof (XtPointer),
                XtOffset (HTMLWidget, html.resolveImage),
                XtRImmediate, (caddr_t) NULL
        },
                    
};



HTMLClassRec htmlClassRec = {
   {						/* core class fields  */
#ifdef MOTIF
      (WidgetClass) &xmManagerClassRec,		/* superclass         */
#else
      (WidgetClass) &constraintClassRec,	/* superclass         */
#endif /* MOTIF */
      "HTML",					/* class_name         */
      sizeof(HTMLRec),				/* widget_size        */
      NULL,					/* class_initialize   */
      NULL,					/* class_part_init    */
      FALSE,					/* class_inited       */
      (XtInitProc) Initialize,			/* initialize         */
      NULL,					/* initialize_hook    */
      XtInheritRealize,				/* realize            */
      actionsList,				/* actions	      */
      XtNumber(actionsList),			/* num_actions	      */
      resources,				/* resources          */
      XtNumber(resources),			/* num_resources      */
      NULLQUARK,				/* xrm_class          */
      TRUE,					/* compress_motion    */
      FALSE,					/* compress_exposure  */
      TRUE,					/* compress_enterlv   */
      FALSE,					/* visible_interest   */
      NULL,			                /* destroy            */
      (XtWidgetProc) Resize,			/* resize             */
      (XtExposeProc) Redisplay,			/* expose             */
      (XtSetValuesFunc) SetValues,		/* set_values         */
      NULL,					/* set_values_hook    */
      XtInheritSetValuesAlmost,			/* set_values_almost  */
      NULL,					/* get_values_hook    */
      NULL,					/* accept_focus       */
      XtVersion,				/* version            */
      NULL,					/* callback_private   */
      defaultTranslations,			/* tm_table           */
      XtInheritQueryGeometry,			/* query_geometry     */
      XtInheritDisplayAccelerator,              /* display_accelerator*/
      NULL,		                        /* extension          */
   },

   {		/* composite_class fields */
      NULL,				    	/* geometry_manager   */
      NULL,					/* change_managed     */
      XtInheritInsertChild,			/* insert_child       */
      XtInheritDeleteChild,			/* delete_child       */
      NULL,                                     /* extension          */
   },

   {		/* constraint_class fields */
      NULL,					/* resource list        */   
      0,					/* num resources        */   
      0,					/* constraint size      */   
      NULL,					/* init proc            */   
      NULL,					/* destroy proc         */   
      NULL,					/* set values proc      */   
      NULL,                                     /* extension            */
   },

#ifdef MOTIF
   {		/* manager_class fields */
      XtInheritTranslations,			/* translations           */
      NULL,					/* syn_resources      	  */
      0,					/* num_syn_resources 	  */
      NULL,					/* syn_cont_resources     */
      0,					/* num_syn_cont_resources */
      XmInheritParentProcess,                   /* parent_process         */
      NULL,					/* extension 	          */    
   },
#endif /* MOTIF */

   {		/* html_class fields */     
      0						/* none			  */
   }	
};

WidgetClass htmlWidgetClass = (WidgetClass)&htmlClassRec;



/*
 * Initialize is called when the widget is first initialized.
 * Check to see that all the starting resources are valid.
 */
static void
#ifdef _NO_PROTO
Initialize (request, new)
            HTMLWidget request ;
            HTMLWidget new ;
#else
Initialize(
            HTMLWidget request,
            HTMLWidget new)
#endif
{
	/*
	 *	Make sure height and width are not zero.
	 */
	if (new->core.width == 0)
	{
		new->core.width = new->html.margin_width << 1 ;
	} 
	if (new->core.width == 0)
	{
		new->core.width = 10 ;
	} 
	if (new->core.height == 0)
	{
		new->core.height = new->html.margin_height << 1 ;
	} 
	if (new->core.height == 0)
	{
		new->core.height = 10 ;
	} 

	/*
	 *	Make sure the underline numbers are within bounds.
	 */
	if (new->html.num_anchor_underlines < 0)
	{
		new->html.num_anchor_underlines = 0;
	} 
	if (new->html.num_anchor_underlines > MAX_UNDERLINES)
	{
		new->html.num_anchor_underlines = MAX_UNDERLINES;
	} 
	if (new->html.num_visitedAnchor_underlines < 0)
	{
		new->html.num_visitedAnchor_underlines = 0;
	} 
	if (new->html.num_visitedAnchor_underlines > MAX_UNDERLINES)
	{
		new->html.num_visitedAnchor_underlines = MAX_UNDERLINES;
	} 

	/*
	 * Make sure page_height is within bounds.
	 */
	if (new->html.page_height > PAGE_HEIGHT_MAX)
	{
		fprintf(stderr, "Warning: pageHeight resource cannot exceed %d\n", PAGE_HEIGHT_MAX);
		new->html.page_height = PAGE_HEIGHT_MAX;
	}
	else if (new->html.page_height < PAGE_HEIGHT_MIN)
	{
		fprintf(stderr, "Warning: pageHeight resource must exceed %d\n", PAGE_HEIGHT_MIN);
		new->html.page_height = PAGE_HEIGHT_MIN;
	}

	/*
	 * Parse the raw text with the HTML parser.  And set the formatted 
	 * element list to NULL.
	 */
	new->html.html_objects = HTMLParse(NULL, request->html.raw_text);
	new->html.html_header_objects =
		HTMLParse(NULL, request->html.header_text);
	new->html.html_footer_objects =
		HTMLParse(NULL, request->html.footer_text);
	new->html.formatted_elements = NULL;
	new->html.my_visited_hrefs = NULL;

	/*
	 * if we are autosizing, make sure the width is enough for
	 * the longest line of unformatted text, and the height is
	 * enough for the whole formatted document.
	 */
	if (new->html.auto_size == True)
	{
		int temp;

		temp = DocumentWidth(new, new->html.html_objects);
		new->html.max_pre_width = temp;
		temp = temp + (2 * new->html.margin_width);
		if (new->core.width < temp)
		{
			new->core.width = temp;
		}

		new->html.line_array = NULL;
		new->html.line_count = 0;
		new->html.page_cnt = 0;
		new->html.pages = NULL;
		temp = FormatAll(new, new->core.width);
		new->core.height = temp;
	}
	else
	{
		new->html.line_array = NULL;
		new->html.line_count = 0;
		new->html.page_cnt = 0;
		new->html.pages = NULL;
		new->html.max_pre_width = 0;
	}

	/*
	 * Initialize private widget resources
	 */
	new->html.format_width = 0;
	new->html.format_height = 0;
	new->html.reformat = True;
	new->html.drawGC = NULL;
	new->html.select_start = NULL;
	new->html.select_end = NULL;
	new->html.sel_start_pos = 0;
	new->html.sel_end_pos = 0;
	new->html.new_start = NULL;
	new->html.new_end = NULL;
	new->html.new_start_pos = 0;
	new->html.new_end_pos = 0;
	new->html.active_anchor = NULL;

        return;
}


#ifdef DEBUG
void
DebugHook(x, y, width, height)
	int x, y, width, height;
{
fprintf(stderr, "Redrawing (%d,%d) %dx%d\n", x, y, width, height);
}
#endif


/*
 * The Redisplay function is what you do with an expose event.
 * Right now we call user callbacks, and then call the CompositeWidget's
 * Redisplay routine.
 */
static void
#ifdef _NO_PROTO
Redisplay (hw, event, region)
            HTMLWidget hw;
            XEvent * event;
            Region region;
#else
Redisplay(
            HTMLWidget hw,
            XEvent * event,
            Region region)
#endif
{
	XExposeEvent *ExEvent = (XExposeEvent *)event;
	XExposeEvent report;
	int i, start, end;
	int x, y;
	int width, height;

	/*
	 * Make sure we have a valid drawGC.  Only do this if we have a real
	 * event, since it is possible to have this be called internally
	 * before we have a window.
	 */
	if ((event != NULL)&&(hw->html.drawGC == NULL))
	{
		unsigned long valuemask;
		XGCValues values;

		values.function = GXcopy;
		values.plane_mask = AllPlanes;
/*
 * Without motif we use our own foreground resource instead of
 * using the manager's
 */
#ifdef MOTIF
		values.foreground = hw->manager.foreground;
#else
		values.foreground = hw->html.foreground;
#endif /* MOTIF */
		values.background = hw->core.background_pixel;

		valuemask = GCFunction|GCPlaneMask|GCForeground|GCBackground;

		hw->html.drawGC = XCreateGC(XtDisplay(hw), XtWindow(hw),
			valuemask, &values);
	}

	/*
	 * If we need to reformat, clear the entire window, reformat the
	 * text to the core width, and redraw the window.
	 * Otherwise, just find the lines that overlap the exposed
	 * area, and redraw those lines.
	 */
	if (hw->html.reformat == True)
	{
		int nwidth;

		/*
		 * On auto-resize we want to make sure we are as wide
		 * as the widest text.
		 */
		if (hw->html.auto_size == True)
		{
			nwidth = hw->html.max_pre_width;
			nwidth = nwidth + (2 * hw->html.margin_width);
			if (hw->core.width > nwidth)
			{
				nwidth = hw->core.width;
			}
		}
		else
		{
			nwidth = hw->core.width;
		}
		if ((hw->html.format_width != nwidth)||
			(hw->html.format_height != hw->core.height))
		{
			height = FormatAll(hw, nwidth);
			hw->html.format_width = nwidth;
			hw->html.format_height = height;
		}
		else
		{
			height = hw->html.format_height;
		}

		if ((hw->html.auto_size == True)&&((height != hw->core.height)||
			(nwidth != hw->core.width)))
		{
			XtResizeWidget((Widget)hw, nwidth, height,
					hw->core.border_width);
			/*
			 * leave now since the XtResizeWidget we just
			 * did is going to send us back later
			 */
			return;
		}

		hw->html.reformat = False;
		/*
		 * Only if we have a window
		 */
		if (hw->html.drawGC != NULL)
		{
			XClearArea(XtDisplay(hw), XtWindow(hw), 0, 0, 0, 0,
				False);
		}

		x = 0;
		y = 0;
		width = hw->core.width;
		height = hw->core.height;
#ifdef LIMIT_REDRAWS
		/*
		 * Kind of a hack, but our entire height can be huge,
		 * and since we are probably in some kind of scrolled window
		 * we don't need to redraw any bigger than our parent.
		 */
		{
			Widget parent;

			parent = XtParent(hw);
			if (parent != NULL)
			{
				width = parent->core.width;
				height = parent->core.height;
			}
			/*
			 * Final hack.  It takes a long time to redraw, so
			 * since we don't believe we will have windows
			 * taller than 2000 pixels anytime soon, cap height
			 * at 2000.
			 */
			if (height > 2000)
			{
				height = 2000;
			}
		}
#endif /* LIMIT_REDRAWS */
	}
	else
	{
		/*
		 * if Redisplay is called internally, it means redisplay
		 * the whole screen.
		 */
		if (event == NULL)
		{
			/*
			 * Only if we have a window
			 */
			if (hw->html.drawGC != NULL)
			{
				XClearArea(XtDisplay(hw), XtWindow(hw),
					0, 0, 0, 0, False);
			}
			x = 0;
			y = 0;
			width = hw->core.width;
			height = hw->core.height;

#ifdef LIMIT_REDRAWS
			/*
			 * Kind of a hack, but our entire height can be huge,
			 * and since we are probably in some kind of
			 * scrolled window we don't need to redraw any
			 * bigger than our parent.
			 */
			{
				Widget parent;

				parent = XtParent(hw);
				if (parent != NULL)
				{
					width = parent->core.width;
					height = parent->core.height;
				}
				/*
				 * Final hack.  It takes a long time to
				 * redraw, so since we don't believe we will
				 * have windows taller than 2000 pixels
				 * anytime soon, cap height at 2000.
				 */
				if (height > 2000)
				{
					height = 2000;
				}
			}
#endif /* LIMIT_REDRAWS */
		}
		else
		{
			x = ExEvent->x;
			y = ExEvent->y;
			width = (int)ExEvent->width;
			height = (int)ExEvent->height;
		}
	}
#ifdef DEBUG
DebugHook(x, y, width, height);
#endif

	/*
	 * Find the lines that overlap the exposed area.
	 */
	start = 0;
	end = hw->html.line_count - 1;
	for (i=0; i<hw->html.line_count; i++)
	{
		if (hw->html.line_array[i] == NULL)
		{
			continue;
		}

		if (hw->html.line_array[i]->y < y)
		{
			start = i;
		}
		if (hw->html.line_array[i]->y > (y + height))
		{
			end = i;
			break;
		}
	}

#ifdef COMPRESS
	/*
	 * Compression of expose events.  If there are any other expose
	 * events in the queue that overlap our current one, fetch them
	 * out, and redraw everything at once.
	 */
	XFlush(XtDisplay(hw));
	XSync(XtDisplay(hw), False);
	while (XCheckTypedWindowEvent(XtDisplay(hw), XtWindow(hw),
			ExposureMask, &report))
	{
		int new_start, new_end;
		int y;
		unsigned int height;

		y = report.y;
		height = report.height;

		/*
		 * Find the lines that overlap the exposed area.
		 */
		new_start = 0;
		new_end = hw->html.line_count - 1;
		for (i=0; i<hw->html.line_count; i++)
		{
			if (hw->html.line_array[i] == NULL)
			{
				continue;
			}

			if (hw->html.line_array[i]->y < y)
			{
				new_start = i;
			}
			if (hw->html.line_array[i]->y > (y + height))
			{
				new_end = i;
				break;
			}
		}
		if ((new_start >= start)&&(new_start <= end))
		{
			if (new_end > end)
			{
				end = new_end;
			}
		}
		else if ((new_end >= start)&&(new_end <= end))
		{
			if (new_start < start)
			{
				start = new_start;
			}
		}
		else
		{
			XPutBackEvent(XtDisplay(hw), &report);
			break;
		}
	}
#endif /* COMPRESS */


	/*
	 * If we have a GC draw the lines that overlap the exposed area.
	 */
	if (hw->html.drawGC != NULL)
	{
		for (i=start; i<=end; i++)
		{
			PlaceLine(hw, i);
		}
#ifdef EXTRA_FLUSH
		XFlush(XtDisplay(hw));
#endif

#ifdef MOTIF
#ifdef MOTIF1_2
		_XmRedisplayGadgets ((Widget)hw, (XEvent*)event, region);
#else
  		_XmRedisplayGadgets ((CompositeWidget)hw, (XExposeEvent*)event, region);
#endif /* MOTIF1_2 */
#endif /* MOTIF */
	}

	return;
}


/*
 * Resize is called when the widget changes size.
 * Right now we just call user resize callbacks.
 */
static void
#ifdef _NO_PROTO
Resize (hw)
            HTMLWidget hw;
#else
Resize(
            HTMLWidget hw)
#endif
{
#ifdef ALTERNATE_RESIZE
	XtWidgetGeometry request_geom, return_geom;

	/*
	 * If the width has changes from the width we formatted to, set
	 * the reformat flag, and redisplay the widget.
	 * If we are autosizing, compute the desired height, and try to set it.
	 */
	if (hw->core.width != hw->html.format_width)
	{
		if (hw->html.auto_size == True)
		{
			int temp;

			temp = FormatAll(hw, hw->core.width);
			if (temp != hw->core.height)
			{
				request_geom.request_mode = CWHeight;
				request_geom.height = temp;
				if (XtMakeGeometryRequest(XtParent(hw),
					&request_geom, &return_geom) !=
					XtGeometryYes)
				{
					XtResizeWidget((Widget)hw,
						hw->core.width, temp,
						hw->core.border_width);
				}
			}
			hw->html.reformat = True;
			Redisplay(hw, (XEvent *)NULL, (Region)NULL);
		}
		else
		{
			hw->html.reformat = True;
			Redisplay(hw, (XEvent *)NULL, (Region)NULL);
		}
	}
	else if (hw->html.auto_size == True)
	{
		int temp;

		temp = FormatAll(hw, hw->core.width);
		if (temp != hw->core.height)
		{
			request_geom.request_mode = CWHeight;
			request_geom.height = temp;
			if (XtMakeGeometryRequest(XtParent(hw),
				&request_geom, &return_geom) !=
				XtGeometryYes)
			{
				XtResizeWidget((Widget)hw,
					hw->core.width, temp,
					hw->core.border_width);
			}
		}
		hw->html.reformat = True;
		Redisplay(hw, (XEvent *)NULL, (Region)NULL);
	}
#endif /* ALTERNATE_RESIZE */

	if ((hw->core.width != hw->html.format_width)||
		(hw->core.height != hw->html.format_height))
	{
		hw->html.reformat = True;
		Redisplay(hw, (XEvent *)NULL, (Region)NULL);
	}
	else if (hw->html.reformat == True)
	{
		hw->html.reformat = False;
		Redisplay(hw, (XEvent *)NULL, (Region)NULL);
	}

	return;
}


/*
 * Find the complete text for this the anchor that aptr is a part of
 * and set it into the selection.
 */
static void
FindSelectAnchor(hw, aptr)
	HTMLWidget hw;
	struct ele_rec *aptr;
{
	struct ele_rec *eptr;

	eptr = aptr;
	while ((eptr->prev != NULL)&&
		(eptr->prev->anchorHRef != NULL)&&
		(strcmp(eptr->prev->anchorHRef, eptr->anchorHRef) == 0))
	{
		eptr = eptr->prev;
	}
	hw->html.select_start = eptr;
	hw->html.sel_start_pos = 0;

	eptr = aptr;
	while ((eptr->next != NULL)&&
		(eptr->next->anchorHRef != NULL)&&
		(strcmp(eptr->next->anchorHRef, eptr->anchorHRef) == 0))
	{
		eptr = eptr->next;
	}
	hw->html.select_end = eptr;
	hw->html.sel_end_pos = eptr->edata_len - 1;
}


/*
 * Set as active all elements in the widget that are part of the anchor
 * in the widget's start ptr.
 */
static void
SetAnchor(hw)
	HTMLWidget hw;
{
	struct ele_rec *eptr;
	struct ele_rec *start;
	struct ele_rec *end;
	unsigned long fg, bg;
	unsigned long old_fg, old_bg;

	eptr = hw->html.active_anchor;
	if ((eptr == NULL)||(eptr->anchorHRef == NULL))
	{
		return;
	}
	fg = hw->html.activeAnchor_fg;
	bg = hw->html.activeAnchor_bg;

	FindSelectAnchor(hw, eptr);

	start = hw->html.select_start;
	end = hw->html.select_end;

	eptr = start;
	while ((eptr != NULL)&&(eptr != end))
	{
		if (eptr->type == E_TEXT)
		{
			old_fg = eptr->fg;
			old_bg = eptr->bg;
			eptr->fg = fg;
			eptr->bg = bg;
			TextRefresh(hw, eptr,
				0, (eptr->edata_len - 1));
			eptr->fg = old_fg;
			eptr->bg = old_bg;
		}
		else if (eptr->type == E_IMAGE)
		{
			old_fg = eptr->fg;
			old_bg = eptr->bg;
			eptr->fg = fg;
			eptr->bg = bg;
			ImageRefresh(hw, eptr);
			eptr->fg = old_fg;
			eptr->bg = old_bg;
		}
	/*
	 * Linefeeds in anchor spanning multiple lines should NOT
	 * be highlighted!
		else if (eptr->type == E_LINEFEED)
		{
			old_fg = eptr->fg;
			old_bg = eptr->bg;
			eptr->fg = fg;
			eptr->bg = bg;
			LinefeedRefresh(hw, eptr);
			eptr->fg = old_fg;
			eptr->bg = old_bg;
		}
	*/
		eptr = eptr->next;
	}
	if (eptr != NULL)
	{
		if (eptr->type == E_TEXT)
		{
			old_fg = eptr->fg;
			old_bg = eptr->bg;
			eptr->fg = fg;
			eptr->bg = bg;
			TextRefresh(hw, eptr,
				0, (eptr->edata_len - 1));
			eptr->fg = old_fg;
			eptr->bg = old_bg;
		}
		else if (eptr->type == E_IMAGE)
		{
			old_fg = eptr->fg;
			old_bg = eptr->bg;
			eptr->fg = fg;
			eptr->bg = bg;
			ImageRefresh(hw, eptr);
			eptr->fg = old_fg;
			eptr->bg = old_bg;
		}
	/*
	 * Linefeeds in anchor spanning multiple lines should NOT
	 * be highlighted!
		else if (eptr->type == E_LINEFEED)
		{
			old_fg = eptr->fg;
			old_bg = eptr->bg;
			eptr->fg = fg;
			eptr->bg = bg;
			LinefeedRefresh(hw, eptr);
			eptr->fg = old_fg;
			eptr->bg = old_bg;
		}
	*/
	}
}


/*
 * Draw selection for all elements in the widget
 * from start to end.
 */
static void
DrawSelection(hw, start, end, start_pos, end_pos)
	HTMLWidget hw;
	struct ele_rec *start;
	struct ele_rec *end;
	int start_pos, end_pos;
{
	struct ele_rec *eptr;
	int epos;

	if ((start == NULL)||(end == NULL))
	{
		return;
	}

	/*
	 * Keep positions within bounds (allows us to be sloppy elsewhere)
	 */
	if (start_pos < 0)
	{
		start_pos = 0;
	}
	if (start_pos >= start->edata_len)
	{
		start_pos = start->edata_len - 1;
	}
	if (end_pos < 0)
	{
		end_pos = 0;
	}
	if (end_pos >= end->edata_len)
	{
		end_pos = end->edata_len - 1;
	}

	if (SwapElements(start, end, start_pos, end_pos))
	{
		eptr = start;
		start = end;
		end = eptr;
		epos = start_pos;
		start_pos = end_pos;
		end_pos = epos;
	}

	eptr = start;
	while ((eptr != NULL)&&(eptr != end))
	{
		int p1, p2;

		if (eptr == start)
		{
			p1 = start_pos;
		}
		else
		{
			p1 = 0;
		}
		p2 = eptr->edata_len - 1;

		if (eptr->type == E_TEXT)
		{
			eptr->selected = True;
			eptr->start_pos = p1;
			eptr->end_pos = p2;
			TextRefresh(hw, eptr, p1, p2);
		}
		else if (eptr->type == E_LINEFEED)
		{
			eptr->selected = True;
			LinefeedRefresh(hw, eptr);
		}
		eptr = eptr->next;
	}
	if (eptr != NULL)
	{
		int p1, p2;

		if (eptr == start)
		{
			p1 = start_pos;
		}
		else
		{
			p1 = 0;
		}

		if (eptr == end)
		{
			p2 = end_pos;
		}
		else
		{
			p2 = eptr->edata_len - 1;
		}

		if (eptr->type == E_TEXT)
		{
			eptr->selected = True;
			eptr->start_pos = p1;
			eptr->end_pos = p2;
			TextRefresh(hw, eptr, p1, p2);
		}
		else if (eptr->type == E_LINEFEED)
		{
			eptr->selected = True;
			LinefeedRefresh(hw, eptr);
		}
	}
}


/*
 * Set selection for all elements in the widget's
 * start to end list.
 */
static void
SetSelection(hw)
	HTMLWidget hw;
{
	struct ele_rec *eptr;
	struct ele_rec *start;
	struct ele_rec *end;
	int start_pos, end_pos;

	start = hw->html.select_start;
	end = hw->html.select_end;
	start_pos = hw->html.sel_start_pos;
	end_pos = hw->html.sel_end_pos;
	DrawSelection(hw, start, end, start_pos, end_pos);
}


/*
 * Erase the selection from start to end
 */
static void
EraseSelection(hw, start, end, start_pos, end_pos)
	HTMLWidget hw;
	struct ele_rec *start;
	struct ele_rec *end;
	int start_pos, end_pos;
{
	struct ele_rec *eptr;
	int epos;

	if ((start == NULL)||(end == NULL))
	{
		return;
	}

	/*
	 * Keep positoins within bounds (allows us to be sloppy elsewhere)
	 */
	if (start_pos < 0)
	{
		start_pos = 0;
	}
	if (start_pos >= start->edata_len)
	{
		start_pos = start->edata_len - 1;
	}
	if (end_pos < 0)
	{
		end_pos = 0;
	}
	if (end_pos >= end->edata_len)
	{
		end_pos = end->edata_len - 1;
	}

	if (SwapElements(start, end, start_pos, end_pos))
	{
		eptr = start;
		start = end;
		end = eptr;
		epos = start_pos;
		start_pos = end_pos;
		end_pos = epos;
	}

	eptr = start;
	while ((eptr != NULL)&&(eptr != end))
	{
		int p1, p2;

		if (eptr == start)
		{
			p1 = start_pos;
		}
		else
		{
			p1 = 0;
		}
		p2 = eptr->edata_len - 1;

		if (eptr->type == E_TEXT)
		{
			eptr->selected = False;
/*
			eptr->start_pos = p1;
			eptr->end_pos = p2;
*/
			TextRefresh(hw, eptr, p1, p2);
		}
		else if (eptr->type == E_LINEFEED)
		{
			eptr->selected = False;
			LinefeedRefresh(hw, eptr);
		}
		eptr = eptr->next;
	}
	if (eptr != NULL)
	{
		int p1, p2;

		if (eptr == start)
		{
			p1 = start_pos;
		}
		else
		{
			p1 = 0;
		}

		if (eptr == end)
		{
			p2 = end_pos;
		}
		else
		{
			p2 = eptr->edata_len - 1;
		}

		if (eptr->type == E_TEXT)
		{
			eptr->selected = False;
/*
			eptr->start_pos = p1;
			eptr->end_pos = p2;
*/
			TextRefresh(hw, eptr, p1, p2);
		}
		else if (eptr->type == E_LINEFEED)
		{
			eptr->selected = False;
			LinefeedRefresh(hw, eptr);
		}
	}
}


/*
 * Clear the current selection (if there is one)
 */
static void
ClearSelection(hw)
	HTMLWidget hw;
{
	struct ele_rec *eptr;
	struct ele_rec *start;
	struct ele_rec *end;
	int start_pos, end_pos;

	start = hw->html.select_start;
	end = hw->html.select_end;
	start_pos = hw->html.sel_start_pos;
	end_pos = hw->html.sel_end_pos;
	EraseSelection(hw, start, end, start_pos, end_pos);

	if ((start == NULL)||(end == NULL))
	{
		hw->html.select_start = NULL;
		hw->html.select_end = NULL;
		hw->html.sel_start_pos = 0;
		hw->html.sel_end_pos = 0;
		hw->html.active_anchor = NULL;
		return;
	}

	hw->html.select_start = NULL;
	hw->html.select_end = NULL;
	hw->html.sel_start_pos = 0;
	hw->html.sel_end_pos = 0;
	hw->html.active_anchor = NULL;
}


/*
 * clear from active all elements in the widget that are part of the anchor.
 * (These have already been previously set into the start and end of the
 * selection.
 */
static void
UnsetAnchor(hw)
	HTMLWidget hw;
{
	struct ele_rec *eptr;

	/*
	 * Clear any activated images
	 */
	eptr = hw->html.select_start;
	while ((eptr != NULL)&&(eptr != hw->html.select_end))
	{
		if (eptr->type == E_IMAGE)
		{
			ImageRefresh(hw, eptr);
		}
		eptr = eptr->next;
	}
	if ((eptr != NULL)&&(eptr->type == E_IMAGE))
	{
		ImageRefresh(hw, eptr);
	}

	/*
	 * Clear the activated anchor
	 */
	ClearSelection(hw);
}


/*
 * Erase the old selection, and draw the new one in such a way
 * that advantage is taken of overlap, and there is no obnoxious
 * flashing.
 */
static void
ChangeSelection(hw, start, end, start_pos, end_pos)
	HTMLWidget hw;
	struct ele_rec *start;
	struct ele_rec *end;
	int start_pos, end_pos;
{
	struct ele_rec *old_start;
	struct ele_rec *old_end;
	struct ele_rec *new_start;
	struct ele_rec *new_end;
	struct ele_rec *eptr;
	int epos;
	int new_start_pos, new_end_pos;
	int old_start_pos, old_end_pos;

	old_start = hw->html.new_start;
	old_end = hw->html.new_end;
	old_start_pos = hw->html.new_start_pos;
	old_end_pos = hw->html.new_end_pos;
	new_start = start;
	new_end = end;
	new_start_pos = start_pos;
	new_end_pos = end_pos;

	if ((new_start == NULL)||(new_end == NULL))
	{
		return;
	}

	if ((old_start == NULL)||(old_end == NULL))
	{
		DrawSelection(hw, new_start, new_end,
			new_start_pos, new_end_pos);
		return;
	}

	if (SwapElements(old_start, old_end, old_start_pos, old_end_pos))
	{
		eptr = old_start;
		old_start = old_end;
		old_end = eptr;
		epos = old_start_pos;
		old_start_pos = old_end_pos;
		old_end_pos = epos;
	}

	if (SwapElements(new_start, new_end, new_start_pos, new_end_pos))
	{
		eptr = new_start;
		new_start = new_end;
		new_end = eptr;
		epos = new_start_pos;
		new_start_pos = new_end_pos;
		new_end_pos = epos;
	}

	/*
	 * Deal with all possible intersections of the 2 selection sets.
	 *
	 ********************************************************
	 *			*				*
	 *      |--		*	     |--		*
	 * old--|		*	new--|			*
	 *      |--		*	     |--		*
	 *			*				*
	 *      |--		*	     |--		*
	 * new--|		*	old--|			*
	 *      |--		*	     |--		*
	 *			*				*
	 ********************************************************
	 *			*				*
	 *      |----		*	       |--		*
	 * old--|		*	  new--|		*
	 *      | |--		*	       |		*
	 *      |-+--		*	     |-+--		*
	 *        |		*	     | |--		*
	 *   new--|		*	old--|			*
	 *        |--		*	     |----		*
	 *			*				*
	 ********************************************************
	 *			*				*
	 *      |---------	*	     |---------		*
	 *      |		*	     |			*
	 *      |      |--	*	     |      |--		*
	 * new--| old--|	*	old--| new--|		*
	 *      |      |--	*	     |      |--		*
	 *      |		*	     |			*
	 *      |---------	*	     |---------		*
	 *			*				*
	 ********************************************************
	 *
	 */
	if ((ElementLessThan(old_end, new_start, old_end_pos, new_start_pos))||
	    (ElementLessThan(new_end, old_start, new_end_pos, old_start_pos)))
	{
		EraseSelection(hw, old_start, old_end,
			old_start_pos, old_end_pos);
		DrawSelection(hw, new_start, new_end,
			new_start_pos, new_end_pos);
	}
	else if ((ElementLessThan(old_start, new_start,
			old_start_pos, new_start_pos))&&
		 (ElementLessThan(old_end, new_end, old_end_pos, new_end_pos)))
	{
		if (new_start_pos != 0)
		{
			EraseSelection(hw, old_start, new_start,
				old_start_pos, new_start_pos - 1);
		}
		else
		{
			EraseSelection(hw, old_start, new_start->prev,
				old_start_pos, new_start->prev->edata_len - 1);
		}
		if (old_end_pos < (old_end->edata_len - 1))
		{
			DrawSelection(hw, old_end, new_end,
				old_end_pos + 1, new_end_pos);
		}
		else
		{
			DrawSelection(hw, old_end->next, new_end,
				0, new_end_pos);
		}
	}
	else if ((ElementLessThan(new_start, old_start,
			new_start_pos, old_start_pos))&&
		 (ElementLessThan(new_end, old_end, new_end_pos, old_end_pos)))
	{
		if (old_start_pos != 0)
		{
			DrawSelection(hw, new_start, old_start,
				new_start_pos, old_start_pos - 1);
		}
		else
		{
			DrawSelection(hw, new_start, old_start->prev,
				new_start_pos, old_start->prev->edata_len - 1);
		}
		if (new_end_pos < (new_end->edata_len - 1))
		{
			EraseSelection(hw, new_end, old_end,
				new_end_pos + 1, old_end_pos);
		}
		else
		{
			EraseSelection(hw, new_end->next, old_end,
				0, old_end_pos);
		}
	}
	else if ((ElementLessThan(new_start, old_start,
			new_start_pos, old_start_pos))||
		 (ElementLessThan(old_end, new_end, old_end_pos, new_end_pos)))
	{
		if ((new_start != old_start)||(new_start_pos != old_start_pos))
		{
			if (old_start_pos != 0)
			{
				DrawSelection(hw, new_start, old_start,
					new_start_pos, old_start_pos - 1);
			}
			else
			{
				DrawSelection(hw, new_start, old_start->prev,
					new_start_pos,
					old_start->prev->edata_len - 1);
			}
		}
		if ((old_end != new_end)||(old_end_pos != new_end_pos))
		{
			if (old_end_pos < (old_end->edata_len - 1))
			{
				DrawSelection(hw, old_end, new_end,
					old_end_pos + 1, new_end_pos);
			}
			else
			{
				DrawSelection(hw, old_end->next, new_end,
					0, new_end_pos);
			}
		}
	}
	else
	{
		if ((old_start != new_start)||(old_start_pos != new_start_pos))
		{
			if (new_start_pos != 0)
			{
				EraseSelection(hw, old_start, new_start,
					old_start_pos, new_start_pos - 1);
			}
			else
			{
				EraseSelection(hw, old_start, new_start->prev,
					old_start_pos,
					new_start->prev->edata_len - 1);
			}
		}
		if ((new_end != old_end)||(new_end_pos != old_end_pos))
		{
			if (new_end_pos < (new_end->edata_len - 1))
			{
				EraseSelection(hw, new_end, old_end,
					new_end_pos + 1, old_end_pos);
			}
			else
			{
				EraseSelection(hw, new_end->next, old_end,
					0, old_end_pos);
			}
		}
	}
}


static void
SelectStart(w, event, params, num_params)
	Widget w;
	XEvent *event;
	String *params;         /* unused */
	Cardinal *num_params;   /* unused */
{
	HTMLWidget hw = (HTMLWidget)w;
	XButtonPressedEvent *BuEvent = (XButtonPressedEvent *)event;
	struct ele_rec *eptr;
	int epos;

	/*
	 * Because X sucks, we can get the button pressed in the window, but
	 * released out of the window.  This will highlight some text, but
	 * never complete the selection.  Now on the next button press we
	 * have to clean up this mess.
	 */
	EraseSelection(hw, hw->html.new_start, hw->html.new_end,
		hw->html.new_start_pos, hw->html.new_end_pos);

	/*
	 * We want to erase the currently selected text, but still save the
	 * selection internally in case we don't create a new one.
	 */
	EraseSelection(hw, hw->html.select_start, hw->html.select_end,
		hw->html.sel_start_pos, hw->html.sel_end_pos);
	hw->html.new_start = hw->html.select_start;
	hw->html.new_end = NULL;
	hw->html.new_start_pos = hw->html.sel_start_pos;
	hw->html.new_end_pos = 0;

	eptr = LocateElement(hw, BuEvent->x, BuEvent->y, &epos);
	if (eptr != NULL)
	{
		/*
		 * If this is an anchor assume for now we are activating it
		 * and not selecting it.
		 */
		if (eptr->anchorHRef != NULL)
		{
			hw->html.active_anchor = eptr;
			hw->html.press_x = BuEvent->x;
			hw->html.press_y = BuEvent->y;
			SetAnchor(hw);
		}
		/*
		 * Else if we are on an image we can't select text so
		 * pretend we got eptr==NULL, and exit here.
		 */
		else if (eptr->type == E_IMAGE)
		{
			hw->html.new_start = NULL;
			hw->html.new_end = NULL;
			hw->html.new_start_pos = 0;
			hw->html.new_end_pos = 0;
			hw->html.press_x = BuEvent->x;
			hw->html.press_y = BuEvent->y;
			hw->html.but_press_time = BuEvent->time;
			return;
		}
		/*
		 * Else if we used button2, we can't select text, so exit
		 * here.
		 */
		else if (BuEvent->button == Button2)
		{
			hw->html.press_x = BuEvent->x;
			hw->html.press_y = BuEvent->y;
			hw->html.but_press_time = BuEvent->time;
			return;
		}
		/*
		 * Else a single click will not select a new object
		 * but it will prime that selection on the next mouse
		 * move.
		 * Ignore special internal text added for multi-page
		 * documents.
		 */
		else if (eptr->internal == False)
		{
			hw->html.new_start = eptr;
			hw->html.new_start_pos = epos;
			hw->html.new_end = NULL;
			hw->html.new_end_pos = 0;
			hw->html.press_x = BuEvent->x;
			hw->html.press_y = BuEvent->y;
		}
		else
		{
			hw->html.new_start = NULL;
			hw->html.new_end = NULL;
			hw->html.new_start_pos = 0;
			hw->html.new_end_pos = 0;
			hw->html.press_x = BuEvent->x;
			hw->html.press_y = BuEvent->y;
		}
	}
	else
	{
		hw->html.new_start = NULL;
		hw->html.new_end = NULL;
		hw->html.new_start_pos = 0;
		hw->html.new_end_pos = 0;
		hw->html.press_x = BuEvent->x;
		hw->html.press_y = BuEvent->y;
	}
	hw->html.but_press_time = BuEvent->time;
}


static void
ExtendStart(w, event, params, num_params)
	Widget w;
	XEvent *event;
	String *params;         /* unused */
	Cardinal *num_params;   /* unused */
{
	HTMLWidget hw = (HTMLWidget)w;
	XButtonPressedEvent *BuEvent = (XButtonPressedEvent *)event;
	struct ele_rec *eptr;
	struct ele_rec *start, *end;
	struct ele_rec *old_start, *old_end;
	int old_start_pos, old_end_pos;
	int start_pos, end_pos;
	int epos;

	eptr = LocateElement(hw, BuEvent->x, BuEvent->y, &epos);

	/*
	 * Ignore IMAGE elements.
	 */
	if ((eptr != NULL)&&(eptr->type == E_IMAGE))
	{
		eptr = NULL;
	}

	/*
	 * Ignore NULL elements.
	 * Ignore special internal text added for multi-page
	 * documents.
	 */
	if ((eptr != NULL)&&(eptr->internal == False))
	{
		old_start = hw->html.new_start;
		old_start_pos = hw->html.new_start_pos;
		old_end = hw->html.new_end;
		old_end_pos = hw->html.new_end_pos;
		if (hw->html.new_start == NULL)
		{
			hw->html.new_start = hw->html.select_start;
			hw->html.new_start_pos = hw->html.sel_start_pos;
			hw->html.new_end = hw->html.select_end;
			hw->html.new_end_pos = hw->html.sel_end_pos;
		}
		else
		{
			hw->html.new_end = eptr;
			hw->html.new_end_pos = epos;
		}

		if (SwapElements(hw->html.new_start, hw->html.new_end,
			hw->html.new_start_pos, hw->html.new_end_pos))
		{
			if (SwapElements(eptr, hw->html.new_end,
				epos, hw->html.new_end_pos))
			{
				start = hw->html.new_end;
				start_pos = hw->html.new_end_pos;
				end = eptr;
				end_pos = epos;
			}
			else
			{
				start = hw->html.new_start;
				start_pos = hw->html.new_start_pos;
				end = eptr;
				end_pos = epos;
			}
		}
		else
		{
			if (SwapElements(eptr, hw->html.new_start,
				epos, hw->html.new_start_pos))
			{
				start = hw->html.new_start;
				start_pos = hw->html.new_start_pos;
				end = eptr;
				end_pos = epos;
			}
			else
			{
				start = hw->html.new_end;
				start_pos = hw->html.new_end_pos;
				end = eptr;
				end_pos = epos;
			}
		}

		if (start == NULL)
		{
			start = eptr;
			start_pos = epos;
		}

		if (old_start == NULL)
		{
			hw->html.new_start = hw->html.select_start;
			hw->html.new_end = hw->html.select_end;
			hw->html.new_start_pos = hw->html.sel_start_pos;
			hw->html.new_end_pos = hw->html.sel_end_pos;
		}
		else
		{
			hw->html.new_start = old_start;
			hw->html.new_end = old_end;
			hw->html.new_start_pos = old_start_pos;
			hw->html.new_end_pos = old_end_pos;
		}
		ChangeSelection(hw, start, end, start_pos, end_pos);
		hw->html.new_start = start;
		hw->html.new_end = end;
		hw->html.new_start_pos = start_pos;
		hw->html.new_end_pos = end_pos;
	}
	else
	{
		if (hw->html.new_start == NULL)
		{
			hw->html.new_start = hw->html.select_start;
			hw->html.new_start_pos = hw->html.sel_start_pos;
			hw->html.new_end = hw->html.select_end;
			hw->html.new_end_pos = hw->html.sel_end_pos;
		}
	}
	hw->html.press_x = BuEvent->x;
	hw->html.press_y = BuEvent->y;
}


static void
ExtendAdjust(w, event, params, num_params)
	Widget w;
	XEvent *event;
	String *params;         /* unused */
	Cardinal *num_params;   /* unused */
{
	HTMLWidget hw = (HTMLWidget)w;
	XPointerMovedEvent *MoEvent = (XPointerMovedEvent *)event;
	struct ele_rec *eptr;
	struct ele_rec *start, *end;
	int start_pos, end_pos;
	int epos;

	/*
	 * Very small mouse motion immediately after button press
	 * is ignored.
	 */
	if ((ABS((hw->html.press_x - MoEvent->x)) <= SELECT_THRESHOLD)&&
	    (ABS((hw->html.press_y - MoEvent->y)) <= SELECT_THRESHOLD))
	{
		return;
	}

	/*
	 * If we have an active anchor and we got here, we have moved the
	 * mouse too far.  Deactivate anchor, and prime a selection.
	 * If the anchor is internal from multi-page text, don't
	 * prime a selection.
	 */
	if (hw->html.active_anchor != NULL)
	{
		eptr = hw->html.active_anchor;
		UnsetAnchor(hw);
		if (eptr->internal == False)
		{
			hw->html.new_start = NULL;
			hw->html.new_start_pos = 0;
			hw->html.new_end = NULL;
			hw->html.new_end_pos = 0;
		}
	}

	/*
	 * If we used button2, we can't select text, so
	 * clear selection and exit here.
	 */
	if ((MoEvent->state & Button2Mask) != 0)
	{
		hw->html.select_start = NULL;
		hw->html.select_end = NULL;
		hw->html.sel_start_pos = 0;
		hw->html.sel_end_pos = 0;
		hw->html.new_start = NULL;
		hw->html.new_end = NULL;
		hw->html.new_start_pos = 0;
		hw->html.new_end_pos = 0;
		return;
	}

	eptr = LocateElement(hw, MoEvent->x, MoEvent->y, &epos);

	/*
	 * If we are on an image pretend we are nowhere
	 * and just return;
	 */
	if ((eptr != NULL)&&(eptr->type == E_IMAGE))
	{
		return;
	}

	/*
	 * Ignore NULL items.
	 * Ignore if the same as last selected item and position.
	 * Ignore special internal text from multi-page documents.
	 */
	if ((eptr != NULL)&&
	    ((eptr != hw->html.new_end)||(epos != hw->html.new_end_pos))&&
	    (eptr->internal == False))
	{
		start = hw->html.new_start;
		start_pos = hw->html.new_start_pos;
		end = eptr;
		end_pos = epos;
		if (start == NULL)
		{
			start = eptr;
			start_pos = epos;
		}

		ChangeSelection(hw, start, end, start_pos, end_pos);
		hw->html.new_start = start;
		hw->html.new_end = end;
		hw->html.new_start_pos = start_pos;
		hw->html.new_end_pos = end_pos;
	}
}


static void
ExtendEnd(w, event, params, num_params)
	Widget w;
	XEvent *event;
	String *params;
	Cardinal *num_params;
{
	HTMLWidget hw = (HTMLWidget)w;
	XButtonReleasedEvent *BuEvent = (XButtonReleasedEvent *)event;
	struct ele_rec *eptr;
	struct ele_rec *start, *end;
	Atom *atoms;
	int i, buffer;
	int start_pos, end_pos;
	int epos;
	char *text;

	eptr = LocateElement(hw, BuEvent->x, BuEvent->y, &epos);

	/*
	 * If we just released button one or two, and we are on an object,
	 * and we have an active anchor, and we are on the active anchor,
	 * and if we havn't waited too long.  Activate that anchor.
	 */
	if (((BuEvent->button == Button1)||(BuEvent->button == Button2))&&
		(eptr != NULL)&&
		(hw->html.active_anchor != NULL)&&
		(eptr == hw->html.active_anchor)&&
		((BuEvent->time - hw->html.but_press_time) < CLICK_TIME))
	{
		_HTMLInput(hw, event, params, num_params);
		return;
	}
	else if (hw->html.active_anchor != NULL)
	{
		start = hw->html.active_anchor;
		UnsetAnchor(hw);
		if (start->internal == False)
		{
			hw->html.new_start = eptr;
			hw->html.new_start_pos = epos;
			hw->html.new_end = NULL;
			hw->html.new_end_pos = 0;
		}
	}

	/*
	 * If we used button2, we can't select text, so clear
	 * selection and exit here.
	 */
	if (BuEvent->button == Button2)
	{
		hw->html.new_start = hw->html.select_start;
		hw->html.new_end = NULL;
		hw->html.new_start_pos = hw->html.sel_start_pos;
		hw->html.new_end_pos = 0;
		return;
	}

	/*
	 * If we are on an image, pretend we are nowhere
	 * and NULL out the eptr
	 */
	if ((eptr != NULL)&&(eptr->type == E_IMAGE))
	{
		eptr = NULL;
	}

	/*
	 * If button released on a NULL item, take the last non-NULL
	 * item that we highlighted.
	 */
	if ((eptr == NULL)&&(hw->html.new_end != NULL))
	{
		eptr = hw->html.new_end;
		epos = hw->html.new_end_pos;
	}

	if ((eptr != NULL)&&(eptr->internal == False)&&
		(hw->html.new_end != NULL))
	{
		start = hw->html.new_start;
		start_pos = hw->html.new_start_pos;
		end = eptr;
		end_pos = epos;
		if (start == NULL)
		{
			start = eptr;
			start_pos = epos;
		}
		ChangeSelection(hw, start, end, start_pos, end_pos);
		hw->html.select_start = start;
		hw->html.sel_start_pos = start_pos;
		hw->html.select_end = end;
		hw->html.sel_end_pos = end_pos;
		SetSelection(hw);
		hw->html.new_start = NULL;
		hw->html.new_end = NULL;
		hw->html.new_start_pos = 0;
		hw->html.new_end_pos = 0;

		atoms = (Atom *)malloc(*num_params * sizeof(Atom));
		if (atoms == NULL)
		{
			fprintf(stderr, "cannot allocate atom list\n");
			return;
		}
		XmuInternStrings(XtDisplay((Widget)hw), params, *num_params, atoms);
		hw->html.selection_time = BuEvent->time;
		for (i=0; i< *num_params; i++)
		{
			switch (atoms[i])
			{
				case XA_CUT_BUFFER0: buffer = 0; break;
				case XA_CUT_BUFFER1: buffer = 1; break;
				case XA_CUT_BUFFER2: buffer = 2; break;
				case XA_CUT_BUFFER3: buffer = 3; break;
				case XA_CUT_BUFFER4: buffer = 4; break;
				case XA_CUT_BUFFER5: buffer = 5; break;
				case XA_CUT_BUFFER6: buffer = 6; break;
				case XA_CUT_BUFFER7: buffer = 7; break;
				default: buffer = -1; break;
			}
			if (buffer >= 0)
			{
				if (hw->html.fancy_selections == True)
				{
				    text = ParseTextToPrettyString(hw,
					hw->html.formatted_elements,
					hw->html.select_start,
					hw->html.select_end,
					hw->html.sel_start_pos,
					hw->html.sel_end_pos,
					hw->html.font->max_bounds.width,
					hw->html.margin_width);
				}
				else
				{
				    text = ParseTextToString(
					hw->html.formatted_elements,
					hw->html.select_start,
					hw->html.select_end,
					hw->html.sel_start_pos,
					hw->html.sel_end_pos,
					hw->html.font->max_bounds.width,
					hw->html.margin_width);
				}
				XStoreBuffer(XtDisplay((Widget)hw),
					text, strlen(text), buffer);
				if (text != NULL)
				{
					free(text);
				}
			}
			else
			{
				XtOwnSelection((Widget)hw, atoms[i],
					BuEvent->time, ConvertSelection,
					LoseSelection, SelectionDone);
			}
		}
		free((char *)atoms);
	}
	else if (eptr == NULL)
	{
		hw->html.select_start = NULL;
		hw->html.sel_start_pos = 0;
		hw->html.select_end = NULL;
		hw->html.sel_end_pos = 0;
		hw->html.new_start = NULL;
		hw->html.new_start_pos = 0;
		hw->html.new_end = NULL;
		hw->html.new_end_pos = 0;
	}
}


/*
 * Goto the page passed in a multi-page document
 */
void
GotoPage(hw, page)
	HTMLWidget hw;
	int page;
{
#ifdef MOTIF
	Widget swin;
#endif /* MOTIF */

	if (hw->html.pages[(page - 1)].elist == NULL)
	{
		return;
	}

	if (page > hw->html.page_cnt)
	{
		page = hw->html.page_cnt;
	}
	if (page < 1)
	{
		page = 1;
	}
	hw->html.select_start = NULL;
	hw->html.select_end = NULL;
	hw->html.sel_start_pos = 0;
	hw->html.sel_end_pos = 0;
	hw->html.new_start = NULL;
	hw->html.new_end = NULL;
	hw->html.new_start_pos = 0;
	hw->html.new_end_pos = 0;
	hw->html.active_anchor = NULL;
	hw->html.formatted_elements = hw->html.pages[(page - 1)].elist;
	hw->html.format_height = hw->html.pages[(page - 1)].pheight;
	hw->html.line_count = hw->html.pages[(page - 1)].line_num;
	if (hw->html.line_array != NULL)
	{
		free((char *)hw->html.line_array);
	}
	hw->html.line_array = MakeLineList(hw->html.formatted_elements,
		hw->html.line_count);
	hw->html.reformat = False;
	hw->html.current_page = page;

/*
 * Hack to get the assumed XmScrolledWindow parent to do the right thing if
 * we are Motif, and its child.
 */
#ifdef MOTIF
	swin = XtParent((Widget)hw);
	if (swin != NULL)
	{
		XtUnmanageChild((Widget)hw);
	}
#endif /* MOTIF */

	if ((hw->html.auto_size == True)&&
		(hw->html.format_height != hw->core.height))
	{
		XtResizeWidget((Widget)hw, hw->core.width,
			hw->html.format_height, hw->core.border_width);
	}

#ifdef MOTIF
	if (swin != NULL)
	{
		XtManageChild((Widget)hw);
	}
        XtCallCallbackList ((Widget)hw,
                            hw->html.document_page_callback,
                            /* No call data, I don't think. */
                            0);
#endif /* MOTIF */
}


/*
 * Process mouse input to the HTML widget
 * Currently only processes an anchor-activate when Button1
 * is pressed
 */
static void
#ifdef _NO_PROTO
_HTMLInput( hw, event, params, num_params)
	HTMLWidget hw ;
	XEvent *event ;
	String *params;		/* unused */
	Cardinal *num_params;	/* unused */
#else
_HTMLInput(
	HTMLWidget hw,
	XEvent *event,
	String *params,		/* unused */
	Cardinal *num_params)	/* unused */
#endif
{   
	struct ele_rec *eptr;
	WbAnchorCallbackData cbdata;
	int epos;
#ifdef MOTIF
	Boolean on_gadget;
#endif /* MOTIF */

#ifdef MOTIF
	/*
	 * If motif is defined, we don't want to process this button press
	 * if it is on a gadget
	 */
#ifdef MOTIF1_2
	on_gadget = (_XmInputForGadget((Widget)hw,
#else
	on_gadget = (_XmInputForGadget((CompositeWidget)hw,
#endif /* MOTIF1_2 */
				event->xbutton.x, event->xbutton.y) != NULL);

	if (on_gadget)
	{
		return;
	}
#endif /* MOTIF */

	if (event->type == ButtonRelease)
	{
		eptr = LocateElement(hw, event->xbutton.x, event->xbutton.y,
				&epos);
		if (eptr != NULL)
		{
			if (eptr->anchorHRef != NULL)
			{
			    char *tptr, *ptr;

			   /*
			    * Save the anchor text, replace newlines with
			    * spaces.
			    */
			    tptr = ParseTextToString(hw->html.select_start,
				hw->html.select_start, hw->html.select_end,
				hw->html.sel_start_pos, hw->html.sel_end_pos,
				hw->html.font->max_bounds.width,
				hw->html.margin_width);
			    ptr = tptr;
			    while ((ptr != NULL)&&(*ptr != '\0'))
			    {
				if (*ptr == '\n')
				{
					*ptr = ' ';
				}
				ptr++;
			    }

			   /*
			    * Clear the activated anchor
			    */
			    UnsetAnchor(hw);
#ifdef EXTRA_FLUSH
			    XFlush(XtDisplay(hw));
#endif

			    if (strncmp(eptr->anchorHRef, "Internal Page ",
				strlen("Internal Page ")) == 0)
			    {
				char buf[100];
				char *ptr;
				int pnum;

				/*
				 * If the page we let isn't marked visited,
				 * mark it now.
				 */
				sprintf(buf, "Internal Page %d",
					hw->html.current_page);
				if (FindHRef(hw->html.my_visited_hrefs, buf) ==
					NULL)
				{
					hw->html.my_visited_hrefs = AddHRef(
						hw->html.my_visited_hrefs, buf);
					RecolorInternalHRefs(hw, buf);
				}

				/*
				 * Add href to list of visited internal hrefs.
				 * Change colors to reflect visit.
				 */
				hw->html.my_visited_hrefs = AddHRef(
					hw->html.my_visited_hrefs,
					eptr->anchorHRef);
				RecolorInternalHRefs(hw, eptr->anchorHRef);

				ptr = (char *)(eptr->anchorHRef +
					strlen("Internal Page "));
				pnum = atoi(ptr);
				GotoPage(hw, pnum);
			    }
			    else
			    {
			     	/* The following is a hack to send the
			     	 * selection location along with the HRef
			     	 * for images.  This allows you to
			     	 * point at a location on a map and have
			     	 * the server send you the related document.
			     	 * Tony Sanders, April 1993 <sanders@bsdi.com>
			     	 */
			     	int alloced = 0;
			     	char *buf = eptr->anchorHRef;
				if (eptr->type == E_IMAGE && eptr->pic_data
				        && eptr->pic_data->ismap) {
				    buf = (char *)
				        malloc(strlen(eptr->anchorHRef) + 256);
				    alloced = 1;
				    sprintf(buf, "%s?%d,%d",
					eptr->anchorHRef,
					event->xbutton.x - eptr->x,
					event->xbutton.y - eptr->y);
			        }
				/*
				 * XXX: should call a media dependent
				 * function that decides how to munge the
				 * HRef.  For example mpeg data will want
				 * to know on what frame the event occured.
				 *
				 * cddata.href = *(eptr->eventf)(eptr, event);
			         */
				cbdata.event = event;
				cbdata.element_id = eptr->ele_id;
				cbdata.page = hw->html.current_page;
				cbdata.href = buf;
				/* cbdata.href = eptr->anchorHRef; */
				cbdata.text = tptr;
				XtCallCallbackList ((Widget)hw,
					hw->html.anchor_callback,
					(XtPointer)&cbdata);
			        if (alloced) free(buf);
			    }
			}
		}
	}

	return;
}



/*
 * SetValues is called when XtSetValues is used to change resources in this
 * widget.
 */
static Boolean
#ifdef _NO_PROTO
SetValues (current, request, new)
            HTMLWidget current ;
            HTMLWidget request ;
            HTMLWidget new ;
#else
SetValues(
            HTMLWidget current,
            HTMLWidget request,
            HTMLWidget new)
#endif
{
	int reformatted;

	/*
	 *	Make sure the underline numbers are within bounds.
	 */
	if (request->html.num_anchor_underlines < 0)
	{
		new->html.num_anchor_underlines = 0;
	} 
	if (request->html.num_anchor_underlines > MAX_UNDERLINES)
	{
		new->html.num_anchor_underlines = MAX_UNDERLINES;
	} 
	if (request->html.num_visitedAnchor_underlines < 0)
	{
		new->html.num_visitedAnchor_underlines = 0;
	} 
	if (request->html.num_visitedAnchor_underlines > MAX_UNDERLINES)
	{
		new->html.num_visitedAnchor_underlines = MAX_UNDERLINES;
	} 

	reformatted = 0;
	if ((request->html.raw_text != current->html.raw_text)||
	    (request->html.header_text != current->html.header_text)||
	    (request->html.footer_text != current->html.footer_text))
	{
		/*
		 * Free up the old visited href list.
		 */
		FreeHRefs(current->html.my_visited_hrefs);
		new->html.my_visited_hrefs = NULL;

		/*
		 * Free any old colors and pixmaps
		 */
		FreeColors(XtDisplay(current), DefaultColormapOfScreen(XtScreen(current)));
		FreeImages(current);

		/*
		 * Parse the raw text with the HTML parser
		 */
		new->html.html_objects = HTMLParse(current->html.html_objects,
			request->html.raw_text);
		new->html.html_header_objects =
			HTMLParse(current->html.html_header_objects,
			request->html.header_text);
		new->html.html_footer_objects =
			HTMLParse(current->html.html_footer_objects,
			request->html.footer_text);

		/*
		 * Redisplay for the changed data.
		 * if auto-sizing, change window size.
		 */
		if (new->html.auto_size == True)
		{
			int nwidth, nheight;

			nwidth = DocumentWidth(new, new->html.html_objects);
			new->html.max_pre_width = nwidth;
			nwidth = nwidth + (2 * new->html.margin_width);
			if (new->core.width > nwidth)
			{
				nwidth = new->core.width;
			}

			new->html.format_width = 0;
			new->html.format_height = 0;
			new->html.reformat = True;
			Redisplay(new, (XEvent *)NULL, (Region)NULL);
			reformatted = 1;
		}
		else
		{
			new->html.format_width = 0;
			new->html.format_height = 0;
			new->html.reformat = True;
			Redisplay(new, (XEvent *)NULL, (Region)NULL);
			reformatted = 1;
		}

		/*
		 * Clear any previous selection
		 */
		new->html.select_start = NULL;
		new->html.select_end = NULL;
		new->html.sel_start_pos = 0;
		new->html.sel_end_pos = 0;
		new->html.new_start = NULL;
		new->html.new_end = NULL;
		new->html.new_start_pos = 0;
		new->html.new_end_pos = 0;
		new->html.active_anchor = NULL;
	}
	else if ((request->html.font != current->html.font)||
	         (request->html.italic_font != current->html.italic_font)||
	         (request->html.bold_font != current->html.bold_font)||
	         (request->html.fixed_font != current->html.fixed_font)||
	         (request->html.header1_font != current->html.header1_font)||
	         (request->html.header2_font != current->html.header2_font)||
	         (request->html.header3_font != current->html.header3_font)||
	         (request->html.header4_font != current->html.header4_font)||
	         (request->html.header5_font != current->html.header5_font)||
	         (request->html.header6_font != current->html.header6_font)||
	         (request->html.address_font != current->html.address_font)||
	         (request->html.plain_font != current->html.plain_font)||
	         (request->html.listing_font != current->html.listing_font)||
	         (request->html.activeAnchor_fg != current->html.activeAnchor_fg)||
	         (request->html.activeAnchor_bg != current->html.activeAnchor_bg)||
	         (request->html.anchor_fg != current->html.anchor_fg)||
	         (request->html.visitedAnchor_fg != current->html.visitedAnchor_fg)||
	         (request->html.dashed_anchor_lines != current->html.dashed_anchor_lines)||
	         (request->html.dashed_visitedAnchor_lines != current->html.dashed_visitedAnchor_lines)||
	         (request->html.num_anchor_underlines != current->html.num_anchor_underlines)||
	         (request->html.num_visitedAnchor_underlines != current->html.num_visitedAnchor_underlines))
	{
		if ((request->html.plain_font != current->html.plain_font)||
		    (request->html.listing_font != current->html.listing_font))
		{
			int temp;

			temp = DocumentWidth(new, new->html.html_objects);
			new->html.max_pre_width = temp;
			temp = temp + (2 * new->html.margin_width);
			if (new->core.width < temp)
			{
				XtResizeWidget((Widget)new, temp,
					new->core.height,
					new->core.border_width);
			}
		}

		new->html.format_width = 0;
		new->html.format_height = 0;
		new->html.reformat = True;
		Redisplay(new, (XEvent *)NULL, (Region)NULL);
		reformatted = 1;
	}

	if (request->html.page_height != current->html.page_height)
	{
		/*
		 * Make sure page_height is within bounds.
		 */
		if (new->html.page_height > PAGE_HEIGHT_MAX)
		{
			fprintf(stderr, "Warning: pageHeight resource cannot exceed %d\n", PAGE_HEIGHT_MAX);
			new->html.page_height = PAGE_HEIGHT_MAX;
		}
		else if (new->html.page_height < PAGE_HEIGHT_MIN)
		{
			fprintf(stderr, "Warning: pageHeight resource must exceed %d\n", PAGE_HEIGHT_MIN);
			new->html.page_height = PAGE_HEIGHT_MIN;
		}

		new->html.format_width = 0;
		new->html.format_height = 0;
		new->html.reformat = True;
		Redisplay(new, (XEvent *)NULL, (Region)NULL);
		reformatted = 1;
	}

	/*
	 * image borders have been changed
	 */
	if (request->html.border_images != current->html.border_images)
	{
		new->html.reformat = True;
		Redisplay(new, (XEvent *)NULL, (Region)NULL);
		reformatted = 1;
	}

	/*
	 * vertical space has been changed
	 */
	if(request->html.percent_vert_space != current->html.percent_vert_space)
	{
		new->html.reformat = True;
		Redisplay(new, (XEvent *)NULL, (Region)NULL);
		reformatted = 1;
	}

	/*
	 * If the current page has been changed, and we have not
	 * reformatted, we better go to that page.
	 */
	if (request->html.current_page != current->html.current_page)
	{
		if (!reformatted)
		{
			GotoPage(new, request->html.current_page);
		}
	}

	return(False);
}


/*
 * Search through the whole document, and recolor the internal elements with
 * the passed HREF.
 */
static void
#ifdef _NO_PROTO
RecolorInternalHRefs(hw, href)
	HTMLWidget hw;
	char *href;
#else
RecolorInternalHRefs(HTMLWidget hw, char *href)
#endif
{
	int p;
	struct ele_rec *start;
	unsigned long fg;

	fg = hw->html.visitedAnchor_fg;
	/*
	 * Search all elements on all pages.
	 */
	for (p = 1; p <= hw->html.page_cnt; p++)
	{
		start = hw->html.pages[(p - 1)].elist;
		/*
		 * loop through this whole page
		 */
		while (start != NULL)
		{
			/*
			 * This one is internal
			 * This one has an href
			 * This is the href we want
			 */
			if ((start->internal == True)&&
			    (start->anchorHRef != NULL)&&
			    (strcmp(start->anchorHRef, href) == 0))
			{
				start->fg = fg;
				start->underline_number =
					hw->html.num_visitedAnchor_underlines;
				start->dashed_underline =
					hw->html.dashed_visitedAnchor_lines;
			}
			start = start->next;
		}
	}
}



static Boolean
ConvertSelection(w, selection, target, type, value, length, format)
	Widget w;
	Atom *selection, *target, *type;
	caddr_t *value;
	unsigned long *length;
	int *format;
{
	Display *d = XtDisplay(w);
	HTMLWidget hw = (HTMLWidget)w;
	char *text;

	if (hw->html.select_start == NULL)
	{
		return False;
	}

	if (*target == XA_TARGETS(d))
	{
		Atom *targetP;
		Atom *std_targets;
		unsigned long std_length;
		XmuConvertStandardSelection( w, hw->html.selection_time,
			selection, target, type, (caddr_t*)&std_targets,
			&std_length, format);

		*length = std_length + 5;
		*value = (caddr_t)XtMalloc(sizeof(Atom)*(*length));
		targetP = *(Atom**)value;
		*targetP++ = XA_STRING;
		*targetP++ = XA_TEXT(d);
		*targetP++ = XA_COMPOUND_TEXT(d);
		*targetP++ = XA_LENGTH(d);
		*targetP++ = XA_LIST_LENGTH(d);

		bcopy((char*)std_targets, (char*)targetP,
			sizeof(Atom)*std_length);
		XtFree((char*)std_targets);
		*type = XA_ATOM;
		*format = 32;
		return True;
	}

	if (*target == XA_STRING || *target == XA_TEXT(d) ||
		*target == XA_COMPOUND_TEXT(d))
	{
		if (*target == XA_COMPOUND_TEXT(d))
		{
			*type = *target;
		}
		else
		{
			*type = XA_STRING;
		}
		if (hw->html.fancy_selections == True)
		{
			text = ParseTextToPrettyString(hw,
				hw->html.formatted_elements,
				hw->html.select_start, hw->html.select_end,
				hw->html.sel_start_pos, hw->html.sel_end_pos,
				hw->html.font->max_bounds.width,
				hw->html.margin_width);
		}
		else
		{
			text = ParseTextToString(hw->html.formatted_elements,
				hw->html.select_start, hw->html.select_end,
				hw->html.sel_start_pos, hw->html.sel_end_pos,
				hw->html.font->max_bounds.width,
				hw->html.margin_width);
		}
		*value = text;
		*length = strlen(*value);
		*format = 8;
		return True;
	}

	if (*target == XA_LIST_LENGTH(d))
	{
		*value = XtMalloc(4);
		if (sizeof(long) == 4)
		{
			*(long*)*value = 1;
		}
		else
		{
			long temp = 1;
			bcopy( ((char*)&temp)+sizeof(long)-4, (char*)*value, 4);
		}
		*type = XA_INTEGER;
		*length = 1;
		*format = 32;
		return True;
	}

	if (*target == XA_LENGTH(d))
	{
		if (hw->html.fancy_selections == True)
		{
			text = ParseTextToPrettyString(hw,
				hw->html.formatted_elements,
				hw->html.select_start, hw->html.select_end,
				hw->html.sel_start_pos, hw->html.sel_end_pos,
				hw->html.font->max_bounds.width,
				hw->html.margin_width);
		}
		else
		{
			text = ParseTextToString(hw->html.formatted_elements,
				hw->html.select_start, hw->html.select_end,
				hw->html.sel_start_pos, hw->html.sel_end_pos,
				hw->html.font->max_bounds.width,
				hw->html.margin_width);
		}
		*value = XtMalloc(4);
		if (sizeof(long) == 4)
		{
			*(long*)*value = strlen(text);
		}
		else
		{
			long temp = strlen(text);
			bcopy( ((char*)&temp)+sizeof(long)-4, (char*)*value, 4);
		}
		free(text);
		*type = XA_INTEGER;
		*length = 1;
		*format = 32;
		return True;
	}

	if (XmuConvertStandardSelection(w, hw->html.selection_time, selection,
				    target, type, value, length, format))
	{
		return True;
	}

	return False;
}


static void
LoseSelection(w, selection)
	Widget w;
	Atom *selection;
{
	HTMLWidget hw = (HTMLWidget)w;

	ClearSelection(hw);
}


static void
SelectionDone(w, selection, target)
	Widget w;
	Atom *selection, *target;
{
	/* empty proc so Intrinsics know we want to keep storage */
}


/*
 *************************************************************************
 ******************************* PUBLIC FUNCTIONS ************************
 *************************************************************************
 */


/*
 * Convenience function to return the text of the HTML document as a plain
 * ascii text string.
 * This function allocates memory for the returned string, that it is up
 * to the user to free.
 * Extra option flags "pretty" text to be returned.
 */
char *
#ifdef _NO_PROTO
HTMLGetText (w, pretty)
	Widget w;
	int pretty;
#else
HTMLGetText(Widget w, int pretty)
#endif
{
	HTMLWidget hw = (HTMLWidget)w;
	int page;
	char *text;
	char *tptr, *buf;
	struct ele_rec *start;
	struct ele_rec *end;

	text = NULL;
	for (page = 1; page <= hw->html.page_cnt; page++)
	{
		start = hw->html.pages[(page - 1)].elist;
		end = start;
		while (end != NULL)
		{
			end = end->next;
		}

		if (pretty == 2)
		{
			tptr = ParseTextToPSString(hw, start, start, end,
					0, 0,
					hw->html.font->max_bounds.width,
					hw->html.margin_width);
		}
		else if (pretty)
		{
			tptr = ParseTextToPrettyString(hw, start, start, end,
					0, 0,
					hw->html.font->max_bounds.width,
					hw->html.margin_width);
		}
		else
		{
			tptr = ParseTextToString(start, start, end,
					0, 0,
					hw->html.font->max_bounds.width,
					hw->html.margin_width);
		}
		if (tptr != NULL)
		{
			if (text == NULL)
			{
				text = tptr;
			}
			else
			{
				buf = (char *)malloc(strlen(text) +
					strlen(tptr) + 1);
				strcpy(buf, text);
				strcat(buf, tptr);
				free(text);
				free(tptr);
				text = buf;
			}
		}
	}
	return(text);
}


/*
 * Convenience function to return the element id of the element
 * nearest to the x,y coordinates passed in.
 * If there is no element there, return the first element in the
 * line we are on.  If there we are on no line, either return the
 * beginning, or the end of the document.
 */
int
#ifdef _NO_PROTO
HTMLPositionToId(w, x, y)
	Widget w;
	int x, y;
#else
HTMLPositionToId(Widget w, int x, int y)
#endif
{
	HTMLWidget hw = (HTMLWidget)w;
	int i;
	int epos;
	struct ele_rec *eptr;

	eptr = LocateElement(hw, x, y, &epos);
	if (eptr == NULL)
	{
		eptr = hw->html.line_array[0];
		for (i=0; i<hw->html.line_count; i++)
		{
			if (hw->html.line_array[i] == NULL)
			{
				continue;
			}
			else if (hw->html.line_array[i]->y <= y)
			{
				eptr = hw->html.line_array[i];
				continue;
			}
			else
			{
				break;
			}
		}
	}
	if (eptr == NULL)
	{
		return(0);
	}
	else
	{
		return(eptr->ele_id);
	}
}


/*
 * Convenience function to return the position of the element
 * based on the element id passed in.
 * Function returns the page number and fills in x,y pixel values.
 * If there is no such element, 0,0 of page 1 is returned.
 */
int
#ifdef _NO_PROTO
HTMLIdToPosition(w, element_id, x, y)
	Widget w;
	int element_id;
	int *x, *y;
#else
HTMLIdToPosition(Widget w, int element_id, int *x, int *y)
#endif
{
	HTMLWidget hw = (HTMLWidget)w;
	int p;
	struct ele_rec *start;
	struct ele_rec *eptr;

	eptr = NULL;
	for (p = 1; p <= hw->html.page_cnt; p++)
	{
		start = hw->html.pages[(p - 1)].elist;
		while (start != NULL)
		{
			if (start->ele_id == element_id)
			{
				eptr = start;
				break;
			}
			start = start->next;
		}
		if (eptr != NULL)
		{
			break;
		}
	}
	if (eptr == NULL)
	{
		*x = 0;
		*y = 0;
		return(1);
	}
	else
	{
		*x = eptr->x;
		*y = eptr->y;
		return(p);
	}
}


/*
 * Convenience function to return the position of the anchor
 * based on the anchor NAME passed.
 * Function returns the page number and fills in x,y pixel values.
 * If there is no such element, 0,0 and page -1 is returned.
 */
int
#ifdef _NO_PROTO
HTMLAnchorToPosition(w, name, x, y)
	Widget w;
	char *name;
	int *x, *y;
#else
HTMLAnchorToPosition(Widget w, char *name, int *x, int *y)
#endif
{
	HTMLWidget hw = (HTMLWidget)w;
	int p;
	struct ele_rec *start;
	struct ele_rec *eptr;

	eptr = NULL;
	for (p = 1; p <= hw->html.page_cnt; p++)
	{
		start = hw->html.pages[(p - 1)].elist;
		while (start != NULL)
		{
			if ((start->anchorName)&&
                            (strcmp(start->anchorName, name) == 0))
			{
				eptr = start;
				break;
			}
			start = start->next;
		}
		if (eptr != NULL)
		{
			break;
		}
	}
	if (eptr == NULL)
	{
		*x = 0;
		*y = 0;
		return(-1);
	}
	else
	{
		*x = eptr->x;
		*y = eptr->y;
		return(p);
	}
}


/*
 * Convenience function to return the HREFs of all active anchors in the
 * document.
 * Function returns an array of strings and fills num_hrefs passed.
 * If there are no HREFs NULL returned.
 */
char **
#ifdef _NO_PROTO
HTMLGetHRefs(w, num_hrefs)
	Widget w;
	int *num_hrefs;
#else
HTMLGetHRefs(Widget w, int *num_hrefs)
#endif
{
	HTMLWidget hw = (HTMLWidget)w;
	int p, cnt;
	struct ele_rec *start;
	struct ele_rec *list;
	struct ele_rec *eptr;
	char **harray;

	list = NULL;
	cnt = 0;
	/*
	 * Construct a linked list of all the diffeent hrefs, counting
	 * then as we go.
	 */
	for (p = 1; p <= hw->html.page_cnt; p++)
	{
		start = hw->html.pages[(p - 1)].elist;
		/*
		 * loop through this whole page
		 */
		while (start != NULL)
		{
			/*
			 * This one has an HREF
			 */
			if (start->anchorHRef != NULL)
			{
				/*
				 * Check to see if we already have
				 * this HREF in our list.
				 */
				eptr = list;
				while (eptr != NULL)
				{
					if (strcmp(eptr->anchorHRef,
						start->anchorHRef) == 0)
					{
						break;
					}
					eptr = eptr->next;
				}
				/*
				 * This HREF is not, in our list.  Add it.
				 * That is, if it's not an internal reference.
				 */
				if ((eptr == NULL)&&(start->internal == False))
				{
					eptr = (struct ele_rec *)
						malloc(sizeof(struct ele_rec));
					eptr->anchorHRef = start->anchorHRef;
					eptr->next = list;
					list = eptr;
					cnt++;
				}
			}
			start = start->next;
		}
	}

	if (cnt == 0)
	{
		*num_hrefs = 0;
		return(NULL);
	}
	else
	{
		*num_hrefs = cnt;
		harray = (char **)malloc(sizeof(char *) * cnt);
		eptr = list;
		cnt--;
		while (eptr != NULL)
		{
			harray[cnt] = (char *)
				malloc(strlen(eptr->anchorHRef) + 1);
			strcpy(harray[cnt], eptr->anchorHRef);
			start = eptr;
			eptr = eptr->next;
			free((char *)start);
			cnt--;
		}
		return(harray);
	}
}


/*
 * Convenience function to redraw all active anchors in the
 * document.
 * Can also pass a new predicate function to check visited
 * anchors.  If NULL passed for function, uses default predicate
 * function.
 */
void
#ifdef _NO_PROTO
HTMLRetestAnchors(w, testFunc)
	Widget w;
	visitTestProc testFunc;
#else
HTMLRetestAnchors(Widget w, visitTestProc testFunc)
#endif
{
	HTMLWidget hw = (HTMLWidget)w;
	int p;
	struct ele_rec *start;

	if (testFunc == NULL)
	{
		testFunc = (visitTestProc)hw->html.previously_visited_test;
	}

	/*
	 * Search all elements on all pages.
	 */
	for (p = 1; p <= hw->html.page_cnt; p++)
	{
		start = hw->html.pages[(p - 1)].elist;
		/*
		 * loop through this whole page
		 */
		while (start != NULL)
		{
			if ((start->internal == True)||
			    (start->anchorHRef == NULL))
			{
				start = start->next;
				continue;
			}

			if (testFunc != NULL)
			{
				if ((*testFunc)(hw, start->anchorHRef))
				{
				    start->fg = hw->html.visitedAnchor_fg;
				    start->underline_number =
					hw->html.num_visitedAnchor_underlines;
				    start->dashed_underline =
					hw->html.dashed_visitedAnchor_lines;
				}
				else
				{
				    start->fg = hw->html.anchor_fg;
				    start->underline_number =
					hw->html.num_anchor_underlines;
				    start->dashed_underline =
					hw->html.dashed_anchor_lines;
				}
			}
			else
			{
				start->fg = hw->html.anchor_fg;
				start->underline_number =
					hw->html.num_anchor_underlines;
				start->dashed_underline =
					hw->html.dashed_anchor_lines;
			}

			/*
			 * Since the element may have changed, redraw it
			 * if it is on the current page.
			 */
			if (p == hw->html.current_page)
			{
				switch(start->type)
				{
					case E_TEXT:
						TextRefresh(hw, start,
						     0, (start->edata_len - 1));
						break;
					case E_IMAGE:
						ImageRefresh(hw, start);
						break;
					case E_BULLET:
						BulletRefresh(hw, start);
						break;
					case E_LINEFEED:
						LinefeedRefresh(hw, start);
						break;
				}
			}

			start = start->next;
		}
	}
}


void
#ifdef _NO_PROTO
HTMLClearSelection (w)
	Widget w;
#else
HTMLClearSelection(Widget w)
#endif
{
	LoseSelection (w, NULL);
}


/*
 * Set the current selection based on the ElementRefs passed in.
 * Both refs must be valid, and be on the same page currently being
 * displayed.
 */
void
#ifdef _NO_PROTO
HTMLSetSelection (w, start, end)
	Widget w;
	ElementRef *start;
	ElementRef *end;
#else
HTMLSetSelection(Widget w, ElementRef *start, ElementRef *end)
#endif
{
	HTMLWidget hw = (HTMLWidget)w;
	int page;
	int found;
	struct ele_rec *eptr;
	struct ele_rec *e_start;
	struct ele_rec *e_end;
	int start_pos, end_pos;
	Atom *atoms;
	int i, buffer;
	char *text;
	char *params[2];

	/*
	 * If the starting position is not valid, fail the selection
	 */
	if ((start->id > 0)&&(start->pos >= 0))
	{
		found = 0;
		page = hw->html.current_page;
		eptr = hw->html.pages[(page - 1)].elist;

		while (eptr != NULL)
		{
			if (eptr->ele_id == start->id)
			{
				e_start = eptr;
				start_pos = start->pos;
				found = 1;
				break;
			}
			eptr = eptr->next;
		}
		if (!found)
		{
			return;
		}
	}

	/*
	 * If the ending position is not valid, fail the selection
	 */
	if ((end->id > 0)&&(end->pos >= 0))
	{
		found = 0;
		page = hw->html.current_page;
		eptr = hw->html.pages[(page - 1)].elist;

		while (eptr != NULL)
		{
			if (eptr->ele_id == end->id)
			{
				e_end = eptr;
				end_pos = end->pos;
				found = 1;
				break;
			}
			eptr = eptr->next;
		}
		if (!found)
		{
			return;
		}
	}

	LoseSelection (w, NULL);

	/*
	 * We expect the ElementRefs came from HTMLSearchText, so we know
	 * that the end_pos is one past what we want to select.
	 */
	end_pos = end_pos - 1;

	/*
	 * Sanify the position data
	 */
	if ((start_pos > 0)&&(start_pos >= e_start->edata_len))
	{
		start_pos = e_start->edata_len - 1;
	}
	if ((end_pos > 0)&&(end_pos >= e_end->edata_len))
	{
		end_pos = e_end->edata_len - 1;
	}

	hw->html.select_start = e_start;
	hw->html.sel_start_pos = start_pos;
	hw->html.select_end = e_end;
	hw->html.sel_end_pos = end_pos;
	SetSelection(hw);
	hw->html.new_start = NULL;
	hw->html.new_end = NULL;
	hw->html.new_start_pos = 0;
	hw->html.new_end_pos = 0;

	/*
	 * Do all the gunk form the end of the ExtendEnd function
	 */
	params[0] = "PRIMARY";
	params[1] = "CUT_BUFFER0";
	atoms = (Atom *)malloc(2 * sizeof(Atom));
	if (atoms == NULL)
	{
		fprintf(stderr, "cannot allocate atom list\n");
		return;
	}
	XmuInternStrings(XtDisplay((Widget)hw), params, 2, atoms);
	hw->html.selection_time = CurrentTime;
	for (i=0; i< 2; i++)
	{
		switch (atoms[i])
		{
			case XA_CUT_BUFFER0: buffer = 0; break;
			case XA_CUT_BUFFER1: buffer = 1; break;
			case XA_CUT_BUFFER2: buffer = 2; break;
			case XA_CUT_BUFFER3: buffer = 3; break;
			case XA_CUT_BUFFER4: buffer = 4; break;
			case XA_CUT_BUFFER5: buffer = 5; break;
			case XA_CUT_BUFFER6: buffer = 6; break;
			case XA_CUT_BUFFER7: buffer = 7; break;
			default: buffer = -1; break;
		}
		if (buffer >= 0)
		{
			if (hw->html.fancy_selections == True)
			{
			    text = ParseTextToPrettyString(hw,
				hw->html.formatted_elements,
				hw->html.select_start,
				hw->html.select_end,
				hw->html.sel_start_pos,
				hw->html.sel_end_pos,
				hw->html.font->max_bounds.width,
				hw->html.margin_width);
			}
			else
			{
			    text = ParseTextToString(
				hw->html.formatted_elements,
				hw->html.select_start,
				hw->html.select_end,
				hw->html.sel_start_pos,
				hw->html.sel_end_pos,
				hw->html.font->max_bounds.width,
				hw->html.margin_width);
			}
			XStoreBuffer(XtDisplay((Widget)hw),
				text, strlen(text), buffer);
			free(text);
		}
		else
		{
			XtOwnSelection((Widget)hw, atoms[i],
				CurrentTime, ConvertSelection,
				LoseSelection, SelectionDone);
		}
	}
	free((char *)atoms);
}


/*
 * Convenience function to return the text of the HTML document as a single
 * white space separated string, with pointers to the various start and
 * end points of selections.
 * This function allocates memory for the returned string, that it is up
 * to the user to free.
 */
char *
#ifdef _NO_PROTO
HTMLGetTextAndSelection (w, startp, endp, insertp)
	Widget w;
	char **startp;
	char **endp;
	char **insertp;
#else
HTMLGetTextAndSelection(Widget w, char **startp, char **endp, char **insertp)
#endif
{
	HTMLWidget hw = (HTMLWidget)w;
	int page;
	int length;
	char *text;
	char *tptr;
	struct ele_rec *eptr;
	struct ele_rec *sel_start;
	struct ele_rec *sel_end;
	struct ele_rec *insert_start;
	int start_pos, end_pos, insert_pos;

	if (SwapElements(hw->html.select_start, hw->html.select_end,
		hw->html.sel_start_pos, hw->html.sel_end_pos))
	{
		sel_end = hw->html.select_start;
		end_pos = hw->html.sel_start_pos;
		sel_start = hw->html.select_end;
		start_pos = hw->html.sel_end_pos;
	}
	else
	{
		sel_start = hw->html.select_start;
		start_pos = hw->html.sel_start_pos;
		sel_end = hw->html.select_end;
		end_pos = hw->html.sel_end_pos;
	}

	insert_start = hw->html.new_start;
	insert_pos = hw->html.new_start_pos;
	*startp = NULL;
	*endp = NULL;
	*insertp = NULL;

	length = 0;
	for (page = 1; page <= hw->html.page_cnt; page++)
	{
		eptr = hw->html.pages[(page - 1)].elist;

		while (eptr != NULL)
		{
			/*
			 * Skip the special internal text added for multi-page
			 * documents.
			 */
			if (eptr->internal == True)
			{
				eptr = eptr->next;
				continue;
			}

			if (eptr->type == E_TEXT)
			{
				length = length + eptr->edata_len;
			}
			else if (eptr->type == E_LINEFEED)
			{
				length = length + 1;
			}
			eptr = eptr->next;
		}
	}

	text = (char *)malloc(length + 1);
	if (text == NULL)
	{
		fprintf(stderr, "No space for return string\n");
		return(NULL);
	}
	strcpy(text, "");

	tptr = text;
	for (page = 1; page <= hw->html.page_cnt; page++)
	{
		eptr = hw->html.pages[(page - 1)].elist;

		while (eptr != NULL)
		{
			/*
			 * Skip the special internal text added for multi-page
			 * documents.
			 */
			if (eptr->internal == True)
			{
				eptr = eptr->next;
				continue;
			}

			if (eptr->type == E_TEXT)
			{
				if (eptr == sel_start)
				{
					*startp = (char *)(tptr + start_pos);
				}

				if (eptr == sel_end)
				{
					*endp = (char *)(tptr + end_pos);
				}

				if (eptr == insert_start)
				{
					*insertp = (char *)(tptr + insert_pos);
				}

				strcat(text, (char *)eptr->edata);
				tptr = tptr + eptr->edata_len;
			}
			else if (eptr->type == E_LINEFEED)
			{
				if (eptr == sel_start)
				{
					*startp = tptr;
				}

				if (eptr == sel_end)
				{
					*endp = tptr;
				}

				if (eptr == insert_start)
				{
					*insertp = tptr;
				}

				strcat(text, " ");
				tptr = tptr + 1;
			}
			eptr = eptr->next;
		}
	}
	return(text);
}


/*
 * Convenience function to set the raw text into the widget.
 * Forces a reparse and a reformat.
 * If any pointer is passed in as NULL that text is unchanged,
 * if a pointer points to an empty string, that text is set to NULL;
 */
void
#ifdef _NO_PROTO
HTMLSetText (w, text, header_text, footer_text)
	Widget w;
	char *text;
	char *header_text;
	char *footer_text;
#else
HTMLSetText(Widget w, char *text, char *header_text, char *footer_text)
#endif
{
	HTMLWidget hw = (HTMLWidget)w;

	if ((text == NULL)&&(header_text == NULL)&&(footer_text == NULL))
	{
		return;
	}

	/*
	 * Free up the old visited href list.
	 */
	FreeHRefs(hw->html.my_visited_hrefs);
	hw->html.my_visited_hrefs = NULL;

	if (text != NULL)
	{
		if (*text == '\0')
		{
			text = NULL;
		}
		hw->html.raw_text = text;

		/*
		 * Free any old colors and pixmaps
		 */
		FreeColors(XtDisplay(hw),DefaultColormapOfScreen(XtScreen(hw)));
		FreeImages(hw);

		/*
		 * Parse the raw text with the HTML parser
		 */
		hw->html.html_objects = HTMLParse(hw->html.html_objects,
			hw->html.raw_text);
	}
	if (header_text != NULL)
	{
		if (*header_text == '\0')
		{
			header_text = NULL;
		}
		hw->html.header_text = header_text;

		/*
		 * Parse the header text with the HTML parser
		 */
		hw->html.html_header_objects =
			HTMLParse(hw->html.html_header_objects,
			hw->html.header_text);
	}
	if (footer_text != NULL)
	{
		if (*footer_text == '\0')
		{
			footer_text = NULL;
		}
		hw->html.footer_text = footer_text;

		/*
		 * Parse the footer text with the HTML parser
		 */
		hw->html.html_footer_objects =
			HTMLParse(hw->html.html_footer_objects,
			hw->html.footer_text);
	}

	/*
	 * Redisplay for the changed data.
	 * if auto-sizing, change window size.
	 */
	if (hw->html.auto_size == True)
	{
		int nwidth, nheight;

		nwidth = DocumentWidth(hw, hw->html.html_objects);
		hw->html.max_pre_width = nwidth;
		nwidth = nwidth + (2 * hw->html.margin_width);
		if (hw->core.width > nwidth)
		{
			nwidth = hw->core.width;
		}

		hw->html.format_width = 0;
		hw->html.format_height = 0;
		hw->html.reformat = True;
		Redisplay(hw, (XEvent *)NULL, (Region)NULL);
	}
	else
	{
		hw->html.format_width = 0;
		hw->html.format_height = 0;
		hw->html.reformat = True;
		Redisplay(hw, (XEvent *)NULL, (Region)NULL);
	}

	/*
	 * Clear any previous selection
	 */
	hw->html.select_start = NULL;
	hw->html.select_end = NULL;
	hw->html.sel_start_pos = 0;
	hw->html.sel_end_pos = 0;
	hw->html.new_start = NULL;
	hw->html.new_end = NULL;
	hw->html.new_start_pos = 0;
	hw->html.new_end_pos = 0;
	hw->html.active_anchor = NULL;
}


/*
 * To use faster TOLOWER as set up in HTMLparse.c
 */
#ifdef NOT_ASCII
#define TOLOWER(x)      (tolower(x))
#else
extern char map_table[];
#define TOLOWER(x)      (map_table[x])
#endif /* NOT_ASCII */


/*
 * Convenience function to search the text of the HTML document as a single
 * white space separated string. Linefeeds are converted into spaces.
 *
 * Takes a pattern, pointers to the start and end blocks to store the
 * start and end of the match into.  Start is also used as the location to
 * start the search from for incremental searching.  If start is an invalid
 * position (id = 0).  Default start is the beginning of the document for
 * forward searching, and the end of the document for backwards searching.
 * The backward and caseless parameters I hope are self-explanatory.
 *
 * returns page of match (starting at 1) if found
 *      (and the start and end positions of the match).
 * returns 0 otherwise (and start and end are unchanged).
 */
int
#ifdef _NO_PROTO
HTMLSearchText (w, pattern, m_start, m_end, backward, caseless)
	Widget w;
	char *pattern;
	ElementRef *m_start;
	ElementRef *m_end;
	int backward;
	int caseless;
#else
HTMLSearchText (Widget w, char *pattern, ElementRef *m_start, ElementRef *m_end,
		int backward, int caseless)
#endif
{
	HTMLWidget hw = (HTMLWidget)w;
	int page;
	int found, equal;
	char *match;
	char *tptr;
	char *mptr;
	char cval;
	struct ele_rec *eptr;
	int s_page, s_pos;
	int b_page;
	struct ele_rec *s_eptr;
	ElementRef s_ref, e_ref;
	ElementRef *start, *end;

	/*
	 * If bad parameters are passed, just fail the search
	 */
	if ((pattern == NULL)||(*pattern == '\0')||
		(m_start == NULL)||(m_end == NULL))
	{
		return(0);
	}

	/*
	 * If we are caseless, make a lower case copy of the pattern to
	 * match to use in compares.
	 *
	 * remember to free this before returning
	 */
	if (caseless)
	{
		match = (char *)malloc(strlen(pattern) + 1);
		tptr = pattern;
		mptr = match;
		while (*tptr != '\0')
		{
			*mptr = (char)TOLOWER((int)*tptr);
			mptr++;
			tptr++;
		}
		*mptr = '\0';
	}
	else
	{
		match = pattern;
	}

	/*
	 * Slimy coding.  I later decided I didn't want to change start and
	 * end if the search failed.  Rather than changing all the code,
	 * I just copy it into locals here, and copy it out again if a match
	 * is found.
	 */
	start = &s_ref;
	end = &e_ref;
	start->id = m_start->id;
	start->pos = m_start->pos;
	end->id = m_end->id;
	end->pos = m_end->pos;

	/*
	 * Find the user specified start position.
	 */
	if (start->id > 0)
	{
		found = 0;
		for (page = 1; page <= hw->html.page_cnt; page++)
		{
			eptr = hw->html.pages[(page - 1)].elist;

			while (eptr != NULL)
			{
				if (eptr->ele_id == start->id)
				{
					s_page = page;
					s_eptr = eptr;
					found = 1;
					break;
				}
				eptr = eptr->next;
			}
			if (found)
			{
				break;
			}
		}
		/*
		 * Bad start position, fail them out.
		 */
		if (!found)
		{
			if (caseless)
			{
				free(match);
			}
			return(0);
		}
		/*
		 * Sanify the start position
		 */
		s_pos = start->pos;
		if (s_pos >= s_eptr->edata_len)
		{
			s_pos = s_eptr->edata_len - 1;
		}
		if (s_pos < 0)
		{
			s_pos = 0;
		}
	}
	else
	{
		/*
		 * Default search starts at end for backward, and
		 * beginning for forwards.
		 */
		if (backward)
		{
			s_page = hw->html.page_cnt;
			s_eptr = hw->html.pages[(s_page - 1)].elist;
			while (s_eptr->next != NULL)
			{
				s_eptr = s_eptr->next;
			}
			s_pos = s_eptr->edata_len - 1;
		}
		else
		{
			s_page = 1;
			s_eptr = hw->html.pages[0].elist;
			s_pos = 0;
		}
	}

    if (backward)
    {
	char *mend;

	/*
	 * Save the end of match here for easy end to start searching
	 */
	mend = match;
	while (*mend != '\0')
	{
		mend++;
	}
	if (mend > match)
	{
		mend--;
	}
	found = 0;
	equal = 0;
	mptr = mend;
	for (page = s_page; page >= 1; page--)
	{
		if (page == s_page)
		{
			eptr = s_eptr;
		}
		else
		{
			eptr = hw->html.pages[(page - 1)].elist;
			while (eptr->next != NULL)
			{
				eptr = eptr->next;
			}
		}

		while (eptr != NULL)
		{
			/*
			 * Skip the special internal text added for multi-page
			 * documents.
			 */
			if (eptr->internal == True)
			{
				eptr = eptr->prev;
				continue;
			}

			if (eptr->type == E_TEXT)
			{
			    tptr = (char *)(eptr->edata + eptr->edata_len - 1);
			    if (eptr == s_eptr)
			    {
				tptr = (char *)(eptr->edata + s_pos);
			    }
			    while (tptr >= eptr->edata)
			    {
				if (equal)
				{
					if (caseless)
					{
						cval =(char)TOLOWER((int)*tptr);
					}
					else
					{
						cval = *tptr;
					}
					while ((mptr >= match)&&
						(tptr >= eptr->edata)&&
						(cval == *mptr))
					{
						tptr--;
						mptr--;
						if (caseless)
						{
						cval =(char)TOLOWER((int)*tptr);
						}
						else
						{
							cval = *tptr;
						}
					}
					if (mptr < match)
					{
						found = 1;
						b_page = page;
						start->id = eptr->ele_id;
						start->pos = (int)
						    (tptr - eptr->edata + 1);
						break;
					}
					else if (tptr < eptr->edata)
					{
						break;
					}
					else
					{
						equal = 0;
					}
				}
				else
				{
					mptr = mend;
					if (caseless)
					{
						cval =(char)TOLOWER((int)*tptr);
					}
					else
					{
						cval = *tptr;
					}
					while ((tptr >= eptr->edata)&&
						(cval != *mptr))
					{
						tptr--;
						if (caseless)
						{
						cval =(char)TOLOWER((int)*tptr);
						}
						else
						{
							cval = *tptr;
						}
					}
					if ((tptr >= eptr->edata)&&
						(cval == *mptr))
					{
						equal = 1;
						end->id = eptr->ele_id;
						end->pos = (int)
						    (tptr - eptr->edata + 1);
					}
				}
			    }
			}
			/*
			 * Linefeeds match to single space characters.
			 */
			else if (eptr->type == E_LINEFEED)
			{
				if (equal)
				{
					if (*mptr == ' ')
					{
						mptr--;
						if (mptr < match)
						{
							found = 1;
							b_page = page;
							start->id =eptr->ele_id;
							start->pos = 0;
						}
					}
					else
					{
						equal = 0;
					}
				}
				else
				{
					mptr = mend;
					if (*mptr == ' ')
					{
						equal = 1;
						end->id = eptr->ele_id;
						end->pos = 0;
						mptr--;
						if (mptr < match)
						{
							found = 1;
							b_page = page;
							start->id =eptr->ele_id;
							start->pos = 0;
						}
					}
				}
			}
			if (found)
			{
				break;
			}
			eptr = eptr->prev;
		}
		if (found)
		{
			break;
		}
	}
    }
    else
    {
	found = 0;
	equal = 0;
	mptr = match;
	for (page = s_page; page <= hw->html.page_cnt; page++)
	{
		if (page == s_page)
		{
			eptr = s_eptr;
		}
		else
		{
			eptr = hw->html.pages[(page - 1)].elist;
		}

		while (eptr != NULL)
		{
			/*
			 * Skip the special internal text added for multi-page
			 * documents.
			 */
			if (eptr->internal == True)
			{
				eptr = eptr->next;
				continue;
			}

			if (eptr->type == E_TEXT)
			{
			    tptr = eptr->edata;
			    if (eptr == s_eptr)
			    {
				tptr = (char *)(tptr + s_pos);
			    }
			    while (*tptr != '\0')
			    {
				if (equal)
				{
					if (caseless)
					{
						cval =(char)TOLOWER((int)*tptr);
					}
					else
					{
						cval = *tptr;
					}
					while ((*mptr != '\0')&&
						(cval == *mptr))
					{
						tptr++;
						mptr++;
						if (caseless)
						{
						cval =(char)TOLOWER((int)*tptr);
						}
						else
						{
							cval = *tptr;
						}
					}
					if (*mptr == '\0')
					{
						found = 1;
						b_page = page;
						end->id = eptr->ele_id;
						end->pos = (int)
							(tptr - eptr->edata);
						break;
					}
					else if (*tptr == '\0')
					{
						break;
					}
					else
					{
						equal = 0;
					}
				}
				else
				{
					mptr = match;
					if (caseless)
					{
						cval =(char)TOLOWER((int)*tptr);
					}
					else
					{
						cval = *tptr;
					}
					while ((*tptr != '\0')&&
						(cval != *mptr))
					{
						tptr++;
						if (caseless)
						{
						cval =(char)TOLOWER((int)*tptr);
						}
						else
						{
							cval = *tptr;
						}
					}
					if (cval == *mptr)
					{
						equal = 1;
						start->id = eptr->ele_id;
						start->pos = (int)
							(tptr - eptr->edata);
					}
				}
			    }
			}
			else if (eptr->type == E_LINEFEED)
			{
				if (equal)
				{
					if (*mptr == ' ')
					{
						mptr++;
						if (*mptr == '\0')
						{
							found = 1;
							b_page = page;
							end->id = eptr->ele_id;
							end->pos = 0;
						}
					}
					else
					{
						equal = 0;
					}
				}
				else
				{
					mptr = match;
					if (*mptr == ' ')
					{
						equal = 1;
						start->id = eptr->ele_id;
						start->pos = 0;
						mptr++;
						if (*mptr == '\0')
						{
							found = 1;
							b_page = page;
							end->id = eptr->ele_id;
							end->pos = 0;
						}
					}
				}
			}
			if (found)
			{
				break;
			}
			eptr = eptr->next;
		}
		if (found)
		{
			break;
		}
	}
    }

	if (found)
	{
		m_start->id = start->id;
		m_start->pos = start->pos;
		m_end->id = end->id;
		m_end->pos = end->pos;
		found = b_page;
	}

	if (caseless)
	{
		free(match);
	}

	return(found);
}

