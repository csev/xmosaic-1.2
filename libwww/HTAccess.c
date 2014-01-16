/*		Access Manager					HTAccess.c
**		==============
**
**       8 Jun 92 Telnet hopping prohibited as telnet is not secure TBL
**	26 Jun 92 When over DECnet, suppressed FTP, Gopher and News. JFG
**	 6 Oct 92 Moved HTClientHost and logfile into here. TBL
*/


#include "HTParse.h"
#include "HTUtils.h"
#include "WWW.h"
#include "HTAnchor.h"
#include "HTTP.h"
#include "HTFile.h"
#include <errno.h>
#include <stdio.h>

#include "tcp.h"
#include "HText.h"
#ifndef DECNET
#include "HTFTP.h"
#include "HTGopher.h"
#include "HTNews.h"
#endif
/* #include "HTBrowse.h"	 Needed global HTClientHost */

#include "HTAccess.h"

#define HT_NO_DATA -9999

/*	These flags may be set to modify the operation of this module
*/
PUBLIC int HTDiag = 0;		/* Diagnostics: load source as text */
PUBLIC char * HTClientHost = 0;	/* Name of remote login host if any */
PUBLIC FILE * logfile = 0;	/* File to output one-liners to */

/* #define TRACE 1 */

#ifdef _NO_PROTO
            extern void application_user_feedback ();
#else
            extern void application_user_feedback (char *);
#endif


/*	Telnet or "rlogin" access
**	-------------------------
*/
PRIVATE int remote_session ARGS2(char *, access, char *, host)
{
	char * user;
	char * hostname;
	char * port;
	char   command[256];
	BOOL rlogin;
        char *xterm_str;

        user = host;
        if (!host)
          {
            application_user_feedback ("Cannot open remote session, because\nUniversal Resource Locator is malformed.\0");
            return HT_NO_DATA;
          }
            
        hostname = strchr(host, '@');
        port = strchr(host, ':');
        rlogin = !strcmp(access, "rlogin");
        
	if (hostname) {
	    *hostname++ = 0;	/* Split */
	} else {
	    hostname = host;
	    user = 0;		/* No user specified */
	}
	if (port) *port++ = 0;	/* Split */


/* If the person is already telnetting etc, forbid hopping */
/* This is a security precaution, for us and remote site */

	if (HTClientHost) {

	    sprintf(command, 
	      "finger @%s | mail -s \"**telnethopper %s\" tbl@dxcern.cern.ch",
	       HTClientHost, HTClientHost);
	    system(command);

	    printf("\n\nSorry, but the service you have selected is one\n");
	    printf("to which you have to log in.  If you were running www\n");
	    printf("on your own computer, you would be automatically connected.\n");
	    printf("For security reasons, this is not allowed when\n");
	    printf("you log in to this information service remotely.\n\n");

	    printf("You can manually connect to this service using %s\n",
		   access);
	    printf("to host %s", hostname);
	    if (user) printf(", user name %s", user);
	    if (port) printf(", port %s", port);
	    printf(".\n\n");
	    return HT_NO_DATA;
	}

/* This used to check for 'unix', which isn't safe. */
#ifndef VMS
        xterm_str = global_xterm_str;

        if (rlogin)
          {
            /* For rlogin, we should use -l user. */
            sprintf(command, "%s -e %s %s %s %s %s &", xterm_str, access,
                    hostname,
                    port ? port : "",
                    user ? "-l" : "",
                    user ? user : "");
          }
        else
          {
            /* For telnet, -l isn't safe to use at all -- most platforms
               don't understand it. */
            sprintf(command, "%s -e %s %s %s &", xterm_str, access,
                    hostname,
                    port ? port : "");
          }

	if (TRACE) fprintf(stderr, "HTaccess: Command is: %s\n", command);
	system(command);

        /* No need for application feedback if we're rlogging directly
           in... */
        if (user && !rlogin)
          {
            char str[200];
            /* Sleep to let the xterm get up first.
               Otherwise, the popup will get buried. */
            sleep (2);
            sprintf (str, "When you are connected, log in as '%s'.\0", user);
            application_user_feedback (str);
          }
	
	return HT_NO_DATA;		/* Ok - it was done but no data */
#define TELNET_DONE
#endif

#ifdef MULTINET				/* VMS varieties */
	if (!rlogin) {			/* telnet */
	    sprintf(command, "TELNET %s%s %s",
		port ? "/PORT=" : "",
		port ? port : "",
		hostname);
	} else {
	    sprintf(command, "RLOGIN%s%s%s%s %s", access,
		user ? "/USERNAME=" : "",
		user ? user : "",
		port ? "/PORT=" : "",
		port ? port : "",
		hostname);
	}
	if (TRACE) fprintf(stderr, "HTaccess: Command is: %s\n", command);
	system(command);

        if (user) 
          {
            char str[200];
            sleep (2);
            sprintf (str, "When you are connected, log in as '%s'.\0", user);
            application_user_feedback (str);
          }

	return HT_NO_DATA;		/* Ok - it was done but no data */
#define TELNET_DONE
#endif

#ifdef UCX
#define SIMPLE_TELNET
#endif
#ifdef VM
#define SIMPLE_TELNET
#endif
#ifdef SIMPLE_TELNET
	if (!rlogin) {			/* telnet only */
	    sprintf(command, "TELNET  %s",	/* @@ Bug: port ignored */
		hostname);
	    if (TRACE) fprintf(stderr, "HTaccess: Command is: %s\n", command);
	    system(command);

            if (user) 
              {
                char str[200];
                sleep (2);
                sprintf (str, "When you are connected, log in as '%s'.\0", user);
                application_user_feedback (str);
              }
            
	    return HT_NO_DATA;		/* Ok - it was done but no data */
	}
#endif

#ifndef TELNET_DONE
	fprintf(stderr,
	"Sorry, this browser was compiled without the %s access option.\n",
		access);
	fprintf(stderr,
	"\nTo access the information you must %s to %s", access, hostname);
	if (port) fprintf(stderr," (port %s)", port);
	if (user) fprintf(stderr," logging in with username %s", user);
	fprintf(stderr, ".\n");
	return -1;
#endif
}



