/*			File Transfer Protocol (FTP) Client
**			for a WorldWideWeb browser
**			===================================
**
**	A cache of control connections is kept.
**
** Note: Port allocation
**
**	It is essential that the port is allocated by the system, rather
**	than chosen in rotation by us (POLL_PORTS), or the following
**	problem occurs.
**
**	It seems that an attempt by the server to connect to a port which has
**	been used recently by a listen on the same socket, or by another
**	socket this or another process causes a hangup of (almost exactly)
**	one minute. Therefore, we have to use a rotating port number.
**	The problem remains that if the application is run twice in quick
**	succession, it will hang for what remains of a minute.
**
** Authors
**	TBL	Tim Berners-lee <timbl@info.cern.ch>
**	DLR	Denis DeLaRoca 310 825-4580 <CSP1DWD@mvs.oac.ucla.edu>
** History:
**	 2 May 91	Written TBL, as a part of the WorldWideWeb project.
**	15 Jan 92	Bug fix: close() was used for NETCLOSE for control soc
**	10 Feb 92	Retry if cached connection times out or breaks
**	 8 Dec 92	Bug fix 921208 TBL after 
**
** Options:
**	LISTEN		We listen, the other guy connects for data.
**			Otherwise, other way round, but problem finding our
**			internet address!
*/

#define LISTEN		/* @@@@ Test */

/*
BUGS:	@@@  	Limit connection cache size!
		Error reporting to user.
		400 & 500 errors are acked by user with windows.
		Use configuration file for user names
		Prompt user for password
		
**		Note for portablility this version does not use select() and
**		so does not watch the control and data channels at the
**		same time.
*/		

#define REPEAT_PORT	/* Give the port number for each file */
#define REPEAT_LISTEN	/* Close each listen socket and open a new one */

/* define POLL_PORTS		 If allocation does not work, poll ourselves.*/
#define LISTEN_BACKLOG 2	/* Number of pending connect requests (TCP)*/

#define FIRST_TCP_PORT	1024	/* Region to try for a listening port */
#define LAST_TCP_PORT	5999	

#define LINE_LENGTH 256
#define COMMAND_LENGTH 256

#include <ctype.h>

#include "HTParse.h"
#include "HTUtils.h"
#include "tcp.h"
#include "HTTCP.h"
#include "HTAnchor.h"
#include "HText.h"
#include "HTStyle.h"

#include "HTFTP.h"

#ifndef IPPORT_FTP
#define IPPORT_FTP	21
#endif

extern HTStyleSheet * styleSheet;
PRIVATE HTStyle *h1Style;			/* For heading level 1 */
PRIVATE HTStyle *h2Style;			/* For heading level 2 */
PRIVATE HTStyle *textStyle;			/* Text style */
PRIVATE HTStyle *dirStyle;			/* Compact List style */

#ifdef REMOVED_CODE
extern char *malloc();
extern void free();
extern char *strncpy();
#endif

typedef struct _connection {
    struct _connection *	next;	/* Link on list 	*/
    u_long			addr;	/* IP address		*/
    int				socket;	/* Socket number for communication */
    BOOL			binary; /* Binary mode? */
} connection;

#ifndef NIL
#define NIL 0
#endif


/*	Module-Wide Variables
**	---------------------
*/
PRIVATE connection * connections =0;	/* Linked list of connections */
PRIVATE char    response_text[LINE_LENGTH+1];/* Last response from NewsHost */
PRIVATE connection * control;		/* Current connection */
PRIVATE int	data_soc = -1;		/* Socket for data transfer =invalid */

#ifdef POLL_PORTS
PRIVATE	unsigned short	port_number = FIRST_TCP_PORT;
#endif

#ifdef LISTEN
PRIVATE int     master_socket = -1;	/* Listening socket = invalid	*/
PRIVATE char	port_command[255];	/* Command for setting the port */
PRIVATE fd_set	open_sockets; 		/* Mask of active channels */
PRIVATE int	num_sockets;  		/* Number of sockets to scan */
#else
PRIVATE	unsigned short	passive_port;	/* Port server specified for data */
#endif


#define NEXT_CHAR HTGetChararcter()	/* Use function in HTFormat.c */

#define DATA_BUFFER_SIZE 2048
PRIVATE char data_buffer[DATA_BUFFER_SIZE];		/* Input data buffer */
PRIVATE char * data_read_pointer;
PRIVATE char * data_write_pointer;
#define NEXT_DATA_CHAR next_data_char()


