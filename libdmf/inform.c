#include	<stdio.h>
#include	<string.h>
#include	<dtm.h>

#include	"dmf.h"



/*
** DMF Server Call
**
** DMFsendInform - send a formatted page of text, typically used to
**	return the full information about a item in response to an 'inquire'
**	message.
**
** Args
**	port	- DTM output port connected to client
**	title	- title of text page
**	text	- formatted text page
*/
int DMFsendInform(port, title, text)
	int		port;
	char	*title, *text;
{
	int		length;
	char	header[DTM_MAX_HEADER];


	dtm_set_class(header, INFORMclass);
	dtm_set_title(header, title);

	DTMbeginWrite(port, header, DTMHL(header));

	/* sends short description of database */
	length = strlen(text)+1;
	DTMwriteDataset(port, &length, 1, DTM_INT);
	DTMwriteDataset(port, text, length, DTM_CHAR);

	DTMendWrite(port);
}


/*
** DMFrecvInform - receives of formatted text page.  Typically, used
**	to return full information about an item.  The results should be
**	display to the user.
**
** Args
**	port	- DTM input port
**	header	- message header
**	title	- title string for the text page
**	text	- formatted text page to be displayed
*/
int DMFrecvInform(port, header, title, text)
	int		port;
	char	*header, **title, **text;
{
	int		length;
	char	s[256];


	dtm_get_title(header, s, sizeof s);
	*title = strdup(s);


	DTMreadDataset(port, &length, 1, DTM_INT);

	if ((*text = (char*)malloc(length)) == NULL)  {
		fprintf(stderr, "Error: insufficient memory\n");
		return -1;
	}

	DTMreadDataset(port, *text, length, DTM_CHAR);
}