/*	Telnet or "rlogin" access
**	-------------------------
*/
PRIVATE int remote_3270_session ARGS2(char *, access, char *, host)
{
	char * user;
	char * hostname;
	char * port;
	char   command[256];
        char *xterm_str;

        user = host;
        if (!host)
          {
            application_user_feedback ("Cannot open remote session, because\nUniversal Resource Locator is malformed.\0");
            return HT_NO_DATA;
          }
            
        hostname = strchr(host, '@');
        port = strchr(host, ':');
	
	if (hostname) {
	    *hostname++ = 0;	/* Split */
	} else {
	    hostname = host;
	    user = 0;		/* No user specified */
	}
	if (port) *port++ = 0;	/* Split */

/* If the person is already telnetting etc, forbid hopping */
/* This is a security precaution, for us and remote site */

	if (HTClientHost) {

	    sprintf(command, 
	      "finger @%s | mail -s \"**telnethopper %s\" tbl@dxcern.cern.ch",
	       HTClientHost, HTClientHost);
	    system(command);

	    printf("\n\nSorry, but the service you have selected is one\n");
	    printf("to which you have to log in.  If you were running www\n");
	    printf("on your own computer, you would be automatically connected.\n");
	    printf("For security reasons, this is not allowed when\n");
	    printf("you log in to this information service remotely.\n\n");

	    printf("You can manually connect to this service using %s\n",
		   access);
	    printf("to host %s", hostname);
	    if (user) printf(", user name %s", user);
	    if (port) printf(", port %s", port);
	    printf(".\n\n");
	    return HT_NO_DATA;
	}

/* Not all telnet servers get it even if user name is specified
** so we always tell the guy what to log in as
*/
        if (user)
          {
            char str[200];
            /* Sleep to let the xterm get up first.
               Otherwise, the popup will get buried. */
            sleep (2);
            sprintf (str, "When you are connected, log in as '%s'.\0", user);
            application_user_feedback (str);
          }

        xterm_str = global_xterm_str;

        /* For tn3270, -l isn't safe to use at all -- most platforms
           don't understand it. */
        sprintf(command, "%s -e %s %s %s &", xterm_str, access,
                hostname,
                port ? port : "");

	if (TRACE) fprintf(stderr, "HTaccess: Command is: %s\n", command);
	system(command);
	return HT_NO_DATA;		/* Ok - it was done but no data */
}

