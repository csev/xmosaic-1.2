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
#include "libwww/HTParse.h"
#include <Xm/SelectioB.h>
#include <Xm/FileSB.h>

#ifdef HAVE_DMF
/*
 * Interface between Mosaic and DMF lives here.
 */

#include <libdtm/dtm.h>
#include <libdmf/dmf.h>

extern Widget toplevel;

#define DMF_ID		"XMosaic"

/*
 * DMF Message Classes
 */
#define	MC_UNKNOWN		0
#define	MC_EXEC			1
#define MC_CREQ			2
#define	MC_DATA			3
#define	MC_DBASE		4
#define	MC_CONFIRM		5
#define	MC_QUERY		6
#define	MC_MATCH		7
#define MC_INQUIRE		8
#define	MC_INFORM		9
#define	MC_INFO			10
#define	MC_REQUEST		11
#define MC_OPTION		12
#define	MC_TOGGLE		13
#define MC_FILE			14
#define MC_COMPLETE		15


struct file_rec {
	char *name;
	char *data;
	int format;
	int length;
};


static Widget dialog = NULL;
static Widget save_dialog = NULL;
static int dialog_prepared = 0;
static int save_dialog_prepared = 0;

static char *CurrentAddress = NULL;
static char *CurrentDatabase = NULL;
static char *CurrentText = NULL;
static QFIELD *CurrentQuery = NULL;
static char *QueryRefText = NULL;
static mo_window *QueryWin = NULL;
static int QueryData = 0;
static mo_window *ItemWin = NULL;



static char *
readit(int in_port, int *typep)
{
	char header[DTM_MAX_HEADER];
	char *h;
	int i, len, type;

	for (i=0; i<DTM_MAX_HEADER; i++)
	{
		header[i] = '\0';
	}

	len = DTMbeginRead(in_port, header, DTM_MAX_HEADER);
	if (len == DTMERROR)
	{
		fprintf(stderr, "Error reading DTM header\n");
		return(NULL);
	}
	if (EXECcompareClass(header))
	{
		type = MC_EXEC;
	}
	else if (CREQcompareClass(header))
	{
		type = MC_CREQ;
	}
	else if (DATAcompareClass(header))
	{
		type = MC_DATA;
	}
	else if (DBASEcompareClass(header))
	{
		type = MC_DBASE;
	}
	else if (CONFIRMcompareClass(header))
	{
		type = MC_CONFIRM;
	}
	else if (QUERYcompareClass(header))
	{
		type = MC_QUERY;
	}
	else if (MATCHcompareClass(header))
	{
		type = MC_MATCH;
	}
	else if (INQUIREcompareClass(header))
	{
		type = MC_INQUIRE;
	}
	else if (INFORMcompareClass(header))
	{
		type = MC_INFORM;
	}
	else if (INFOcompareClass(header))
	{
		type = MC_INFO;
	}
	else if (REQUESTcompareClass(header))
	{
		type = MC_REQUEST;
	}
	else if (OPTIONcompareClass(header))
	{
		type = MC_OPTION;
	}
	else if (TOGGLEcompareClass(header))
	{
		type = MC_TOGGLE;
	}
	else if (FILEcompareClass(header))
	{
		type = MC_FILE;
	}
	else
	{
		type = MC_UNKNOWN;
	}

	*typep = type;

	h = (char *)malloc(len + 1);
	if (h == NULL)
	{
		fprintf(stderr, "Cannot malloc space for header\n");
		return(header);
	}
	strncpy(h, header, len + 1);
	h[len] = '\0';
	return(h);
}


static int
awaitDMFReply(int in_port, char **header, int type)
{
	Dtm_set s[64];
	int ret_type;
	char *h;

	h = NULL;
	ret_type = MC_UNKNOWN;
	/*
	 * MC_INFO is DMF's idea of returning an error message
	 */
	while ((ret_type != type)&&(ret_type != MC_INFO))
	{
		if (h != NULL)
		{
			DTMendRead(in_port);
			free(h);
			h = NULL;
		}
		s[0].port = in_port;
		s[0].status = ~DTM_PORT_READY;
		while (s[0].status != DTM_PORT_READY)
		{
			if (DTMERROR == DTMselectRead(s, 1, 0, 0, 1000000))
			{
				fprintf(stderr,"Error waiting for DTM input\n");
				*header = NULL;
				return(MC_UNKNOWN);
			}
		}
		h = readit(in_port, &ret_type);
#ifdef DEBUG
fprintf(stderr, "Got type %d\n", ret_type);
#endif
	}
	*header = h;
	return(ret_type);
}


static int
dmf_connect (char *addr, char *db_name, char **my_addr)
{
	int in_port;
	int out_port;
	char port_name[100];

	in_port = DTMmakeInPort(":0", DTM_SYNC);
	DTMgetPortAddr(in_port, port_name, 100);
	*my_addr = (char *)malloc(strlen(port_name) + 1);
	strcpy(*my_addr, port_name);
	out_port = DTMmakeOutPort(addr, DTM_SYNC);
	ESERVsendDbaseReq(out_port, port_name, db_name, DMF_ID);
	DTMdestroyPort(out_port);

	return(in_port);
}


static void
dmf_shutdown (int in_port, int out_port)
{
	DMFsendComplete(out_port);
	DTMdestroyPort(out_port);
	DTMdestroyPort(in_port);
}


/*
 * Construct a Hypertext page from the current Query list.
 */
