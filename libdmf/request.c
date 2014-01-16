#include	<stdio.h>
#include	<string.h>
#include	<dtm.h>

#include	"dmf.h"


/*
** DTMsendRequest - used to request a specific item from DMF Server.
**
** Args
**	port	- DTM ouput port connected to DMF server
**	keyid	- unique indentifier of item
**	request	- requesting an exact copy of the file or just the data
**	address - where to send the file or data
*/
int DMFsendRequest(port, keyid, request, address)
	int			port, keyid;
	char		*address;
	REQ_TYPE	request;
{
	char	header[DTM_MAX_HEADER];

	dtm_set_class(header, REQUESTclass);
	dtm_set_int(header, "KEYID", keyid);
	dtm_set_int(header, "REQTYPE", request);
	dtm_set_address(header, address);

	DTMwriteMsg(port, header, DTMHL(header), NULL, 0, DTM_CHAR);
}


/*
** DMF Server Call
**
** DMFrecvRequest - receives the request for a specific item
**
** Args
**	port	- DTM input port
**	header	- message header
**	keyid	- unique identifier for item
**	request	- request type (file or data)
**	address - return address for the file or data
*/
int DMFrecvRequest(port, header, keyid, request, address)
	int			port, *keyid;
	char		*header, **address;
	REQ_TYPE	*request;
{
	char	s[DTM_MAX_HEADER];

	dtm_get_int(header, "KEYID", keyid);
	dtm_get_int	(header, "REQTYPE", (int*)request);

	dtm_get_address(header, s, sizeof s);
	*address = strdup(s);
}
