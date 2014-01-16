#include	<stdio.h>
#include	<string.h>
#include	<varargs.h>
#include	<dtm.h>

#include	"dmf.h"


/*
** DMFsendInfo - sends the single text string to DMF interface
**
** Args
**	port	- dtm port, if -1 prints to stderr
**	text	- string to be sent
*/
int DMFsendInfo(port, text)
	int		port;
	char	*text;
{
	int		length;
	char	header[DTM_MAX_HEADER];

	dtm_set_class(header, INFOclass);

	DTMbeginWrite(port, header, DTMHL(header));

	/* sends status message */
	length = strlen(text)+1;
	DTMwriteDataset(port, &length, 1, DTM_INT);
	DTMwriteDataset(port, text, length, DTM_CHAR);

	DTMendWrite(port);
}


/*
** DMFrecvInfo - receive the text string
**
** Args
**	port	- dtm port
**	header	- message header
**	text	- text received
*/
int DMFrecvInfo(port, header, text)
	int		port;
	char	*header, **text;
{
	int		length;
	char	s[256];

	DTMreadDataset(port, &length, 1, DTM_INT);

	if ((*text = (char*)malloc(length)) == NULL)  {
		fprintf(stderr, "Error: insufficient memory\n");
		return -1;
	}

	DTMreadDataset(port, *text, length, DTM_CHAR);
}


/*
** DMFsendError - send DMF error messages
**
** Args
**	port	- destination port for errors, if -1 the error is printed
**			  to stderr
**  error	- error index number
**	format	- format string as with printf
**	...		- any remaining arguements
**
*/
void DMFsendError(va_alist) va_dcl
{
	va_list	ap;
	int		port, error;
	char	*format;
	char	msg[1024], header[DTM_MAX_HEADER];

	va_start(ap);

	port = va_arg(ap, int);
	error = va_arg(ap, int);
	format = va_arg(ap, char *);

	if (format != NULL)
		vsprintf(msg, format, ap);

	va_end(ap);

	if (port == -1)  {
		fprintf(stderr, "%s\n", msg);
		fprintf(stderr, "DMF error = %d\n", error);
	}

	else  {
		dtm_set_class(header, ERRORclass);
		dtm_set_int(header, "ERRID", error);
		dtm_set_char(header, "ERRMSG", msg);

		DTMwriteMsg(port, header, DTMHL(header), NULL, 0, DTM_CHAR);
	}
}


/*
** DMFrecvError - receive the DMF error index and string
**
** Args
**	port	- DTM port
**	header	- message header
**	error	- return value, error index
**	msg		- return value, error message
**/
void DMFrecvError(port, header, error, msg)
	int		port, *error;
	char	*header, **msg;
{
	char	buf[1024];

	dtm_get_int(header, "ERRID", error);
	dtm_get_char(header, "ERRMSG", buf, sizeof buf);

	*msg = strdup(buf);
}