/*	Open a file descriptor for a document
**	-------------------------------------
**
** On entry,
**	addr		must point to the fully qualified hypertext reference.
**
** On exit,
**	returns		<0	Error has occured.
**			>=0	Value of file descriptor or socket to be used
**				 to read data.
**	*pFormat	Set to the format of the file, if known.
**			(See WWW.h)
**
*/
PRIVATE int HTOpen ARGS4(
	CONST char *,addr1,
	HTFormat *,pFormat,
	HTParentAnchor *,anchor,
        int *,compressed)
{
    char * access=0;	/* Name of access method */
    int status;
    char * gateway;
    char * gateway_parameter;
    char * addr = (char *)malloc(strlen(addr1)+1);
    
    if (addr == NULL) outofmem(__FILE__, "HTOpen");
    strcpy(addr, addr1);			/* Copy to play with */
    
    access =  HTParse(addr, "file:", PARSE_ACCESS);

    /* It is possible that we will end up here without an access
       method -- I'm not sure how, but it happens; e.g, for
       URL "://cbl.leeds.ac.uk/". */
    if (access == NULL || (access && *access == '\0'))
      {
        application_user_feedback ("Cannot access remote document, since\nUniversal Resource Locator is malformed.");
        free (addr);
        status = -1;
        return status;
      }

    gateway_parameter = (char *)malloc(strlen(access)+20);
    if (gateway_parameter == NULL) outofmem(__FILE__, "HTOpen");
    strcpy(gateway_parameter, "WWW_");
    strcat(gateway_parameter, access);
    strcat(gateway_parameter, "_GATEWAY");
    gateway = (char *)getenv(gateway_parameter); /* coerce for decstation */
    free(gateway_parameter);
    
    if (gateway) 
      {
	status = HTLoadHTTP(addr, gateway, anchor, HTDiag);
	if (status<0) 
          {
            char msg[300];
            sprintf 
              (msg, 
               "Cannot connect to information gateway\n%s\n",
               gateway);
            application_user_feedback (msg);
          }
      } 
    else if (0==strcmp(access, "http")) 
      {
        status = HTLoadHTTP(addr, 0, anchor, HTDiag);
	if (status<0) 
          {
            application_user_feedback 
              ("Cannot connect to information server.");
          }
      } 
    else if ((0==strcmp(access, "file"))
             ||(0==strcmp(access, "ftp"))) 
      {	/* TBL 921021 */
        status = HTOpenFile(addr, pFormat, anchor, compressed);
#ifndef DECNET
      } 
    else if (0==strcmp(access, "news")) 
      {
        status = HTLoadNews(addr, anchor, HTDiag);
	if (status>0) status = HT_LOADED;
      } 
    else if (0==strcmp(access, "gopher")) 
      {
        status = HTLoadGopher(addr, anchor, HTDiag);
	if (status>0) status = HT_LOADED;
#endif
      } 
    else if (!strcmp(access, "telnet") ||		/* TELNET */
             !strcmp(access, "rlogin")) 
      {			/* RLOGIN */
        char * host = HTParse(addr, "", PARSE_HOST);
        remote_session(access, host);
        status = HT_LOADED;
	free(host);
      } 
    else if (!strcmp(access, "tn3270")) 
      {              /* TN3270 */
        char * host = HTParse(addr, "", PARSE_HOST);
	remote_3270_session(access, host);
        status = HT_LOADED;
	free(host);
      } 
    else if (0==strcmp(access, "wais")) 
      {
        application_user_feedback ("HTAccess: For WAIS access set WWW_wais_GATEWAY to gateway address.\0");
        status = -1;
      } 
    else 
      {
        char msg[200];
        
        sprintf 
          (msg,
           "Name scheme `%s'\nunknown by this version of NCSA Mosaic.\0", 
           access);
        application_user_feedback (msg);
        status = -1;
      }
    if (access)
      free(access);
    if (addr)
      free(addr);
    return status;
}


