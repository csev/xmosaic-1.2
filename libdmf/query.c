#include	<stdio.h>
#include	<string.h>
#include	<dtm.h>

#include	"dmf.h"


/*
** DMFsendQuery - send the query string to the DMF Server.
**
** Args
**	port	- DTM output port
**	query	- the query string
**	count	- the maximum matches the DMF server should return
**/
int DMFsendQuery(port, query, count)
	int		port, count;
	char	*query;
{
	char	header[DTM_MAX_HEADER];

	dtm_set_class(header, QUERYclass);
	dtm_set_int(header, "MAXMATCHES", count);
	dtm_set_char(header, "QSTRING", query);

	DTMwriteMsg(port, header, DTMHL(header), NULL, 0, DTM_CHAR);
}


/*
** DMF Server call
**
** DMFrecvQuery - receive the query string
**
** Args
**	port	- input port
**	header	- query header
**	query	- return pointer for query string
**	count	- max matches the interface will accept
*/
int DMFrecvQuery(port, header, query, count)
	int		port, *count;
	char	*header, **query;
{
	char	s[DTM_MAX_HEADER];

	dtm_get_int(header, "MAXMATCHES", count);

	dtm_get_char(header, "QSTRING", s, sizeof s);
	*query = strdup(s);
}