/*	Procedure: Read a character from the data connection
**	----------------------------------------------------
*/
PRIVATE char next_data_char
NOARGS
{
    int status;
    if (data_read_pointer >= data_write_pointer) {
	status = NETREAD(data_soc, data_buffer, DATA_BUFFER_SIZE);
	/* Get some more data */
	if (status <= 0) return (char)-1;
	data_write_pointer = data_buffer + status;
	data_read_pointer = data_buffer;
    }
#ifdef NOT_ASCII
    {
        char c = *data_read_pointer++;
	return FROMASCII(c);
    }
#else
    return *data_read_pointer++;
#endif
}


/*	Close an individual connection
**
*/
#ifdef __STDC__
PRIVATE int close_connection(connection * con)
#else
PRIVATE int close_connection(con)
    connection *con;
#endif
{
    connection * scan;
    int status = NETCLOSE(con->socket);
    if (TRACE) fprintf(stderr, "FTP: Closing control socket %d\n", con->socket);
    if (connections==con) {
        connections = con->next;
	return status;
    }
    for(scan=connections; scan; scan=scan->next) {
        if (scan->next == con) {
	    scan->next = con->next;	/* Unlink */
	    if (control==con) control = (connection*)0;
	    return status;
	} /*if */
    } /* for */
    return -1;		/* very strange -- was not on list. */
}


/*	Execute Command and get Response
**	--------------------------------
**
**	See the state machine illustrated in RFC959, p57. This implements
**	one command/reply sequence.  It also interprets lines which are to
**	be continued, which are marked with a "-" immediately after the
**	status code.
**
** On entry,
**	con	points to the connection which is established.
**	cmd	points to a command, or is NIL to just get the response.
**
**	The command is terminated with the CRLF pair.
**
** On exit,
**	returns:  The first digit of the reply type,
**		  or negative for communication failure.
*/
#ifdef __STDC__
PRIVATE int response(char * cmd)
#else
PRIVATE int response(cmd)
    char * cmd;
#endif
{
    int result;				/* Three-digit decimal code */
    char	continuation;
    int status;
    int multiline_response = 0;

    if (!control) {
          if(TRACE) fprintf(stderr, "FTP: No control connection set up!!\n");
	  return -99;
    }
    
    if (cmd) {
    
	if (TRACE) fprintf(stderr, "  Tx: %s", cmd);

#ifdef NOT_ASCII
	{
	    char * p;
	    for(p=cmd; *p; p++) {
	        *p = TOASCII(*p);
	    }
	}
#endif 
	status = NETWRITE(control->socket, cmd, (int)strlen(cmd));
	if (status<0) {
	    if (TRACE) fprintf(stderr, 
	    	"FTP: Error %d sending command: closing socket %d\n",
		status, control->socket);
	    close_connection(control);
	    return status;
	}
    }



/* Patch to be generally compatible with RFC 959 servers  -spok@cs.cmu.edu  */
/* Multiline responses start with a number and a hyphen;
   end with same number and a space.  When it ends, the number must
   be flush left.
*/
    do {
        char *p = response_text;
        /* If nonzero, it's set to initial code of multiline response */
        for(;;) {
            if (((*p++=NEXT_CHAR) == '\n')
                        || (p == &response_text[LINE_LENGTH])) {
                *p++=0;                 /* Terminate the string */
                if (TRACE) fprintf(stderr, "    Rx: %s", response_text);
                sscanf(response_text, "%d%c", &result, &continuation);
                if (continuation == '-' && !multiline_response) {
                    multiline_response = result;
                }
                else if (multiline_response && continuation == ' ' &&
                         multiline_response == result &&
                         isdigit(response_text[0])) {
                    /* End of response (number must be flush on left) */
                    multiline_response = 0;
                }
                break;
            } /* if end of line */

            if (*(p-1) < 0) {
                if(TRACE) fprintf(stderr, "Error on rx: closing socket %d\n",
                    control->socket);
                strcpy(response_text, "000 *** TCP read error on response\n");
                close_connection(control);
                return -1;      /* End of file on response */
            }
        } /* Loop over characters */

    } while (multiline_response);


#ifdef OLD
    do {
	char *p = response_text;
	for(;;) {  
	    if (((*p++=NEXT_CHAR) == '\n')
			|| (p == &response_text[LINE_LENGTH])) {
		*p++=0;			/* Terminate the string */
		if (TRACE) fprintf(stderr, "    Rx: %s", response_text);
		sscanf(response_text, "%d%c", &result, &continuation);
		break;	    
	    } /* if end of line */
	    
	    if (*(p-1) < 0) {
		if(TRACE) fprintf(stderr, "Error on rx: closing socket %d\n",
		    control->socket);
		strcpy(response_text, "000 *** TCP read error on response\n");
	        close_connection(control);
	    	return -1;	/* End of file on response */
	    }
	} /* Loop over characters */

    } while (continuation == '-');
#endif
    
    if (result==421) {
	if(TRACE) fprintf(stderr, "FTP: They close so we close socket %d\n",
	    control->socket);
	close_connection(control);
	return -1;
    }
    return result/100;
}