/*	Close socket opened for reading a file
**	--------------------------------------
**
*/
PRIVATE int HTClose ARGS1(int,soc)
{
#ifdef DECNET  /* In that case, soc can only be a file descriptor */
    return NETCLOSE (soc);
#else
    return HTFTP_close_file(soc);
#endif
}


/*		Load a document
**		---------------
**
**    On Entry,
**	  anchor	    is the node_anchor for the document
**        full_address      The address of the document to be accessed.
**        filter            if YES, treat document as HTML
**
**    On Exit,
**        returns    YES     Success in opening document
**                   NO      Failure 
**
*/

PUBLIC BOOL HTLoadDocument ARGS3(HTParentAnchor *,anchor,
	CONST char *,full_address,
	BOOL,	filter)

{
    int	        new_file_number;
    HTFormat    format;
    HText *	text;
    int compressed;

    /* Assume the file isn't compressed. */
    compressed = 0;

    if (TRACE) fprintf (stderr,
      "HTAccess: loading document %s\n", full_address);

#if 0
    if (text=(HText *)HTAnchor_document(anchor)) {	/* Already loaded */
        if (TRACE) fprintf(stderr, "HTAccess: Document already in memory.\n");
        HText_select(text);
	return YES;
    }
#endif
    
#ifdef CURSES
      prompt_set("Retrieving document...");
#endif
    if (filter) {
        new_file_number = 0;
	format = WWW_HTML;
    } else {
	new_file_number = HTOpen(full_address, &format, anchor, &compressed);
    }
/*	Log the access if necessary
*/
    if (logfile) {
	time_t theTime;
	time(&theTime);
	fprintf(logfile, "%24.24s %s %s %s\n",
	    ctime(&theTime),
	    HTClientHost ? HTClientHost : "local",
	    new_file_number<0 ? "FAIL" : "GET",
	    full_address);
	fflush(logfile);	/* Actually update it on disk */
	if (TRACE) fprintf(stderr, "Log: %24.24s %s %s %s\n",
	    ctime(&theTime),
	    HTClientHost ? HTClientHost : "local",
	    new_file_number<0 ? "FAIL" : "GET",
	    full_address);
    }
    
    if (new_file_number == HT_LOADED) {
	if (TRACE) {
	    fprintf(stderr, "HTAccess: `%s' has been accessed.\n",
	    full_address);
	}
	return YES;
    }
    
    if (new_file_number == HT_NO_DATA) {
	if (TRACE) {
	    fprintf(stderr, 
	    "HTAccess: `%s' has been accessed, No data left.\n",
	    full_address);
	}
	return NO;
    }
    
    if (new_file_number<0) {		      /* Failure in accessing a file */

#ifdef CURSES
        user_message("Can't access `%s'", full_address);
#else
	if (TRACE)
	    fprintf(stderr, "HTAccess: Can't access `%s'\n", full_address);
#endif

	return NO;
    }

    /* We just can't allow this to happen.  It hangs on stdin. */
    if (new_file_number == 0)
      return NO;
    
    if (TRACE) {
	fprintf(stderr, "WWW: Opened `%s' as fd %d\n",
	full_address, new_file_number);
    }

    HTParseFormat(HTDiag ? WWW_PLAINTEXT : format, anchor, new_file_number, 
                  compressed);

    HTClose(new_file_number);
    
    return YES;
    
} /* HTLoadDocument */