static char *
GetCurrentQuery()
{
	int len, qlen;
	char *ret_text;
	char *qtext;
	char *ptr;
	QFIELD *qptr;

	/* text paragraph length */
	len = strlen(CurrentText) + strlen("<P>\n");

	/* plus SEARCH text length */
	len = len + strlen("When you have set the constraints you want, click on the highlighted ") +
		strlen("INITIATE SEARCH") +
		strlen(" to send the search request to the database server.<P>\n");

	/* plus fields header length */
	len = len + strlen("<H2>Query Fields</H2>\n");

	/* plus description list */
	len = len + strlen("<DL>\n</DL>\n");

	qlen = 0;
	qptr = CurrentQuery;
	while (qptr != NULL)
	{
		/* plus description title */
		len = len + strlen("<DT>\n") + strlen(qptr->label);

		/* plus anchor */
		len = len + strlen("<A HREF=\"dmffield://") +
			strlen(CurrentAddress) + 1 +
			strlen(CurrentDatabase) + 1 +
			strlen(qptr->label) + strlen("\"></A>");

		/* plus data value */
		if (qptr->data != NULL)
		{
			len = len + strlen(" = ") + strlen(qptr->data);

			/*
			 * Construct the query
			 */
			if (qlen != 0)
			{
				qlen = qlen + 1;
			}
			qlen = qlen + strlen(qptr->label) + 1 +
				strlen(qptr->data);
			ptr = qptr->label;
			while (*ptr != '\0')
			{
				if (*ptr == ' ')
				{
					qlen += 2;
				}
				ptr++;
			}
			ptr = qptr->data;
			while (*ptr != '\0')
			{
				if (*ptr == ' ')
				{
					qlen += 2;
				}
				ptr++;
			}
		}

		/* plus description text */
		len = len + strlen("<DD>\n") + strlen(qptr->help);

		qptr = qptr->next;
	}

	/* plus query paragraph length */
	len = len + strlen("Fill in this form and then go ");

	/* plus query anchor if any */
	if (qlen != 0)
	{
		len = len + strlen("<A HREF=\"dmfquery://") +
			strlen(CurrentAddress) + 1 +
			strlen(CurrentDatabase) + 1 + qlen +
			strlen("\"></A>");
	}

	/* plus ending paragraph */
	len = len + strlen("<P>\n");


	ret_text = (char *)malloc(len + 1);
	qtext = (char *)malloc(qlen + 1);

	/* plus text paragraph */
	strcpy(ret_text, CurrentText);
	strcat(ret_text, "<P>\n");

	/*
	 * Construct the query field to use as an anchor here
	 */
	qtext[0] = '\0';
	qptr = CurrentQuery;
	while (qptr != NULL)
	{
		/* query data */
		if (qptr->data != NULL)
		{
			char *p2;

			/*
			 * Construct the query
			 */
			if (qtext[0] != '\0')
			{
				strcat(qtext, "&");
			}

			/*
			 * p2 at end of qtext
			 */
			p2 = qtext;
			while (*p2 != '\0')
			{
				p2++;
			}

			/*
			 * append label=data, replacing spaces
			 */
			ptr = qptr->label;
			while (*ptr != '\0')
			{
				if (*ptr == ' ')
				{
					*p2++ = '%';
					*p2++ = '2';
					*p2++ = '0';
					ptr++;
				}
				else
				{
					*p2++ = *ptr++;
				}
			}
			*p2++ = '=';
			ptr = qptr->data;
			while (*ptr != '\0')
			{
				if (*ptr == ' ')
				{
					*p2++ = '%';
					*p2++ = '2';
					*p2++ = '0';
					ptr++;
				}
				else
				{
					*p2++ = *ptr++;
				}
			}
			*p2 = '\0';
		}

		qptr = qptr->next;
	}

	/* plus SEARCH text */
	strcat(ret_text, "When you have set the constraints you want, click on the highlighted ");

	if (qtext[0] != '\0')
	{
		strcat(ret_text, "<A HREF=\"dmfquery://");
		strcat(ret_text, CurrentAddress);
		strcat(ret_text, "/");
		strcat(ret_text, CurrentDatabase);
		strcat(ret_text, "/");
		strcat(ret_text, qtext);
		strcat(ret_text, "\">");

		/* plus anchor text */
		strcat(ret_text, "INITIATE SEARCH");
		strcat(ret_text, "</A>");
	}
	else
	{
		strcat(ret_text, "INITIATE SEARCH");
	}

	strcat(ret_text, " to send the search request to the database server.<P>\n");

	/* plus fields header */
	strcat(ret_text, "<H2>Query Fields</H2>\n");

	/* plus description list */
	strcat(ret_text, "<DL>\n");

	qtext[0] = '\0';
	qptr = CurrentQuery;
	while (qptr != NULL)
	{
		/* plus description title and anchor */
		strcat(ret_text, "<DT>");
		strcat(ret_text, "<A HREF=\"dmffield://");
		strcat(ret_text, CurrentAddress);
		strcat(ret_text, "/");
		strcat(ret_text, CurrentDatabase);
		strcat(ret_text, "/");
		strcat(ret_text, qptr->label);
		strcat(ret_text, "\">");

		/* plus anchor text */
		strcat(ret_text, qptr->label);
		strcat(ret_text, "</A>");

		/* plus data value */
		if (qptr->data != NULL)
		{
			char *p2;

			strcat(ret_text, " = ");
			strcat(ret_text, qptr->data);

			/*
			 * Construct the query
			 */
			if (qtext[0] != '\0')
			{
				strcat(qtext, "&");
			}

			/*
			 * p2 at end of qtext
			 */
			p2 = qtext;
			while (*p2 != '\0')
			{
				p2++;
			}

			/*
			 * append label=data, replacing spaces
			 */
			ptr = qptr->label;
			while (*ptr != '\0')
			{
				if (*ptr == ' ')
				{
					*p2++ = '%';
					*p2++ = '2';
					*p2++ = '0';
					ptr++;
				}
				else
				{
					*p2++ = *ptr++;
				}
			}
			*p2++ = '=';
			ptr = qptr->data;
			while (*ptr != '\0')
			{
				if (*ptr == ' ')
				{
					*p2++ = '%';
					*p2++ = '2';
					*p2++ = '0';
					ptr++;
				}
				else
				{
					*p2++ = *ptr++;
				}
			}
			*p2 = '\0';
		}
		strcat(ret_text, "\n");

		/* plus description text */
		strcat(ret_text, "<DD>");
		strcat(ret_text, qptr->help);
		strcat(ret_text, "\n");

		qptr = qptr->next;
	}

	/* plus end description list */
	strcat(ret_text, "</DL>\n");

	/* plus end paragraph */
	strcat(ret_text, "<P>\n");

	return(ret_text);
}