/*	Get a valid connection to the host
**	----------------------------------
**
** On entry,
**	arg	points to the name of the host in a hypertext address
**      allow_reuse    1 if connections are allowed to be reused;
**                     0 if they're not (usual value would be 1)
** On exit,
**	returns	<0 if error
**		socket number if success
**
**	This routine takes care of managing timed-out connections, and
**	limiting the number of connections in use at any one time.
**
**	It ensures that all connections are logged in if they exist.
**	It ensures they have the port number transferred.
*/
PRIVATE int get_connection ARGS2 (CONST char *,arg, int, allow_reuse)
{
    struct sockaddr_in soc_address;	/* Binary network address */
    struct sockaddr_in* sin = &soc_address;

    char * username=0;
    char * password=0;
    
    if (!arg) return -1;		/* Bad if no name sepcified	*/
    if (!*arg) return -1;		/* Bad if name had zero length	*/

/*  Set up defaults:
*/
    sin->sin_family = AF_INET;	    		/* Family, host order  */
    sin->sin_port = htons(IPPORT_FTP);	    	/* Well Known Number    */

    if (TRACE) fprintf(stderr, "FTP: Looking for %s\n", arg);

/* Get node name:
*/
    {
	char *p1 = HTParse(arg, "", PARSE_HOST);
	char *p2 = strchr(p1, '@');	/* user? */
	char * pw;
	if (p2) {
	    username = p1;
	    *p2=0;			/* terminate */
	    p1 = p2+1;			/* point to host */
	    pw = strchr(username, ':');
	    if (pw) {
	        *pw++ = 0;
		password = pw;
	    }
	}
	if (HTParseInet(sin, p1)) { free(p1); return -1;} /* TBL 920622 */

        if (!username) free(p1);
    } /* scope of p1 */

        
/*	Now we check whether we already have a connection to that port.
*/

    /* This breaks. */
    {
      connection * scan;
      for (scan=connections; scan; scan=scan->next) {
        if (sin->sin_addr.s_addr == scan->addr) {
          if (TRACE) fprintf(stderr, 
                             "FTP: Already have connection for %d.%d.%d.%d.\n",
                             (int)*((unsigned char *)(&scan->addr)+0),
                             (int)*((unsigned char *)(&scan->addr)+1),
                             (int)*((unsigned char *)(&scan->addr)+2),
                             (int)*((unsigned char *)(&scan->addr)+3));

#if 0
          if (allow_reuse)
            {
              if (username) free(username);
              return scan->socket;		/* Good return */
            }
          else
#endif
            {
#if 0
              fprintf (stderr, "[get_connection] HEY BOSS!\n");
#endif
              NETCLOSE (scan->socket);
              goto next_step;
            }
        } else {
          if (TRACE) fprintf(stderr, 
                             "FTP: Existing connection is %d.%d.%d.%d\n",
                             (int)*((unsigned char *)(&scan->addr)+0),
                             (int)*((unsigned char *)(&scan->addr)+1),
                             (int)*((unsigned char *)(&scan->addr)+2),
                             (int)*((unsigned char *)(&scan->addr)+3));
        }
      }
    }

  next_step:
   
/*	Now, let's get a socket set up from the server:
*/      
    {
        int status;
	connection * con = (connection *)malloc(sizeof(*con));
	if (con == NULL) outofmem(__FILE__, "get_connection");
	con->addr = sin->sin_addr.s_addr;	/* save it */
	con->binary = NO;
	status = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (status<0) {
	    (void) HTInetStatus("socket");
	    free(con);
	    if (username) free(username);
#ifdef DEBUG
            fprintf (stderr, "[get_connection TWO: return]\n");
#endif
	    return status;
	}
	con->socket = status;

    	status = connect(con->socket, (struct sockaddr*)&soc_address,
	 sizeof(soc_address));
        if (status<0){
	    (void) HTInetStatus("connect");
	    if (TRACE) fprintf(stderr, 
	    	"FTP: Unable to connect to remote host for `%s'.\n",
	    	arg);
	    NETCLOSE(con->socket);
	    free(con);
	    if (username) free(username);
#ifdef DEBUG
            fprintf (stderr, "[get_connection THREE: return]\n");
#endif
	    return status;			/* Bad return */
	}
	
	if (TRACE) fprintf(stderr, "FTP connected, socket %d\n", con->socket);
	control = con;			/* Current control connection */
	con->next = connections;	/* Link onto list of good ones */
	connections = con;
        HTInitInput(con->socket);/* Initialise buffering for contron connection */


/*	Now we log in		Look up username, prompt for pw.
*/
	{
	    int status = response(NIL);	/* Get greeting */
	    
	    if (status == 2) {		/* Send username */
	        char * command;
		if (username) {
		    command = (char*)malloc(10+strlen(username)+2+1);
		    if (command == NULL) outofmem(__FILE__, "get_connection");
		    sprintf(command, "USER %s\r\n", username);
		} else {
		    command = (char*)malloc(25);
		    if (command == NULL) outofmem(__FILE__, "get_connection");
		    sprintf(command, "USER anonymous\r\n");
	        }
		status = response(command);
		free(command);
	    }
	    if (status == 3) {		/* Send password */
	        char * command;
		if (password) {
		    command = (char*)malloc(10+strlen(password)+2+1);
		    if (command == NULL) outofmem(__FILE__, "get_connection");
		    sprintf(command, "PASS %s\r\n", password);
		} else {
#if 0
#ifdef ANON_FTP_HOSTNAME
		    command = (char*)malloc(20+strlen(HTHostName())+2+1);
		    if (command == NULL) outofmem(__FILE__, "get_connection");
		    sprintf(command,
		    "PASS WWWuser@%s\r\n", HTHostName()); /*@@*/
#else
		    /* In fact ftp.uu.net for example prefers just "@"
		    	the fulle domain name, which we can't get - DD */
		    command = (char*)malloc(20);
		    if (command == NULL) outofmem(__FILE__, "get_connection");
		    sprintf(command, "PASS WWWuser@\r\n"); /*@@*/
#endif
#endif
                    {
                      /* Mosaic temporary solution. */
                      extern char *machine_with_domain;
                      command = 
                        (char*)malloc(20+strlen(machine_with_domain)+2+1);
                      if (command == NULL) 
                        outofmem(__FILE__, "get_connection");
                      sprintf(command,
                              "PASS WWWuser@%s\r\n", 
                              machine_with_domain); /*@@*/
                    }
	        }
		status = response(command);
		free(command);
	    }
            if (username) free(username);

            /* Switch to binary mode. */
            {
              char *command;
              command = (char *)malloc (32);
              sprintf (command, "TYPE I\r\n");
              response(command);
              free (command);
            }

	    if (status == 3) status = response("ACCT noaccount\r\n");
	    
	    if (status !=2) {
	        if (TRACE) fprintf(stderr, "FTP: Login fail: %s", response_text);
	    	if (control) close_connection(control);
#ifdef DEBUG
                fprintf (stderr, "[get_connection FOUR: return]\n");
#endif
	        return -1;		/* Bad return */
	    }
	    if (TRACE) fprintf(stderr, "FTP: Logged in.\n");
	}

/*	Now we inform the server of the port number we will listen on
*/
#ifndef REPEAT_PORT
	{
	    int status = response(port_command);
	    if (status !=2) {
	    	if (control) close_connection(control);
#ifdef DEBUG
                fprintf (stderr, "[get_connection FIVE: return]\n");
#endif
	        return -status;		/* Bad return */
	    }
	    if (TRACE) fprintf(stderr, "FTP: Port defined.\n");
	}
#endif
#ifdef DEBUG
        fprintf (stderr, "[get_connection SIX: return]\n");
#endif
	return con->socket;			/* Good return */
    } /* Scope of con */
}


