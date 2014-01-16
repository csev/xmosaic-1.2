#include	<stdio.h>
#include	<string.h>
#include	<dtm.h>

#include	"dmf.h"



static void decode_match(matchbuf, keyid, datatype, mblob)
	char	*matchbuf, *datatype, *mblob;
	int		*keyid;
{
	char	*cptr;
	int		len;

	/* get keyid value */
	*keyid = atoi(matchbuf);
	cptr = strchr(matchbuf, ' ') + 1;

	/* get datatype value */
	len = atoi(cptr);
	cptr = strchr(cptr, ':') + 1;
	strncpy(datatype, cptr, len);
	*(datatype + len) = '\0';
	cptr += len + 1;

	/* get match string */
	len = atoi(cptr);
	cptr = strchr(cptr, ':') + 1;
	strncpy(mblob, cptr, len);
	*(mblob + len) = '\0';
}


/*
** DMF Server Call
**
** DMFinitMatch, DMFappendMatch, DMFendMatch - due to ugliness in
**	the database interface, the matches are returned piecemeal.  These
**	three calls are used to send back the list of matches.
**
** Args
**	port	- DTM output port connected to client
**	title	- Title string of fields being returned in match
*/
int DMFinitMatch(port, title)
	int		port;
	char	*title;
{
	int		i;
	char	header[DTM_MAX_HEADER];

	/* set titlebar in header */
	dtm_set_class(header, MATCHclass);
	dtm_set_char(header, "MATCHTITLE", title);
	DTMbeginWrite(port, header, DTMHL(header));
}

/*
** Args
**	port		- DTM output port connected to client
**	keyid		- keyid of match
**	datatype	- datatype of match
**	matchstring	- string describing match
*/
int DMFappendMatch(port, keyid, datatype, matchstring)
	int		port;
	char	*keyid, *datatype, *matchstring;
{
	int		length;
	char	match[1024];

	/* create match from keyid, datatype and matchstring */
	sprintf(match, "%s %d:%s %d:%s", keyid, strlen(datatype), datatype,
			strlen(matchstring), matchstring);

	/* send length, then match */
    length = strlen(match)+1;
    DTMwriteDataset(port, &length, 1, DTM_INT);
    DTMwriteDataset(port, match, length, DTM_CHAR);
}

/*
** Args
**	port	- DTM output port connected to client
*/
int DMFendMatch(port)
	int		port;
{
    DTMendWrite(port);
}


/*
** DMFrecvMatch - receive a list of matches from the DMF Server.
**	Included is a title string describing the fields in the match as
**	as the type of match and the unique identifier used to reference
**	the item.
**
** Args
**	port		- DTM input port
**	header		- header from message
**	maxmatches	- the maximum number of matches to be placed in match array
**	count		- return value, actual number of matches
**	matchtitle	- return value, string describing fields in match
**	matches		- return value, first element of match array
*/
int DMFrecvMatch(port, header, count, maxmatches, matchtitle, matches)
	int		port, *count, maxmatches;
	char	*header, **matchtitle;
	MATCH	***matches;
{
	int		length, index, keyid;
	char	mbuf[1024], datatype[128], mstring[256];
	MATCH	*ptr;


    /* parse the header */
    dtm_get_char(header, "MATCHTITLE", mbuf, sizeof mbuf);
	*matchtitle = strdup(mbuf);

	/* allocate array of pointers to MATCH */
	if ((*matches = (MATCH **)malloc((maxmatches+1)*sizeof(MATCH *))) == NULL)
		return -1;


	/* Read in each match as it comes in */
	/* First comes the length of the match, then the match itself */
	index = 0;
    while (index < maxmatches && DTMreadDataset(port, &length, 1, DTM_INT))  {

        DTMreadDataset(port, mbuf, length, DTM_CHAR);

		decode_match(mbuf, &keyid, datatype, mstring);

		if ((ptr = (MATCH*)malloc(sizeof (MATCH) + strlen(mstring))) == NULL)
			return -1;

		ptr->keyid = keyid;
		strncpy(ptr->datatype, datatype, sizeof ptr->datatype);
		ptr->datatype[(sizeof ptr->datatype) - 1] = '\0';
		strcpy(ptr->matchblob, mstring);

		(*matches)[index++] = ptr;
	}

	/* mark the end of the array of matches */
	(*matches)[index] = NULL;

	/* Set number of matches recieved */
	*count = index;

    /* make sure the some matches have been returned */
    if (*count ==  0)
        *matches = NULL;

	return 0;
}


/*
** DMFfreeMatch - used to free the match array returned by DMFrecvMatch.
**
** Args
**	matches		- pointer to match array
*/
int DMFfreeMatch(matches)
	MATCH	***matches;
{
	int i;

	/* free structures pointed to by each array pointer */
	for (i = 0; ((*matches)[i]) != NULL ; i++) 
			free((*matches)[i]);

	/* free array of match pointers and set to NULL */
	free(*matches);
	*matches = NULL;
}