char *
DMFAnchor (char *tag, char *addr, char *path)
{
	int i, len, ret;
	int in_port, out_port;
	char *header;
	char *my_addr;
	char *ret_addr;
	char *text;
	char *help;
	char *ret_text;
	QFIELD *qfields;
	QFIELD *qptr;

#ifdef DEBUG
	fprintf(stderr, "DMFAnchor (%s, %s, %s) called\n", tag, addr, path);
#endif

	in_port = dmf_connect(addr, path, &my_addr);

	ret = awaitDMFReply(in_port, &header, MC_CONFIRM);
	if (ret == MC_UNKNOWN)
	{
		return(NULL);
	}
	/*
	 * MC_INFO is DMF's idea of an error return
	 */
	else if (ret == MC_INFO)
	{
		DMFrecvInfo(in_port, header, &text);
		DTMendRead(in_port);
		free(header);
		return(text);
	}
	qfields = NULL;
        DMFrecvConfirm(in_port, header, &ret_addr, &text, &qfields, &help);
        DTMendRead(in_port);
	free(header);

	/*
	 * Set up the locally global variable for this database.
	 */
	if (CurrentAddress != NULL)
	{
		free(CurrentAddress);
	}
	CurrentAddress = (char *)malloc(strlen(addr) + 1);
	strcpy(CurrentAddress, addr);
	if (CurrentDatabase != NULL)
	{
		free(CurrentDatabase);
	}
	CurrentDatabase = (char *)malloc(strlen(path) + 1);
	strcpy(CurrentDatabase, path);
	if (CurrentText != NULL)
	{
		free(CurrentText);
	}
	CurrentText = (char *)malloc(strlen(text) + 1);
	strcpy(CurrentText, text);
	if (CurrentQuery != NULL)
	{
		DMFfreeFields(CurrentQuery);
	}
	CurrentQuery = qfields;

	out_port = DTMmakeOutPort(ret_addr, DTM_SYNC);
	dmf_shutdown(in_port, out_port);

	qptr = qfields;
	while (qptr != NULL)
	{
		/* Free up the data field so we can fill it in later */
		if (qptr->data != NULL)
		{
			free((char *)qptr->data);
			qptr->data = NULL;
		}

		qptr = qptr->next;
	}

	ret_text = GetCurrentQuery();

	return(ret_text);
}


char *
DMFField (char *tag, char *addr, char *path)
{
	char *db_name;
	char *field;
	char *dtype;
	char *text;
	char *ptr;

#ifdef DEBUG
	fprintf(stderr, "DMFField (%s, %s, %s) called\n", tag, addr, path);
#endif

	db_name = path;
	ptr = strchr(db_name, '/');
	/* No field */
	if (ptr == NULL)
	{
		return(NULL);
	}
	*ptr = '\0';
	ptr++;
	field = ptr;

	ptr = strchr(field, '/');

	if (ptr == NULL)
	{
		dtype = NULL;
	}
	else
	{
		*ptr = '\0';
		ptr++;
		dtype = ptr;
	}

	/*
	 * If the current database is not this database we have a problem.
	 */
	if ((CurrentDatabase == NULL)||(strcmp(CurrentDatabase, db_name) != 0)||
		(CurrentQuery == NULL))
	{
		return(NULL);
	}

	/*
	 * All the work was done long ago when the user hit OK on the
	 * enter field data dialog.
	 */
	text = GetCurrentQuery();
	return(text);
}


/*
 * Construct a Hypertext document from the matches array
 */
static char *
MatchesToText(char *query, char *mtitle, MATCH **matches, int mcnt)
{
	int i, len;
	char *ret_text;

	/*
	 * If DMF passed us no title, make one up.
	 */
	if (mtitle == NULL)
	{
		mtitle = (char *)malloc(strlen("Matches") + 1);
		strcpy(mtitle, "Matches");
	}

	/* Length of query as header */
	len = strlen("<H3>Results of query ()</H3>\n") + strlen(query);

	/* Length of title as header */
	len = len + strlen("<H1></H1>\n") + strlen(mtitle);

	/* preformatted menu of items */
	len = len + strlen("<MENU>\n<PRE></PRE></MENU>\n");

	for (i=0; i<mcnt; i++)
	{
		char valstr[20];

		/* length of list item */
		len = len + strlen("<LI>\n") + strlen(matches[i]->matchblob);

		/* length of anchor */
		sprintf(valstr, "%d", matches[i]->keyid);
		len = len + strlen("<A HREF=\"dmfinquire://") +
			strlen(CurrentAddress) + 1 +
			strlen(CurrentDatabase) + 1 + strlen(valstr) + 1 +
			strlen("FULLINFO\"></A>");

#ifdef SHOW_ITEM_TYPE
		/* length of item type */
		len = len + strlen(" [Type=]\n") +
			strlen(matches[i]->datatype);
#else
		len = len + strlen("\n");
#endif /* SHOW_ITEM_TYPE */
	}

	if (mcnt == 0)
	{
		len = len + strlen("<H3>NO MATCHES FOUND</H3>\n");
	}

	ret_text = (char *)malloc(len + 1);
	if (ret_text == NULL)
	{
		return(NULL);
	}

	/* query as header */
	strcpy(ret_text, "<H3>Results of query (");
	strcat(ret_text, query);
	strcat(ret_text, ")</H3>\n");

	/* title as header */
	strcat(ret_text, "<H1>");
	strcat(ret_text, mtitle);
	strcat(ret_text, "</H1>\n");

	/* preformatted menu of items */
	strcat(ret_text, "<MENU>\n<PRE>");

	for (i=0; i<mcnt; i++)
	{
		char valstr[20];

		/* list item */
		strcat(ret_text, "<LI>");

		/* anchor */
		sprintf(valstr, "%d", matches[i]->keyid);
		strcat(ret_text, "<A HREF=\"dmfinquire://");
		strcat(ret_text, CurrentAddress);
		strcat(ret_text, "/");
		strcat(ret_text, CurrentDatabase);
		strcat(ret_text, "/");
		strcat(ret_text, valstr);
		strcat(ret_text, "/FULLINFO");
		strcat(ret_text, "\">");

		/* plus anchor text */
		strcat(ret_text, matches[i]->matchblob);
		strcat(ret_text, "</A>");

#ifdef SHOW_ITEM_TYPE
		/* item type */
		strcat(ret_text, " [Type=");
		strcat(ret_text, matches[i]->datatype);
		strcat(ret_text, "]");
#endif /* SHOW_ITEM_TYPE */
		strcat(ret_text, "\n");
	}

	/* menu of items */
	strcat(ret_text, "</PRE></MENU>\n");

	if (mcnt == 0)
	{
		strcat(ret_text, "<H3>NO MATCHES FOUND</H3>\n");
	}

	return(ret_text);
}


