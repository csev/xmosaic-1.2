/**********************************************************************
**
** The following routines are used to communicate with the execution
**	server (ESERV).  The output port used by the following routines
**	should be connected to the desired ESERV.  Because the ESERV calls
**	'exec' based on the arguements provided, the output port should
**	be destroyed immediately following the call to one of these
**	routines.
**
**********************************************************************/

/* eserv.c,v 1.1 1993/02/26 05:21:13 marca Exp */


#include	<stdio.h>
#include	<string.h>
#include	<dtm.h>

#include	"dmf.h"


/*
** ESERVsendDbaseReq - requests a connection from the DMF server
**	responsible for the named database.
**
** Args
**	port	- DTM output port used to write request
**	address	- input address for current application (The DMF server
**				will send messages to this address)
**	dbase	- name of the requested database
**	user	- optional user name used for validation
*/
ESERVsendDbaseReq(port, address, dbase, user)
	int		port;
	char	*address, *dbase, *user;
{
	char	header[DTM_MAX_HEADER];

	dtm_set_class(header, DBASEclass);
	dtm_set_address(header, address);
	dtm_set_char(header, "DB", dbase);

	if (user != NULL)
		dtm_set_char(header, "USER", user);

	DTMwriteMsg(port, header, DTMHL(header), NULL, 0, DTM_CHAR);
}


/*
** ESERVrecvDbaseReq - complimentary function used by ESERV.
*/
ESERVrecvDbaseReq(port, header, address, dbase, user)
	int		port;
	char	*header, **address, **dbase, **user;
{
	char	s[DTM_MAX_HEADER];
	
	dtm_get_address(header, s, sizeof s);
	*address = strdup(s);

	dtm_get_char(header, "DB", s, sizeof s);
	*dbase = strdup(s);
}


/*
** ESERVsendExecReq - send a request to execute a command to an
**	execution server (ESERV).
**
** Args
**	port		- DTM output port connected to ESERV
**	address		- return address FOR ERROR MESSAGES ONLY!
**	cmd			- command to be executed by ESERV
**	user		- optional user name for validation
**
** Note: command string should have return address for data in it.
*/
ESERVsendExecReq(port, address, cmd, user)
	int		port;
	char	*cmd, *address, *user;
{
	char	header[DTM_MAX_HEADER];

	dtm_set_class(header, EXECclass);
	dtm_set_char(header, "CMD", cmd);
	dtm_set_address(header, address);
	if (user != NULL)
		dtm_set_char(header, "USER", user);

	DTMwriteMsg(port, header, DTMHL(header), NULL, 0, DTM_CHAR);
}


/*
** ESERVrecvExecReq - complimentary function used by ESERV.
*/
ESERVrecvExecReq(port, header, address, cmd, user)
	int		port;
	char	*header;
	char	**cmd, **address, **user;
{
	char	s[DTM_MAX_HEADER];

	dtm_get_char(header, "CMD", s, sizeof s);
	*cmd = strdup(s);

	dtm_get_address(header, s, sizeof s);
	*address = strdup(s);

	*user = NULL;
}


/*
** ESERVsendDataReq - requests data from an execution server (ESERV).
**	The file is first read by a tranlator program and which can then
**	deliver the data using DTM or a tranlated file format.
**
** Args
**	port		- DTM output port connected to ESERV
**	translator	- translator to be invoked by ESERV
**	address		- return address for output of translation
**	fname		- file name to be processed by translator
*/
ESERVsendDataReq(port, translator, address, fname)
	int		port;
	char	*translator, *address, *fname;
{
	char	header[DTM_MAX_HEADER];

	dtm_set_class(header, DATAclass);
	dtm_set_address(header, address);
	dtm_set_char(header, "TRANS", translator);
	dtm_set_char(header, "FILENAME", fname);

	DTMwriteMsg(port, header, DTMHL(header), NULL, 0, DTM_CHAR);
}


/*
** ESERVrecvDataReq - complimetary function used by ESERV
*/
ESERVrecvDataReq(port, header, translator, address, fname)
	int		port;
	char	*header, **translator, **address, **fname;
{
	char	s[DTM_MAX_HEADER];

	dtm_get_address(header, s, sizeof s);
	*address = strdup(s);

	dtm_get_char(header, "TRANS", s, sizeof s);
	*translator = strdup(s);

	dtm_get_char(header, "FILENAME", s, sizeof s);
	*fname = strdup(s);
}
	

/* obsolete call */
DMFrecvCREQ(port, header, database, address)
	int		port;
	char	*header, **database, **address;
{
	char	*cptr, s[256];

	cptr = strtok(header, " \t\n");
	while (cptr != NULL)
		if (!strcmp(cptr, "DBSN"))  {
			strcpy(s, strtok(NULL, " \t\n"));
			*database = strdup(s);
			strcpy(s, strtok(NULL, " \t\n"));
			*address = strdup(s);
			break;
		}
		else
			cptr = strtok(NULL,  " \t\n");
}
