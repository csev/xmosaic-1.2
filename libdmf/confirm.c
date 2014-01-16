#include	<stdio.h>
#include	<string.h>
#include	<dtm.h>

#include	"dmf.h"

static int encodeQField(ptr, buf, len)
	QFIELD	*ptr;
	char	*buf;
	int		len;
{

	sprintf(buf, "%d %d:%s %d:%s %d:%s", (int)ptr->type,
			strlen(ptr->label), ptr->label,
			strlen(ptr->help), ptr->help,
			strlen(ptr->data), ptr->data);

	return (strlen(buf)+1);
}


static void decodeQField(ptr, buf, len)
	QFIELD	*ptr;
	char	*buf;
	int		len;
{
	char	*cptr;
	char	typestr[32];

	cptr = strchr(buf, ' ');
	strncpy(typestr, buf, cptr-buf);
	ptr->type = atoi(typestr);

	/* copy label to structure */
	len = atoi(cptr);
	ptr->label = (char*)malloc(len+1);
	cptr = strchr(cptr, ':') + 1;
	strncpy(ptr->label, cptr, len);
	*(ptr->label + len) = '\0';

	/* copy description to structure */
	cptr += len + 1;
	len = atoi(cptr);
	ptr->help = (char*)malloc(len+1);
	cptr = strchr(cptr, ':') + 1;
	strncpy(ptr->help, cptr, len);
	*(ptr->help + len) = '\0';

	/* copy data to structure */
	cptr += len + 1;
	len = atoi(cptr);
	ptr->data = (char*)malloc(len+1);
	cptr = strchr(cptr, ':') + 1;
	strncpy(ptr->data, cptr, len);
	*(ptr->data + len) = '\0';
}


/*
** DMF Server Call
**
** DMFsendConfirm  -  sends a CONFIRM message consisting of introductory
**	message, a help message, and a field list of queriable fields
**
** Args
**	port		- DTM output port
**	address		- return address of the DMF CONFIRM originator
**	tbuf		- text buffer with short description
**	fields		- a linked list of QFIELDS
**	hbuf		- help page describing QFIELDS
*/
int DMFsendConfirm(port, address, tbuf, fields, hbuf)
	int		port;
	char	*address, *tbuf, *hbuf;
	QFIELD	*fields;
{
	int		length;
	char	header[DTM_MAX_HEADER];


	dtm_set_class(header, "CONFIRM");
	dtm_set_char(header, "PORT", address);

	DTMbeginWrite(port, header, DTMHL(header));

	/* sends short description of database */
	length = strlen(tbuf)+1;
	DTMwriteDataset(port, &length, 1, DTM_INT);
	DTMwriteDataset(port, tbuf, length, DTM_CHAR);

	/* sends help information for browse panel */
	length = strlen(hbuf)+1;
	DTMwriteDataset(port, &length, 1, DTM_INT);
	DTMwriteDataset(port, hbuf, length, DTM_CHAR);

	/* send field descriptions */
	while (fields != NULL)  {
		length = encodeQField(fields, header, sizeof header);
		DTMwriteDataset(port, &length, 1, DTM_INT);
		DTMwriteDataset(port, header, length, DTM_CHAR);
		fields = fields->next;
	}

	DTMendWrite(port);
}


/*
** DMFrecvConfirm  -  this routines reads a DMF CONFIRM message from a
**	DTM port.  It assumes that the application has already read the
**	the header (to determine the message class) and only desires this
**  routine to parse the header and	extract any information stored in
**  the data section.
**
** input arguements:
**	port		- DTM input port
**	header		- the message header
** return arguements:
**	address		- return address of the DMF CONFIRM originator
**	tbuf		- text buffer with short description
**	fields		- a linked list of QFIELDS
**	hbuf		- help page describing QFIELDS
*/
int DMFrecvConfirm(port, header, address, tbuf, fields, hbuf)
	int		port;
	char	*header, **address, **tbuf, **hbuf;
	QFIELD	**fields;
{
	int		length;
    char	qfield[1024], addr[32];
	QFIELD	*qfptr;


	dtm_get_char(header, "PORT", addr, sizeof addr);
	*address = strdup(addr);

	DTMreadDataset(port, &length, 1, DTM_INT);
	if ((*tbuf = (char*)malloc(length)) == NULL)  {
		fprintf(stderr, "Error: insufficient memory\n");
		return -1;
	}
	DTMreadDataset(port, *tbuf, length, DTM_CHAR);

	DTMreadDataset(port, &length, 1, DTM_INT);
	if ((*hbuf = (char*)malloc(length)) == NULL)  {
		fprintf(stderr, "Error: insufficient memory\n");
		return -1;
	}
	DTMreadDataset(port, *hbuf, length, DTM_CHAR);


	while (DTMreadDataset(port, &length, 1, DTM_INT) > 0)  {

		if (*fields == NULL)  {
			if ((*fields = (QFIELD *)malloc(sizeof (QFIELD))) == NULL)  {
				fprintf(stderr, "Error: insufficient memory\n");
				return -1;
			}
			qfptr = *fields;
		}
		else if ((qfptr->next = (QFIELD*)malloc(sizeof (QFIELD))) == NULL)  {
			fprintf(stderr, "Error: insufficient memory\n");
			return -1;
		}
		else
			qfptr = qfptr->next;

		DTMreadDataset(port, qfield, length, DTM_CHAR);
		decodeQField(qfptr, qfield, length);
	}
	qfptr->next = NULL;
}


/*
** DMFfreeFields - frees all the fields linked list
*/
void DMFfreeFields(fields)
	QFIELD	*fields;
{

	/* free any allocated buffers */
	if (fields->label != NULL)
		free(fields->label);
	if (fields->help != NULL)
		free(fields->help);
	if (fields->data != NULL)
		free(fields->data);

	/* recursively free other elements of the linked-list */
	if (fields->next != NULL)
		DMFfreeFields(fields->next);

	/* finally free the field structure itself */
	free(fields);
}


/*
** DMFsendComplete - terminate the session with the DMF server.
**
** Args
**	port 	- DTM output port connected to DMF server
*/
int DMFsendComplete(port)
	int		port;
{
	char	header[DTM_MAX_HEADER];

	dtm_set_class(header, "COMPLETE");

	DTMwriteMsg(port, header, DTMHL(header), NULL, 0, DTM_CHAR);
}