/*
 * Construct a Hypertext document from the full info returned.
 */
static char *
InfoToText(int key_id, char *title, char *text)
{
	int len;
	char *ret_text;
	char *ptr, *dtype;
	char valstr[20];

	/*
	 * convert key_id to string, and locate data type if possible.
	 */
	sprintf(valstr, "%d", key_id);
	dtype = NULL;
	ptr = text;
	while ((ptr != NULL)&&(*ptr != '\0'))
	{
		if ((*ptr == 'D')&&(strncmp(ptr, "DATATYPE", 8) == 0))
		{
			char *start, *end, tchar;

			ptr = (char *)(ptr + 8);
			while (isspace((int)*ptr))
			{
				ptr++;
			}
			start = ptr;
			end = start;
			while ((*end != '\0')&&(!isspace((int)*end)))
			{
				end++;
			}
			tchar = *end;
			*end = '\0';
			dtype = (char *)malloc(strlen(start) + 1);
			strcpy(dtype, start);
			*end = tchar;
			break;
		}
		ptr++;
	}
#ifdef DMF_BIMA_LOG_HACK
	/* for BIMA log files */
	if (dtype == NULL)
	{
		ptr = text;
		while ((ptr != NULL)&&(*ptr != '\0'))
		{
			if ((*ptr == 'P')&&(strncmp(ptr, "PROJECT", 7) == 0))
			{
				char *start, *end, tchar;

				ptr = (char *)(ptr + 7);
				while (isspace((int)*ptr))
				{
					ptr++;
				}
				start = ptr;
				end = start;
				while ((*end != '\0')&&(!isspace((int)*end)))
				{
					end++;
				}
				tchar = *end;
				*end = '\0';
				dtype = (char *)malloc(strlen(start) + 1);
				strcpy(dtype, start);
				*end = tchar;
				break;
			}
			ptr++;
		}
		if (strcmp(dtype, "log") == 0)
		{
			free(dtype);
			dtype = (char *)malloc(strlen("TEXT") + 1);
			strcpy(dtype, "TEXT");
		}
	}
#endif /* DMF_BIMA_LOG_HACK */

	/*
	 * If DMF passed us no title, make one up.
	 */
	if (title == NULL)
	{
		title = (char *)malloc(strlen("Full Information") + 1);
		strcpy(title, "Full Information");
	}

	/* Length of title as header */
	len = strlen(title) + 10;

	/* Length of pre */
	len = len + 13;

	/* Length of info text */
	len = len + strlen(text);

	/* Length of retrieve anchor */
	len = len + strlen("<H3><A HREF=\"dmfitem://") +
		strlen(CurrentAddress) + 1 + strlen(CurrentDatabase) + 1 +
		strlen(valstr) + 1 + strlen("FILE");
	if (dtype != NULL)
	{
		len = len + 1 + strlen(dtype);
	}
	if ((dtype != NULL)&&(strcmp(dtype, "TEXT") == 0))
	{
		len = len + strlen("\">View Full Text</A></H3>\n");
	}
	else
	{
		len = len + strlen("\">Retrieve Full Data</A></H3>\n");
	}

	ret_text = (char *)malloc(len + 1);
	if (ret_text == NULL)
	{
		return(NULL);
	}

	/* title as header */
	strcpy(ret_text, "<H1>");
	strcat(ret_text, title);
	strcat(ret_text, "</H1>\n");

	/* start pre */
	strcat(ret_text, "<PRE>\n");

	/* info text */
	strcat(ret_text, text);

	/* end pre */
	strcat(ret_text, "</PRE>\n");

	/* retrieve anchor */
	strcat(ret_text, "<H3><A HREF=\"dmfitem://");
	strcat(ret_text, CurrentAddress);
	strcat(ret_text, "/");
	strcat(ret_text, CurrentDatabase);
	strcat(ret_text, "/");
	strcat(ret_text, valstr);
	strcat(ret_text, "/FILE");
	if (dtype != NULL)
	{
		strcat(ret_text, "/");
		strcat(ret_text, dtype);
	}
	if ((dtype != NULL)&&(strcmp(dtype, "TEXT") == 0))
	{
		strcat(ret_text, "\">View Full Text</A></H3>\n");
	}
	else
	{
		strcat(ret_text, "\">Retrieve Full Data</A></H3>\n");
	}

	return(ret_text);
}