#ifdef LISTEN

/*	Close Master (listening) socket
**	-------------------------------
**
**
*/
#ifdef __STDC__
PRIVATE int close_master_socket(void)
#else
PRIVATE int close_master_socket()
#endif
{
    int status;
    FD_CLR(master_socket, &open_sockets);
    status = NETCLOSE(master_socket);
    if (TRACE) fprintf(stderr, "FTP: Closed master socket %d\n", master_socket);
    master_socket = -1;
    if (status<0) return HTInetStatus("close master socket");
    else return status;
}


/*	Open a master socket for listening on
**	-------------------------------------
**
**	When data is transferred, we open a port, and wait for the server to
**	connect with the data.
**
** On entry,
**	master_socket	Must be negative if not set up already.
** On exit,
**	Returns		socket number if good
**			less than zero if error.
**	master_socket	is socket number if good, else negative.
**	port_number	is valid if good.
*/
#ifdef __STDC__
PRIVATE int get_listen_socket(void)
#else
PRIVATE int get_listen_socket()
#endif
{
    struct sockaddr_in soc_address;	/* Binary network address */
    struct sockaddr_in* sin = &soc_address;
    int new_socket;			/* Will be master_socket */
    
    
    FD_ZERO(&open_sockets);	/* Clear our record of open sockets */
    num_sockets = 0;
    
#ifndef REPEAT_LISTEN
    if (master_socket>=0) return master_socket;  /* Done already */
#endif

/*  Create internet socket
*/
    new_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
    if (new_socket<0)
	return HTInetStatus("socket for master socket");
    
    if (TRACE) fprintf(stderr, "FTP: Opened master socket number %d\n", new_socket);
    
/*  Search for a free port.
*/
    sin->sin_family = AF_INET;	    /* Family = internet, host order  */
    sin->sin_addr.s_addr = INADDR_ANY; /* Any peer address */
#ifdef POLL_PORTS
    {
        unsigned short old_port_number = port_number;
	for(port_number=old_port_number+1;;port_number++){ 
	    int status;
	    if (port_number > LAST_TCP_PORT)
		port_number = FIRST_TCP_PORT;
	    if (port_number == old_port_number) {
		return HTInetStatus("bind");
	    }
	    soc_address.sin_port = htons(port_number);
	    if ((status=bind(new_socket,
		    (struct sockaddr*)&soc_address,
			    /* Cast to generic sockaddr */
		    sizeof(soc_address))) == 0)
		break;
	    if (TRACE) fprintf(stderr, 
	    	"TCP bind attempt to port %d yields %d, errno=%d\n",
		port_number, status, errno);
	} /* for */
    }
#else
    {
        int status;
	int address_length = sizeof(soc_address);
	status = getsockname(control->socket,
			(struct sockaddr *)&soc_address,
			 &address_length);
	if (status<0) return HTInetStatus("getsockname");
	CTRACE(tfp, "FTP: This host is %s\n",
	    HTInetString(sin));
	
	soc_address.sin_port = 0;	/* Unspecified: please allocate */
	status=bind(new_socket,
		(struct sockaddr*)&soc_address,
			/* Cast to generic sockaddr */
		sizeof(soc_address));
	if (status<0) return HTInetStatus("bind");
	
	address_length = sizeof(soc_address);
	status = getsockname(new_socket,
			(struct sockaddr*)&soc_address,
			&address_length);
	if (status<0) return HTInetStatus("getsockname");
    }
#endif    

    CTRACE(tfp, "FTP: bound to port %d on %s\n",
    		(int)ntohs(sin->sin_port),
		HTInetString(sin));

#ifdef REPEAT_LISTEN
    if (master_socket>=0)
        (void) close_master_socket();
#endif    
    
    master_socket = new_socket;
    
/*	Now we must find out who we are to tell the other guy
*/
    (void)HTHostName(); 	/* Make address valid - doesn't work*/
    sprintf(port_command, "PORT %d,%d,%d,%d,%d,%d\r\n",
		    (int)*((unsigned char *)(&sin->sin_addr)+0),
		    (int)*((unsigned char *)(&sin->sin_addr)+1),
		    (int)*((unsigned char *)(&sin->sin_addr)+2),
		    (int)*((unsigned char *)(&sin->sin_addr)+3),
		    (int)*((unsigned char *)(&sin->sin_port)+0),
		    (int)*((unsigned char *)(&sin->sin_port)+1));


/*	Inform TCP that we will accept connections
*/
    if (listen(master_socket, 1)<0) {
	master_socket = -1;
	return HTInetStatus("listen");
    }
    CTRACE(tfp, "TCP: Master socket(), bind() and listen() all OK\n");
    FD_SET(master_socket, &open_sockets);
    if ((master_socket+1) > num_sockets) num_sockets=master_socket+1;

    return master_socket;		/* Good */

} /* get_listen_socket */
#endif