/*		Load a document from absolute name
**		---------------
**
**    On Entry,
**        addr     The absolute address of the document to be accessed.
**        filter   if YES, treat document as HTML
**
**    On Exit,
**        returns    YES     Success in opening document
**                   NO      Failure 
**
**
*/

PUBLIC BOOL HTLoadAbsolute ARGS2(CONST char *,addr, BOOL, filter)
{
   return HTLoadDocument(
       		HTAnchor_parent(HTAnchor_findAddress(addr)),
       		addr, filter);
}


/*		Load a document from relative name
**		---------------
**
**    On Entry,
**        relative_name     The relative address of the document to be accessed.
**
**    On Exit,
**        returns    YES     Success in opening document
**                   NO      Failure 
**
**
*/

PUBLIC BOOL HTLoadRelative ARGS1(CONST char *,relative_name)
{
    char * 		full_address = 0;
    BOOL       		result;
    char * 		mycopy = 0;
    char * 		stripped = 0;
    char *		current_address =
    				HTAnchor_address((HTAnchor*)HTMainAnchor);

    StrAllocCopy(mycopy, relative_name);

    stripped = HTStrip(mycopy);
    full_address = HTParse(stripped,
	           current_address,
		   PARSE_ACCESS|PARSE_HOST|PARSE_PATH|PARSE_PUNCTUATION);
    result = HTLoadAbsolute(full_address, NO);
    free(full_address);
    if (current_address && *current_address)
      {
        free(current_address);
        current_address = 0;
      }
    free(mycopy);  /* Memory leak fixed 10/7/92 -- JFG */
    return result;
}


/*		Load if necessary, and select an anchor
**		--------------------------------------
**
**    On Entry,
**        destination      	    The child or parenet anchor to be loaded.
**
**    On Exit,
**        returns    YES     Success
**                   NO      Failure 
**
*/

PUBLIC BOOL HTLoadAnchor ARGS1(HTAnchor *,destination)
{
    HTParentAnchor * parent;
    BOOL loaded = NO;
    if (!destination) return NO;	/* No link */
    
    parent  = HTAnchor_parent(destination);
    
    if (HTAnchor_document(parent) == NULL) {	/* If not alread loaded */
    						/* TBL 921202 */
/*  if ( parent != HTMainAnchor) {    before If not already loaded */
        BOOL result;
        char * address = HTAnchor_address((HTAnchor*) parent);
	result = HTLoadDocument(parent, address, NO);
        if (address && *address)
          {
            free(address);
            address = 0;
          }
	if (!result) return NO;
	loaded = YES;
    }
    
    {
	HText *text = (HText*)HTAnchor_document(parent);
	if (destination != (HTAnchor *)parent) {  /* If child anchor */
	    HText_selectAnchor(text, 
		    (HTChildAnchor*)destination); /* Double display? @@ */
	} else {
	    if (!loaded) HText_select(text);
	}
    }
    return YES;
	
} /* HTLoadAnchor */


/*		Search
**		------
**  Performs a keyword search on word given by the user. Adds the keyword to 
**  the end of the current address and attempts to open the new address.
**
**  On Entry,
**       *keywords  	space-separated keyword list or similar search list
**	HTMainAnchor	global must be valid.
*/

PUBLIC BOOL HTSearch ARGS1(char *, keywords)

{
    char * p;	          /* pointer to first non-blank */
    char * q, *s;
    char * address = HTAnchor_address((HTAnchor*)HTMainAnchor);
    BOOL result;
    
    p = HTStrip(keywords);
    for (q=p; *q; q++)
        if (WHITE(*q)) {
	    *q = '+';
	}

    s=strchr(address, '?');		/* Find old search string */
    if (s) *s = 0;			        /* Chop old search off */

    StrAllocCat(address, "?");
    StrAllocCat(address, p);

    result = HTLoadRelative(address);
    if (address && *address)
      {
        free(address);
        address = 0;
      }
    return result;
    
}