static char *
QueryConvert(char *query)
{
	char *new;
	char *ptr;
	char *nptr;
	int len;

	if (query == NULL)
	{
		return(NULL);
	}

	len = 0;
	ptr = query;
	while (*ptr != '\0')
	{
		if (*ptr == '&')
		{
			len += 4;
		}
		else if (*ptr == '|')
		{
			len += 3;
		}

		ptr++;
		len++;
	}

	new = (char *)malloc(len + 1);
	if (new == NULL)
	{
		return(query);
	}

	ptr = query;
	nptr = new;
	while (*ptr != '\0')
	{
		if (*ptr == '=')
		{
			*nptr++ = ' ';
		}
		else if (*ptr == '%')
		{
			int val;

			ptr++;
			sscanf(ptr, "%2x", &val);
			*nptr++ = (char)val;
			ptr++;
		}
		else if (*ptr == '&')
		{
			*nptr++ = ' ';
			*nptr++ = 'A';
			*nptr++ = 'N';
			*nptr++ = 'D';
			*nptr++ = ' ';
		}
		else if (*ptr == '|')
		{
			*nptr++ = ' ';
			*nptr++ = 'O';
			*nptr++ = 'R';
			*nptr++ = ' ';
		}
		else
		{
			*nptr++ = *ptr;
		}
		ptr++;
	}
	*nptr = '\0';

	return(new);
}


char *
DMFQuery (char *tag, char *addr, char *path)
{
	int in_port, out_port;
	char *db_name;
	char *query;
	char *ptr;
	char *header;
	char *my_addr;
	char *ret_addr;
	char *text;
	char *help;
	QFIELD *qfields;
	int ret, mcnt;
	char *mtitle;
	MATCH **matches;


#ifdef DEBUG
	fprintf(stderr, "DMFQuery (%s, %s, %s) called\n", tag, addr, path);
#endif

	db_name = path;
	ptr = strchr(db_name, '/');
	/* No query */
	if (ptr == NULL)
	{
		return(NULL);
	}
	*ptr = '\0';
	ptr++;
	query = ptr;

	in_port = dmf_connect(addr, db_name, &my_addr);

	ret = awaitDMFReply(in_port, &header, MC_CONFIRM);
	if (ret == MC_UNKNOWN)
	{
		return(NULL);
	}
	/*
	 * MC_INFO is DMF's idea of an error return
	 */
	else if (ret == MC_INFO)
	{
		DMFrecvInfo(in_port, header, &text);
		DTMendRead(in_port);
		free(header);
		return(text);
	}

	qfields = NULL;
        DMFrecvConfirm(in_port, header, &ret_addr, &text, &qfields, &help);
        DTMendRead(in_port);
	free(header);
	DMFfreeFields(qfields);

	/*
	 * Set up the locally global variable for this database.
	 */
	if (CurrentAddress != NULL)
	{
		free(CurrentAddress);
	}
	CurrentAddress = (char *)malloc(strlen(addr) + 1);
	strcpy(CurrentAddress, addr);
	if (CurrentDatabase != NULL)
	{
		free(CurrentDatabase);
	}
	CurrentDatabase = (char *)malloc(strlen(db_name) + 1);
	strcpy(CurrentDatabase, db_name);
	if (CurrentText != NULL)
	{
		free(CurrentText);
	}
	CurrentText = (char *)malloc(strlen(text) + 1);
	strcpy(CurrentText, text);
	if (CurrentQuery != NULL)
	{
		DMFfreeFields(CurrentQuery);
	}
	CurrentQuery = NULL;

	out_port = DTMmakeOutPort(ret_addr, DTM_SYNC);

	query = QueryConvert(query);

	DMFsendQuery(out_port, query, 1000);

	ret = awaitDMFReply(in_port, &header, MC_MATCH);
	if (ret == MC_UNKNOWN)
	{
		return(NULL);
	}
	/*
	 * MC_INFO is DMF's idea of an error return
	 */
	else if (ret == MC_INFO)
	{
		DMFrecvInfo(in_port, header, &text);
		DTMendRead(in_port);
		free(header);
		dmf_shutdown(in_port, out_port);
		return(text);
	}

	matches = NULL;
	mcnt = 0;
	DMFrecvMatch(in_port, header, &mcnt, 1000, &mtitle, &matches);
        DTMendRead(in_port);
	free(header);

	dmf_shutdown(in_port, out_port);

	text = MatchesToText(query, mtitle, matches, mcnt);

	if (matches != NULL)
	{
		DMFfreeMatch(&matches);
	}

	return(text);
}