/*	Get Styles from stylesheet
**	--------------------------
*/
PRIVATE void get_styles NOARGS
{
    if (!h1Style) h1Style = HTStyleNamed(styleSheet, "Heading1");
    if (!h2Style) h2Style = HTStyleNamed(styleSheet, "Heading2");
    if (!textStyle) textStyle = HTStyleNamed(styleSheet, "Example");
    if (!dirStyle) dirStyle = HTStyleNamed(styleSheet, "Dir");
}


/*	Read a directory into an hypertext object from the data socket
**	--------------------------------------------------------------
**
** On entry,
**	anchor		Parent anchor to link the HText to
**	address		Address of the directory
** On exit,
**	returns		HT_LOADED if OK
**			<0 if error.
*/
PRIVATE int read_directory
ARGS2 (
  HTParentAnchor *,	parent,
  CONST char *,			address
)
{
  HText * HT = HText_new (parent);
  char *filename = HTParse(address, "", PARSE_PATH + PARSE_PUNCTUATION);
  HTChildAnchor * child;  /* corresponds to an entry of the directory */
  char c = 0;
#define LASTPATH_LENGTH 150  /* @@@@ Horrible limit on the entry size */
  char lastpath[LASTPATH_LENGTH + 1];
  char *p, *entry;
  int lastpath_nulled = 0;

  HText_beginAppend (HT);
  /* get_styles(); */

  HTAnchor_setTitle (parent, "FTP Directory of ");
  HTAnchor_appendTitle (parent, address + 5);  /* +5 gets rid of "file:" */
  /* HText_setStyle (HT, h1Style); */
  HText_appendText (HT, "<H1>FTP Directory ");
  HText_appendText (HT, filename);

  HText_appendText (HT, "</H1>\n");
  /* HText_setStyle (HT, dirStyle); */
  HText_appendText (HT, "<UL>\n");
  data_read_pointer = data_write_pointer = data_buffer;

  if (*filename == 0)  /* Empty filename : use root */
    strcpy (lastpath, "/");
  else {
    p = filename + strlen (filename) - 2;
    while (p >= filename && *p != '/')
      p--;
    strcpy (lastpath, p + 1);  /* relative path */
  }
  free (filename);

  /* But what if lastpath is foo/ (with trailing slash)
     because URL was something like ftp://host/blargh/foo/ ?
     Then the directory entries resolve to, 
       ftp://host/blargh/foo/foo/something
     instead of
       ftp://host/blargh/foo/something
     like they should.
     So if we pick up a trailing slash in lastpath,
     we nullify lastpath. */
  if (lastpath[strlen(lastpath)-1] == '/')
    {
      /* Remember that we want a Parent Directory entry anyway. */
      if (strlen(lastpath) > 1)
        lastpath_nulled = 1;
      lastpath[0] = '\0';
    }

#if 0
  entry = lastpath + strlen (lastpath);
  if (*(entry-1) != '/')
    *entry++ = '/';  /* ready to append entries */
#endif
  entry = lastpath + strlen (lastpath);
  if (entry != lastpath && *(entry-1) != '/')
    {
      *entry++ = '/';  /* ready to append entries */
      *(entry) = '\0'; /* gotta terminate the string */
    }

  if (strlen (lastpath) > 1 || lastpath_nulled) {  /* Current file is not the FTP root */
    strcpy (entry, "../");
    child = HTAnchor_findChildAndLink (parent, 0, lastpath, 0);
    /* HText_beginAnchor (HT, child); */
    HText_appendText (HT, "<LI> ");
    HText_appendText (HT, "<A HREF=\"");
    HText_appendText (HT, lastpath);
    HText_appendText (HT, "\">");
    HText_appendText (HT, "Parent Directory");
    /* HText_endAnchor (HT); */
    HText_appendText (HT, "</A>\n");
    /* HText_appendCharacter (HT, '\t'); */
  }
  for (;;) {
    p = entry;
    while (p - lastpath < LASTPATH_LENGTH) {
      c = NEXT_DATA_CHAR;
      if (c == '\r') {
	c = NEXT_DATA_CHAR;
	if (c != '\n') {
	  if (TRACE)
	    printf ("Warning: No newline but %d after carriage return.\n", c);
	  break;
	}
      }
      if (c == '\n' || c == (char) EOF)
	break;
      *p++ = c;
    }
    if (c == (char) EOF && p == entry)
      break;
    *p = 0;
    child = HTAnchor_findChildAndLink (parent, 0, lastpath, 0);

    /* HText_beginAnchor (HT, child); */
    HText_appendText (HT, "<LI> ");
    HText_appendText (HT, "<A HREF=\"");
    HText_appendText (HT, lastpath);
    HText_appendText (HT, "\">");
    HText_appendText (HT, entry);
    /* HText_endAnchor (HT); */
    HText_appendText (HT, "</A>\n");
    /* HText_appendCharacter (HT, '\t'); */
  }

  HText_appendText (HT, "</UL>\n");
  HText_endAppend (HT);
  return response(NIL) == 2 ? HT_LOADED : -1;
}


