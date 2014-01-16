#include	<stdio.h>
#include	<string.h>
#include	<dtm.h>

#include	"dmf.h"


/*
** DMFsendInquire - sends a query to the DMF server requesting
**	additional information about a specific item.
**
** Args
**	port	- DTM output port connected to DMF Server
**	keyid	- unique reference number of item
**	relation- the relationship string to be matched
**
** Note: the relation string may consist of "DMFALL" which finds
**	all related items.
*/
int DMFsendInquire(port, keyid, inquire)
	int	port, keyid;
	char	*inquire;
{
	char	header[DTM_MAX_HEADER];

	dtm_set_class(header, INQUIREclass);
	dtm_set_int(header, "KEYID", keyid);
	dtm_set_char(header, "INQOPT", inquire);

	DTMwriteMsg(port, header, DTMHL(header), NULL, 0, DTM_CHAR);
}


/*
** DMF Server Call
**
** DMFrecvInquire - receive a query about a specific item.
**
** Args
**	port	- DTM input port
**	header	- message header
**	keyid	- return value, unique identifier for item
**	relation	- return value, relationship string
*/
int DMFrecvInquire(port, header, keyid, relation)
	int	port, *keyid;
	char	*header;
	char	**relation;
{
	char	s[DTM_MAX_HEADER];

	dtm_get_int(header, "KEYID", keyid);
	dtm_get_char(header, "INQOPT", s, sizeof s);
	
	*relation = strdup(s);

	return DMFOK;
}