char *
DMFInquire (char *tag, char *addr, char *path)
{
	int in_port, out_port;
	char *db_name;
	char *id;
	char *itype;
	char *ptr;
	char *header;
	char *my_addr;
	char *ret_addr;
	char *text;
	char *help;
	QFIELD *qfields;
	int ret, mcnt;
	char *mtitle;
	MATCH **matches;
	int key_id;


#ifdef DEBUG
	fprintf(stderr, "DMFInquire (%s, %s, %s) called\n", tag, addr, path);
#endif

	db_name = path;
	ptr = strchr(db_name, '/');
	/* No id */
	if (ptr == NULL)
	{
		return(NULL);
	}
	*ptr = '\0';
	ptr++;
	id = ptr;

	ptr = strchr(id, '/');
	/* No itype */
	if (ptr == NULL)
	{
		itype = NULL;
	}
	else
	{
		*ptr = '\0';
		ptr++;
		itype = ptr;
	}

	in_port = dmf_connect(addr, db_name, &my_addr);

	ret = awaitDMFReply(in_port, &header, MC_CONFIRM);
	if (ret == MC_UNKNOWN)
	{
		return(NULL);
	}
	/*
	 * MC_INFO is DMF's idea of an error return
	 */
	else if (ret == MC_INFO)
	{
		DMFrecvInfo(in_port, header, &text);
		DTMendRead(in_port);
		free(header);
		return(text);
	}

	qfields = NULL;
        DMFrecvConfirm(in_port, header, &ret_addr, &text, &qfields, &help);
        DTMendRead(in_port);
	free(header);
	DMFfreeFields(qfields);

	/*
	 * Set up the locally global variable for this database.
	 */
	if (CurrentAddress != NULL)
	{
		free(CurrentAddress);
	}
	CurrentAddress = (char *)malloc(strlen(addr) + 1);
	strcpy(CurrentAddress, addr);
	if (CurrentDatabase != NULL)
	{
		free(CurrentDatabase);
	}
	CurrentDatabase = (char *)malloc(strlen(db_name) + 1);
	strcpy(CurrentDatabase, db_name);
	if (CurrentText != NULL)
	{
		free(CurrentText);
	}
	CurrentText = (char *)malloc(strlen(text) + 1);
	strcpy(CurrentText, text);
	if (CurrentQuery != NULL)
	{
		DMFfreeFields(CurrentQuery);
	}
	CurrentQuery = NULL;

	out_port = DTMmakeOutPort(ret_addr, DTM_SYNC);

	key_id = atoi(id);

	DMFsendInquire(out_port, key_id, itype);
	if (strcmp(itype, INQ_FULLINFO) == 0)
	{
		ret = awaitDMFReply(in_port, &header, MC_INFORM);
		if (ret == MC_UNKNOWN)
		{
			return(NULL);
		}
		/*
		 * MC_INFO is DMF's idea of an error return
		 */
		else if (ret == MC_INFO)
		{
			DMFrecvInfo(in_port, header, &text);
			DTMendRead(in_port);
			free(header);
			dmf_shutdown(in_port, out_port);
			return(text);
		}

		DMFrecvInform(in_port, header, &mtitle, &text);
		DTMendRead(in_port);
		free(header);
		text = InfoToText(key_id, mtitle, text);
	}
	else
	{
		ret = awaitDMFReply(in_port, &header, MC_MATCH);
		if (ret == MC_UNKNOWN)
		{
			return(NULL);
		}
		/*
		 * MC_INFO is DMF's idea of an error return
		 */
		else if (ret == MC_INFO)
		{
			DMFrecvInfo(in_port, header, &text);
			DTMendRead(in_port);
			free(header);
			dmf_shutdown(in_port, out_port);
			return(text);
		}

		matches = NULL;
		mcnt = 0;
		DMFrecvMatch(in_port, header, &mcnt, 1000, &mtitle, &matches);
		DTMendRead(in_port);
		free(header);
		text = MatchesToText(itype, mtitle, matches, mcnt);
		if (matches != NULL)
		{
			DMFfreeMatch(&matches);
		}
	}

	dmf_shutdown(in_port, out_port);

	return(text);
}


static REQ_TYPE
ItemTypeConvert(char *itype)
{
	REQ_TYPE type;

	if (itype == NULL)
	{
		type = REQ_FILE;
	}
	else if (strcmp(itype, "FILE") == 0)
	{
		type = REQ_FILE;
	}
	else if (strcmp(itype, "DATA") == 0)
	{
		type = REQ_DATA;
	}
	else
	{
		type = REQ_FILE;
	}
	return(type);
}

static void DisplaySaveDialog();

char *
DMFItem (char *tag, char *addr, char *path)
{
	int in_port, out_port;
	char *db_name;
	char *id;
	char *rtype;
	char *dtype;
	char *ptr;
	char *header;
	char *my_addr;
	char *ret_addr;
	char *text;
	char *help;
	QFIELD *qfields;
	int ret, mcnt;
	char *mtitle;
	MATCH **matches;
	int key_id;
	REQ_TYPE type;


#ifdef DEBUG
	fprintf(stderr, "DMFItem (%s, %s, %s) called\n", tag, addr, path);
#endif

	db_name = path;
	ptr = strchr(db_name, '/');
	/* No id */
	if (ptr == NULL)
	{
		return(NULL);
	}
	*ptr = '\0';
	ptr++;
	id = ptr;

	ptr = strchr(id, '/');
	/* No rtype */
	if (ptr == NULL)
	{
		rtype = NULL;
	}
	else
	{
		*ptr = '\0';
		ptr++;
		rtype = ptr;
	}

	ptr = strchr(rtype, '/');
	/* No dtype */
	if (ptr == NULL)
	{
		dtype = "Data";
	}
	else
	{
		*ptr = '\0';
		ptr++;
		dtype = ptr;
	}

	in_port = dmf_connect(addr, db_name, &my_addr);

	ret = awaitDMFReply(in_port, &header, MC_CONFIRM);
	if (ret == MC_UNKNOWN)
	{
		return(NULL);
	}
	/*
	 * MC_INFO is DMF's idea of an error return
	 */
	else if (ret == MC_INFO)
	{
		DMFrecvInfo(in_port, header, &text);
		DTMendRead(in_port);
		free(header);
		return(text);
	}

	qfields = NULL;
        DMFrecvConfirm(in_port, header, &ret_addr, &text, &qfields, &help);
        DTMendRead(in_port);
	free(header);
	DMFfreeFields(qfields);

	/*
	 * Set up the locally global variable for this database.
	 */
	if (CurrentAddress != NULL)
	{
		free(CurrentAddress);
	}
	CurrentAddress = (char *)malloc(strlen(addr) + 1);
	strcpy(CurrentAddress, addr);
	if (CurrentDatabase != NULL)
	{
		free(CurrentDatabase);
	}
	CurrentDatabase = (char *)malloc(strlen(db_name) + 1);
	strcpy(CurrentDatabase, db_name);
	if (CurrentText != NULL)
	{
		free(CurrentText);
	}
	CurrentText = (char *)malloc(strlen(text) + 1);
	strcpy(CurrentText, text);
	if (CurrentQuery != NULL)
	{
		DMFfreeFields(CurrentQuery);
	}
	CurrentQuery = NULL;

	out_port = DTMmakeOutPort(ret_addr, DTM_SYNC);

	key_id = atoi(id);
	type = ItemTypeConvert(rtype);

	DMFsendRequest(out_port, key_id, type, my_addr);
	if ((type == REQ_FILE)&&(strcmp(dtype, "TEXT") == 0))
	{
		int len;
		int format;
		char *tptr;
		char *fname;

		ret = awaitDMFReply(in_port, &header, MC_FILE);
		if (ret == MC_UNKNOWN)
		{
			return(NULL);
		}
		/*
		 * MC_INFO is DMF's idea of an error return
		 */
		else if (ret == MC_INFO)
		{
			DMFrecvInfo(in_port, header, &text);
			DTMendRead(in_port);
			free(header);
			dmf_shutdown(in_port, out_port);
			return(text);
		}

		DMFrecvFileData(in_port, header, &mtitle, &format, &text, &len);
		DTMendRead(in_port);
		free(header);

		tptr = strrchr(mtitle, '/');
		if (tptr == NULL)
		{
			fname = mtitle;
		}
		else
		{
			fname = (char *)(tptr + 1);
		}

		text[len] = '\0';
		len = len + strlen("<H1></H1>\n<PLAINTEXT>\n") + strlen(fname);
		tptr = (char *)malloc(len + 1);
		strcpy(tptr, "<H1>");
		strcat(tptr, fname);
		strcat(tptr, "</H1>\n");
		strcat(tptr, "<PLAINTEXT>\n");
		strcat(tptr, text);

		free(text);
		text = tptr;
	}
	else if (type == REQ_FILE)
	{
		int format, len;
		struct file_rec *frec;

		ret = awaitDMFReply(in_port, &header, MC_FILE);
		if (ret == MC_UNKNOWN)
		{
			return(NULL);
		}
		/*
		 * MC_INFO is DMF's idea of an error return
		 */
		else if (ret == MC_INFO)
		{
			DMFrecvInfo(in_port, header, &text);
			DTMendRead(in_port);
			free(header);
			dmf_shutdown(in_port, out_port);
			return(text);
		}

		DMFrecvFileData(in_port, header, &mtitle, &format, &text, &len);
		DTMendRead(in_port);
		free(header);

		frec = (struct file_rec *)malloc(sizeof(struct file_rec));
		frec->name = (char *)malloc(strlen(mtitle) + 2);
/*
		strcpy(frec->name, ".");
*/
		if (mtitle[0] == '/')
		{
			strcpy(frec->name, (char *)(mtitle + 1));
		}
		else
		{
			strcpy(frec->name, mtitle);
		}
		frec->data = text;
		frec->format = format;
		frec->length = len;

		DisplaySaveDialog((XtPointer)frec);

		text = NULL;
	}
	else
	{
		ret = awaitDMFReply(in_port, &header, MC_DATA);
		if (ret == MC_UNKNOWN)
		{
			return(NULL);
		}
		/*
		 * MC_INFO is DMF's idea of an error return
		 */
		else if (ret == MC_INFO)
		{
			DMFrecvInfo(in_port, header, &text);
			DTMendRead(in_port);
			free(header);
			dmf_shutdown(in_port, out_port);
			return(text);
		}

		DTMendRead(in_port);
		free(header);
		text = NULL;
	}

	dmf_shutdown(in_port, out_port);

	return(text);
}