/*	Retrieve File from Server
**	-------------------------
**
** On entry,
**	name		WWW address of a file: document, including hostname
** On exit,
**	returns		Socket number for file if good.
**			<0 if bad.
*/
PUBLIC int HTFTP_open_file_read
ARGS2 (
  CONST char *,			name,
  HTParentAnchor *,	anchor
)
{
    BOOL isDirectory = NO;
    int status;
    int retry;			/* How many times tried? */
    
    for (retry=0; retry<2; retry++) {	/* For timed out/broken connections */
    
	status = get_connection(name, 1);
	if (status<0) 
          {
            /* This time, don't allow reuse. */
            status = get_connection(name, 0);
            if (status < 0)
              return status;
          }

#ifdef LISTEN
	status = get_listen_socket();
	if (status<0) return status;
    
#ifdef REPEAT_PORT
/*	Inform the server of the port number we will listen on
*/
	{
	    status = response(port_command);
	    if (status !=2) {		/* Could have timed out */
		if (status<0) continue;		/* try again - net error*/
		return -status;			/* bad reply */
	    }
	    if (TRACE) fprintf(stderr, "FTP: Port defined.\n");
	}
#endif
#else	/* Use PASV */
/*	Tell the server to be passive
*/
	{
	    char *p;
	    int reply, h0, h1, h2, h3, p0, p1;	/* Parts of reply */
	    status = response("PASV\r\n");
	    if (status !=2) {
		if (status<0) continue;		/* retry or Bad return */
		return -status;			/* bad reply */
	    }
	    for(p=response_text; *p; p++)
		if ((*p<'0')||(*p>'9')) *p = ' ';	/* Keep only digits */
	    status = sscanf(response_text, "%d%d%d%d%d%d%d",
		    &reply, &h0, &h1, &h2, &h3, &p0, &p1);
	    if (status<5) {
		if (TRACE) fprintf(stderr, "FTP: PASV reply has no inet address!\n");
		return -99;
	    }
	    passive_port = (p0<<8) + p1;
	    if (TRACE) fprintf(stderr, "FTP: Server is listening on port %d\n",
		    passive_port);
	}

/*	Open connection for data:
*/
	{
	    struct sockaddr_in soc_address;
	    int status = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	    if (status<0) {
		(void) HTInetStatus("socket for data socket");
		return status;
	    }
	    data_soc = status;
	    
	    soc_address.sin_addr.s_addr = control->addr;
	    soc_address.sin_port = htons(passive_port);
	    soc_address.sin_family = AF_INET;	    /* Family, host order  */
	    if (TRACE) fprintf(stderr,  
		"FTP: Data remote address is port %d, inet %d.%d.%d.%d\n",
			(unsigned int)ntohs(soc_address.sin_port),
			(int)*((unsigned char *)(&soc_address.sin_addr)+0),
			(int)*((unsigned char *)(&soc_address.sin_addr)+1),
			(int)*((unsigned char *)(&soc_address.sin_addr)+2),
			(int)*((unsigned char *)(&soc_address.sin_addr)+3));
    
	    status = connect(data_soc, (struct sockaddr*)&soc_address,
		    sizeof(soc_address));
	    if (status<0){
		(void) HTInetStatus("connect for data");
		NETCLOSE(data_soc);
		return status;			/* Bad return */
	    }
	    
	    if (TRACE) fprintf(stderr, "FTP data connected, socket %d\n", data_soc);
	}
#endif /* use PASV */
	status = 0;
        break;	/* No more retries */

    } /* for retries */
    if (status<0) return status;	/* Failed with this code */
    
/*	Ask for the file:
*/    
    {
        char *filename = HTParse(name, "", PARSE_PATH + PARSE_PUNCTUATION);
	char command[LINE_LENGTH+1];
	if (!*filename) StrAllocCopy(filename, "/");
	sprintf(command, "RETR %s\r\n", filename);
	status = response(command);
	if (status != 1) {  /* Failed : try to CWD to it */
	  sprintf(command, "CWD %s\r\n", filename);
	  status = response(command);
	  if (status == 2) {  /* Successed : let's NAME LIST it */
	    isDirectory = YES;
	    sprintf(command, "NLST\r\n");
	    status = response (command);
	  }
	}
	free(filename);
	if (status != 1) return -status;		/* Action not started */
    }

#ifdef LISTEN
/*	Wait for the connection
*/
    {
	struct sockaddr_in soc_address;
        int	soc_addrlen=sizeof(soc_address);
	status = accept(master_socket,
			(struct sockaddr *)&soc_address,
			&soc_addrlen);
	if (status<0)
	    return HTInetStatus("accept");
	CTRACE(tfp, "TCP: Accepted new socket %d\n", status);
	data_soc = status;
    }
#else
/* @@ */
#endif
    if (isDirectory)
      return read_directory (anchor, name);
      /* returns HT_LOADED or error */
    else
      return data_soc;
       
} /* open_file_read */


/*	Close socket opened for reading a file, and get final message
**	-------------------------------------------------------------
**
*/
PUBLIC int HTFTP_close_file
ARGS1 (int,soc)
{
    int status;

    if (soc!=data_soc) {
        if (TRACE) fprintf(stderr, 
	"HTFTP: close socket %d: (not FTP data socket).\n",
		soc);
	return NETCLOSE(soc);
    }

    HTInitInput(control->socket);
    /* Reset buffering to control connection 921208 */

    status = NETCLOSE(data_soc);
    if (TRACE) fprintf(stderr, "FTP: Closing data socket %d\n", data_soc);
    if (status<0) (void) HTInetStatus("close");	/* Comment only */
    data_soc = -1;	/* invalidate it */
    
    status = response(NIL);
    if (status!=2) return -status;
    
    return status; 	/* Good */
}


/*___________________________________________________________________________
*/
/*	Test program only
**	-----------------
**
**	Compiling this with -DTEST produces a test program.  Unix only
**
** Syntax:
**	test <file or option> ...
**
**	options:	-v	Verbose: Turn trace on at this point
*/
#ifdef TEST

PUBLIC int WWW_TraceFlag;

#ifdef __STDC__
int main(int argc, char*argv[])
#else
int main(argc, argv)
	int argc;
	char *argv[];