static void CBCancel();
static void CBSaveCancel();


static void
CBItemSave(Widget w, XtPointer client_data, XtPointer call_data)
{
	XmFileSelectionBoxCallbackStruct *fsb =
		(XmFileSelectionBoxCallbackStruct *)call_data;
	struct file_rec *frec = (struct file_rec *)client_data;
	char *filename;
	FILE *fp;

	XtUnmanageChild(save_dialog);
	XtRemoveCallback(save_dialog, XmNokCallback, CBItemSave,
		(XtPointer)frec);
	XtRemoveCallback(save_dialog, XmNcancelCallback, CBSaveCancel,
		(XtPointer)frec);

	XmStringGetLtoR(fsb->value, XmSTRING_DEFAULT_CHARSET, &filename);

	DMFsaveFileData(filename, frec->name, frec->format,
		frec->data, frec->length);

/*
	fp = fopen (filename, "w");
	if (fp == NULL)
	{
		fprintf(stderr, "Cannot open %s for writing\n", filename);
		return;
	}
	fwrite (frec->data, sizeof(char), frec->length, fp);
*/
	free (frec->name);
	free (frec->data);
	free ((char *)frec);
/*
	fclose (fp);
*/
}


static void
CBFieldSet(Widget w, XtPointer client_data, XtPointer call_data)
{
	char *url = (char *)client_data;
	Cardinal argcnt;
	Arg arg[20];
	XmString string;
	char *value, *ptr;
	char *tag, *addr, *path;
	char *db_name, *field, *dtype;
	QFIELD *qptr;

#ifdef DEBUG
	fprintf(stderr, "CBFieldSet(%s) called \n", url);
#endif

	XtUnmanageChild(dialog);
	XtRemoveCallback(dialog, XmNokCallback, CBFieldSet, (XtPointer)url);
	XtRemoveCallback(dialog, XmNcancelCallback, CBCancel, (XtPointer)url);

	tag = HTParse(url, "", PARSE_ACCESS);
	addr = HTParse(url, "", PARSE_HOST);
	path = HTParse(url, "", PARSE_PATH);

	db_name = path;
	ptr = strchr(db_name, '/');
	/* No field */
	if (ptr == NULL)
	{
		return;
	}
	*ptr = '\0';
	ptr++;
	field = ptr;

	ptr = strchr(field, '/');

	if (ptr == NULL)
	{
		dtype = NULL;
	}
	else
	{
		*ptr = '\0';
		ptr++;
		dtype = ptr;
	}

	/*
	 * If the current database is not this database, get the current
	 * database.
	 */
	if ((CurrentDatabase == NULL)||(strcmp(CurrentDatabase, db_name) != 0)||
		(CurrentQuery == NULL))
	{
		char *text;

		text = DMFAnchor(tag, addr, db_name);
		if (text == NULL)
		{
			return;
		}
		free(text);
	}

	qptr = CurrentQuery;
	while (qptr != NULL)
	{
		if (strcmp(qptr->label, field) == 0)
		{
			break;
		}
		qptr = qptr->next;
	}
	if (qptr == NULL)
	{
		return;
	}

	argcnt = 0;
	XtSetArg(arg[argcnt], XmNtextString, &string); argcnt++;
	XtGetValues(dialog, arg, argcnt);

	XmStringGetLtoR(string, XmSTRING_DEFAULT_CHARSET, &value);

	if (qptr->data != NULL)
	{
		free((char *)qptr->data);
	}
	qptr->data = (char *)malloc(strlen(value) + 1);
	strcpy(qptr->data, value);

	dmffield_anchor_cb(url, QueryRefText, QueryWin, QueryData);
}


static void
CBCancel(Widget w, XtPointer client_data, XtPointer call_data)
{
	XtUnmanageChild(dialog);
	XtRemoveCallback(dialog, XmNokCallback, CBFieldSet, client_data);
	XtRemoveCallback(dialog, XmNcancelCallback, CBCancel, client_data);
}
static void
CBSaveCancel(Widget w, XtPointer client_data, XtPointer call_data)
{
	struct file_rec *frec = (struct file_rec *)client_data;

	XtUnmanageChild(save_dialog);
	XtRemoveCallback(save_dialog, XmNokCallback, CBItemSave, client_data);
	XtRemoveCallback(save_dialog, XmNcancelCallback, CBSaveCancel,
		client_data);

	free (frec->name);
	free (frec->data);
	free ((char *)frec);
}


static void
DisplayDialog(char *url, char *field)
{
	Cardinal argcnt;
	Arg arg[20];
	XmString label, valLabel;

        if (!dialog_prepared)
          {
            PrepareDMFDialog (toplevel);
            dialog_prepared = 1;
          }

	label = XmStringCreateSimple(field);
	argcnt = 0;
	XtSetArg(arg[argcnt], XmNselectionLabelString, label); argcnt++;
	XtSetValues(dialog, arg, argcnt);
	XmStringFree(label);

	XtAddCallback(dialog, XmNokCallback, CBFieldSet, (XtPointer)url);
	XtAddCallback(dialog, XmNcancelCallback, CBCancel, (XtPointer)url);

	XtManageChild(dialog);
}


static void
DisplaySaveDialog(XtPointer data)
{
	Cardinal argcnt;
	Arg arg[20];
	XmString label;
	XmString tlabel;
	char *lab;

        if (!save_dialog_prepared)
          {
            PrepareDMFSaveDialog (toplevel);
            save_dialog_prepared = 1;
          }

	lab = (char *)malloc(strlen(((struct file_rec *)data)->name) +
		strlen("Directory for: ") + 1);
	sprintf(lab, "Directory for: %s", ((struct file_rec *)data)->name);
	tlabel = XmStringCreateSimple(lab);
/*
	label = XmStringCreateSimple(((struct file_rec *)data)->name);
*/
	label = XmStringCreateSimple("./");
	argcnt = 0;
	XtSetArg(arg[argcnt], XmNtextString, label); argcnt++;
	XtSetArg(arg[argcnt], XmNselectionLabelString, tlabel); argcnt++;
	XtSetValues(save_dialog, arg, argcnt);
	XmStringFree(label);
	XmStringFree(tlabel);
	free(lab);

	XtAddCallback(save_dialog, XmNokCallback, CBItemSave,
		(XtPointer)data);
	XtAddCallback(save_dialog, XmNcancelCallback, CBSaveCancel,
		(XtPointer)data);

	XtManageChild(save_dialog);
}


void
PrepareDMFDialog(Widget top)
{
	Cardinal argcnt;
	Arg arg[20];
	XmString label, valLabel;

	label = XmStringCreateSimple("field");
	valLabel = XmStringCreateSimple("");
	argcnt = 0;
	XtSetArg(arg[argcnt], XtNtitle, "Enter DMF Field Data"); argcnt++;
	XtSetArg(arg[argcnt], XmNselectionLabelString, label); argcnt++;
	XtSetArg(arg[argcnt], XmNtextString, valLabel); argcnt++;
	XtSetArg(arg[argcnt], XmNkeyboardFocusPolicy, XmPOINTER); argcnt++;
	dialog = XmCreatePromptDialog(top, "anchorDialog", arg, argcnt);
	XmStringFree(valLabel);
	XmStringFree(label);
}

void
PrepareDMFSaveDialog(Widget top)
{
	Cardinal argcnt;
	Arg arg[20];
	XmString label;
	XmString tlabel;
	Widget child;

	label = XmStringCreateSimple("Directory for saved datafiles:");
	tlabel = XmStringCreateSimple("NCSA Mosaic: Save Data");
	argcnt = 0;
	XtSetArg(arg[argcnt], XmNdialogTitle, tlabel); argcnt++;
	XtSetArg(arg[argcnt], XmNselectionLabelString, label); argcnt++;
	XtSetArg(arg[argcnt], XmNkeyboardFocusPolicy, XmPOINTER); argcnt++;
	save_dialog = XmCreateFileSelectionDialog(top, "saveDialog",
		arg, argcnt);
	XmStringFree(tlabel);
	XmStringFree(label);

	/*
	 * Unmanage the files list, so that can only choose
	 * directories.
	 */
	child = XmFileSelectionBoxGetChild(save_dialog, XmDIALOG_LIST);
	XtUnmanageChild(child);
	child = XmFileSelectionBoxGetChild(save_dialog, XmDIALOG_LIST_LABEL);
	XtUnmanageChild(child);
}


void
dmf_field_dialog (char *url, char *reftext, mo_window *win, int force_newwin)
{
	char *path;
	char *db_name;
	char *field;
	char *dtype;
	char *ptr;
	QFIELD *qptr;

	path = HTParse(url, "", PARSE_PATH);

	db_name = path;
	ptr = strchr(db_name, '/');
	/* No field */
	if (ptr == NULL)
	{
		return;
	}
	*ptr = '\0';
	ptr++;
	field = ptr;

	ptr = strchr(field, '/');

	if (ptr != NULL)
	{
		*ptr = '\0';
	}

	QueryRefText = reftext;
	QueryWin = win;
	QueryData = force_newwin;

	DisplayDialog(url, field);
}


#endif /* HAVE_DMF */