#endif
{
    
    int arg;			/* Argument number */
    int status;
    connection * con;
    WWW_TraceFlag = (0==strcmp(argv[1], "-v"));	/* diagnostics ? */

#ifdef SUPRESS    
    status = get_listen_socket();
    if (TRACE) fprintf(stderr, "get_listen_socket returns %d\n\n", status);
    if (status<0) exit(status);

    status = get_connection(argv[1], 1);	/* test double use */
    if (TRACE) fprintf(stderr, "get_connection(`%s') returned %d\n\n", argv[1], status);
    if (status<0) exit(status);
#endif

    for(arg=1; arg<argc; arg++) {		/* For each argument */
         if (0==strcmp(argv[arg], "-v")) {
             WWW_TraceFlag = 1;			/* -v => Trace on */

	 } else {				/* Filename: */
	
	    status = HTTP_open_file_read(argv[arg]);
	    if (TRACE) fprintf(stderr, "open_file_read returned %d\n\n", status);
	    if (status<0) exit(status);
		
	/*	Copy the file to std out:
	*/   
	    {
		char buffer[INPUT_BUFFER_SIZE];
		for(;;) {
		    int status = NETREAD(data_soc, buffer, INPUT_BUFFER_SIZE);
		    if (status<=0) {
			if (status<0) (void) HTInetStatus("read");
			break;
		    }
		    status = write(1, buffer, status);
		    if (status<0) {
			fprintf(stderr, "Write failure!\n");
			break;
		    }
		}
	    }
	
	    status = HTTP_close_file(data_soc);
	    if (TRACE) fprintf(stderr, "Close_file returned %d\n\n", status);

        } /* if */
    } /* for */
    status = response("QUIT\r\n");		/* Be good */
    if (TRACE) fprintf(stderr, "Quit returned %d\n", status);

    while(connections) close_connection(connections);
    
    close_master_socket();
         
} /* main */
#endif  /* TEST */

