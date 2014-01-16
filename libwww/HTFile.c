/*			File Access				HTFile.c
**			===========
**
**	This is unix-specific code in general, with some VMS bits.
**	These are routines for file access used by WWW browsers.
**
** History:
**	   Feb 91	Written Tim Berners-Lee CERN/CN
**	   Apr 91	vms-vms access included using DECnet syntax
**	26 Jun 92 (JFG) When running over DECnet, suppressed FTP.
**			Fixed access bug for relative names on VMS.
**
** Bugs:
**	Cannot access VMS files from a unix machine. How can we know that the
**	target machine runs VMS?
*/

#define INFINITY 512		/* file name length @@ FIXME */

#include "HTUtils.h"
#include "HTFile.h"

#include "WWW.h"
#include "HTParse.h"
#include "tcp.h"
#include "HTTCP.h"
#ifndef DECNET
#include "HTFTP.h"
#endif
#include "HTAnchor.h"
#include "HTAccess.h"

#ifdef unix			/* if this is to compile on a UNIX machine */
#include "HText.h"		/* For directory object building */
#endif

PRIVATE char *HTMountRoot = "/Net/";		/* Where to find mounts */
#ifdef vms
PRIVATE char *HTCacheRoot = "/WWW$SCRATCH/";   /* Where to cache things */
#else
PRIVATE char *HTCacheRoot = "/tmp/W3_Cache_";   /* Where to cache things */
#endif

/* PRIVATE char *HTSaveRoot  = "$(HOME)/WWW/";*/    /* Where to save things */

#ifdef vms
/*	Convert unix-style name into VMS name
**	-------------------------------------
**
** Bug:	Returns pointer to static -- non-reentrant
*/
PRIVATE char * vms_name(CONST char * nn, CONST char * fn)
{

/*	We try converting the filename into Files-11 syntax. That is, we assume
**	first that the file is, like us, on a VMS node. We try remote
**	(or local) DECnet access. Files-11, VMS, VAX and DECnet
**	are trademarks of Digital Equipment Corporation. 
**	The node is assumed to be local if the hostname WITHOUT DOMAIN
**	matches the local one. @@@
*/
    static char vmsname[INFINITY];	/* returned */
    char * filename = (char*)malloc(strlen(fn)+1);
    char * nodename = (char*)malloc(strlen(nn)+2+1);	/* Copies to hack */
    char *second;		/* 2nd slash */
    char *last;			/* last slash */
    
    char * hostname = HTHostName();

    if (!filename || !nodename) outofmem(__FILE__, "vms_name");
    strcpy(filename, fn);
    strcpy(nodename, "");	/* On same node? Yes if node names match */
    {
        char *p, *q;
        for (p=hostname, q=nn; *p && *p!='.' && *q && *q!='.'; p++, q++){
	    if (TOUPPER(*p)!=TOUPPER(*q)) {
	        strcpy(nodename, nn);
		q = strchr(nodename, '.');	/* Mismatch */
		if (q) *q=0;			/* Chop domain */
		strcat(nodename, "::");		/* Try decnet anyway */
		break;
	    }
	}
    }

    second = strchr(filename+1, '/');		/* 2nd slash */
    last = strrchr(filename, '/');	/* last slash */
        
    if (!second) {				/* Only one slash */
	sprintf(vmsname, "%s%s", nodename, filename + 1);
    } else if(second==last) {		/* Exactly two slashes */
	*second = 0;		/* Split filename from disk */
	sprintf(vmsname, "%s%s:%s", nodename, filename+1, second+1);
	*second = '/';	/* restore */
    } else { 				/* More than two slashes */
	char * p;
	*second = 0;		/* Split disk from directories */
	*last = 0;		/* Split dir from filename */
	sprintf(vmsname, "%s%s:[%s]%s",
		nodename, filename+1, second+1, last+1);
	*second = *last = '/';	/* restore filename */
	for (p=strchr(vmsname, '['); *p!=']'; p++)
	    if (*p=='/') *p='.';	/* Convert dir sep.  to dots */
    }
    free(nodename);
    free(filename);
    return vmsname;
}


#endif /* vms */

/*	Make the cache file name for a W3 document
**	------------------------------------------
**	Make up a suitable name for saving the node in
**
**	E.g.	/tmp/WWW_Cache_news/1234@cernvax.cern.ch
**		/tmp/WWW_Cache_http/crnvmc/FIND/xx.xxx.xx
**
** On exit,
**	returns	a malloc'ed string which must be freed by the caller.
*/
PUBLIC char * HTCacheFileName ARGS1(CONST char *,name)
{
    char * access = HTParse(name, "", PARSE_ACCESS);
    char * host = HTParse(name, "", PARSE_HOST);
    char * path = HTParse(name, "", PARSE_PATH+PARSE_PUNCTUATION);
    
    char * result;
    result = (char *)malloc(
	    strlen(HTCacheRoot)+strlen(access)
	    +strlen(host)+strlen(path)+6+1);
    if (result == NULL) outofmem(__FILE__, "HTCacheFileName");
    sprintf(result, "%s/WWW/%s/%s%s", HTCacheRoot, access, host, path);
    free(path);
    free(access);
    free(host);
    return result;
}

/*	Open a file for write, creating the path
**	----------------------------------------
*/
#ifdef NOT_IMPLEMENTED
PRIVATE int HTCreatePath ARGS1(CONST char *,path)
{
    return -1;
}
#endif

/*	Convert filenames between local and WWW formats
**	-----------------------------------------------
**	Make up a suitable name for saving the node in
**
**	E.g.	$(HOME)/WWW/news/1234@cernvax.cern.ch
**		$(HOME)/WWW/http/crnvmc/FIND/xx.xxx.xx
**
** On exit,
**	returns	a malloc'ed string which must be freed by the caller.
*/
PUBLIC char * HTLocalName ARGS1(CONST char *,name)
{
    char * access = HTParse(name, "", PARSE_ACCESS);
    char * host = HTParse(name, "", PARSE_HOST);
    char * path = HTParse(name, "", PARSE_PATH+PARSE_PUNCTUATION);
    
    if (0==strcmp(access, "file")) {
        free(access);	
        if (!host)
          {
	    if (TRACE) fprintf(stderr, "Node `%s' means path `%s'\n", name, path);
	    return(path);
          } else            
	if ((0==strcmp(host, HTHostName())) || (0==strcmp(host, "localhost")) || 
            !*host) {
	    free(host);
	    if (TRACE) fprintf(stderr, "Node `%s' means path `%s'\n", name, path);
	    return(path);
	} else {
	    char * result = (char *)malloc(
	    			strlen("/Net/")+strlen(host)+strlen(path)+1);
              if (result == NULL) outofmem(__FILE__, "HTLocalName");
	    sprintf(result, "%s%s%s", "/Net/", host, path);
	    free(host);
	    free(path);
	    if (TRACE) fprintf(stderr, "Node `%s' means file `%s'\n", name, result);
	    return result;
	}
    } else 
      {  /* other access */
	char * result;
        CONST char * home =  (CONST char*)getenv("HOME");
	if (!home) home = "/tmp"; 
        if (!host) host = strdup ("mangled_url");
        if (!path) path = strdup ("/mangled_url");
	result = 
          (char *)malloc
            (strlen(home)+strlen(access)+strlen(host)+strlen(path)+6+1);
        if (result == NULL) 
          outofmem(__FILE__, "HTLocalName");
	sprintf(result, "%s/WWW/%s/%s%s", home, access, host, path);
	free(path);
	free(access);
	free(host);
	return result;
    }
}


/*	Make a WWW name from a full local path name
**
** Bugs:
**	At present, only the names of two network root nodes are hand-coded
**	in and valid for the NeXT only. This should be configurable in
**	the general case.
*/

PUBLIC char * WWW_nameOfFile ARGS1 (CONST char *,name)
{
    char * result;
#ifdef NeXT
    if (0==strncmp("/private/Net/", name, 13)) {
	result = (char *)malloc(7+strlen(name+13)+1);
	if (result == NULL) outofmem(__FILE__, "WWW_nameOfFile");
	sprintf(result, "file://%s", name+13);
    } else
#endif
    if (0==strncmp(HTMountRoot, name, 5)) {
	result = (char *)malloc(7+strlen(name+5)+1);
	if (result == NULL) outofmem(__FILE__, "WWW_nameOfFile");
	sprintf(result, "file://%s", name+5);
    } else {
        result = (char *)malloc(7+strlen(HTHostName())+strlen(name)+1);
	if (result == NULL) outofmem(__FILE__, "WWW_nameOfFile");
	sprintf(result, "file://%s%s", HTHostName(), name);
    }
    if (TRACE) fprintf(stderr, "File `%s'\n\tmeans node `%s'\n", name, result);
    return result;
}


/*	Determine file format from file name
**	------------------------------------
**
** Note: This function is also in the server file HTRetrieve.c
** If it gets complicated, it should be shared.
**
*/

PRIVATE int mystrcmp ARGS2(char *,s1, char *, s2)
{
  return strncmp (s1, s2, strlen (s1));
}

PUBLIC HTFormat HTFileFormat ARGS3 (CONST char *,filename, int, def,
                                    int *,compressed)
{
    CONST char *extension;
    char *fname_only;
    int fmt;
    int fmt2 = WWW_INVALID;
    /* Mosaic solution -- this is in the widget. */
#ifdef _NO_PROTO
    extern int caseless_equal ();
#else
    extern int caseless_equal (CONST char *, CONST char *);
#endif
    /* This is 1 if we should transfer files as binary and 
       save them to a local file;
       0 if we're operating normally and expect to interpret
       them as the value of parameter 'def' (which is generally
       either WWW_HTML or WWW_PLAINTEXT). */
    extern int binary_transfer;

    for (extension=filename+strlen(filename);
	(extension>filename) &&
		(*extension != '.') &&
		(*extension!='/');
	extension--) /* search */ ;

    /* Strip out mongo WAIS BS at end of extensions. */
    if (strlen (extension) > 8)
      {
        char *a;
        a = strdup (extension);
        if (!strcmp (a + strlen (a) - 7, ";7=%00;"))
          {
            *(a + strlen (a) - 7) = '\0';
          }
        extension = a;
      }

#if 0
    /* Unfortunately, we can figure out the filename here,
       but by the time we get to HTParseFormat we have no way
       of knowing where it is or what to do with it.
       This architecture needs to be rethought. */
    for (fname_only=filename+strlen(filename);
         (fname_only>filename) &&
         (*fname_only!='/');
         fname_only--) /* search */ ;
    if (fname_only && *fname_only == '/')
      fname_only++;
#endif

    /* At this point, fname_only may be a valid filename,
       it may be '\0', or it may be NULL. */

    /* binary_transfer now FORCES use of fmt WWW_UNKNOWN;
       no ifs, ands, or buts. */
    if (binary_transfer)
      fmt = WWW_UNKNOWN;
    else if (*extension == '.') {
      if (caseless_equal(".html",  extension))
        fmt = WWW_HTML;
      else if (caseless_equal(".txt",   extension) ||
               caseless_equal(".text",  extension) ||
               caseless_equal(".tex",   extension))
        fmt = WWW_PLAINTEXT;
      else if (caseless_equal(".ps",    extension))
        fmt =  WWW_POSTSCRIPT;
      else if (caseless_equal(".gif",   extension))
        fmt = WWW_GIF;
      else if (caseless_equal(".hdf",   extension))
        fmt = WWW_HDF;
      else if (caseless_equal(".jpeg",  extension) ||
               caseless_equal(".jpg",   extension))
        fmt = WWW_JPEG;
      else if (caseless_equal(".au",    extension) ||
               caseless_equal(".snd",   extension))
        fmt = WWW_AUDIO;
      else if (caseless_equal(".aiff",  extension) ||
               caseless_equal(".aifc",  extension))
        fmt = WWW_AIFF;
      else if (caseless_equal(".dvi",   extension))
        fmt = WWW_DVI;
      else if (caseless_equal(".tiff",  extension))
        fmt = WWW_TIFF;
      else if (caseless_equal(".tif",   extension))
        fmt = WWW_TIFF;
      else if (caseless_equal(".mpeg",  extension) ||
               caseless_equal(".mpg",   extension))
        fmt = WWW_MPEG;
      else if (caseless_equal(".mime",  extension))
        fmt = WWW_MIME;
      else if (caseless_equal(".xwd",   extension))
        fmt = WWW_XWD;
      else if (caseless_equal(".movie", extension))
        fmt = WWW_SGIMOVIE;
      else if (caseless_equal(".evlm",  extension))
        fmt = WWW_EVLMOVIE;
      else if (caseless_equal(".rgb",   extension))
        fmt = WWW_RGB;
      else if (caseless_equal(".rtf",   extension))
        fmt = WWW_RICHTEXT;
      else if (caseless_equal(".tar",   extension) ||
               caseless_equal(".hqx",   extension) ||
               caseless_equal(".uu",    extension) ||
               caseless_equal(".saveme",extension) ||
               caseless_equal(".dump",  extension))
        fmt = WWW_TAR;
      else if (caseless_equal(".cave",  extension))
        fmt = WWW_CAVE;
      else if (0==strcmp (".Z",     extension))
        fmt = WWW_COMPRESSED;
      else if (0==strcmp (".z",     extension) ||
               0==strcmp (".gz",    extension))
        fmt = WWW_GZIPPED;
      /* Unrecognised, try the default */
      /* Unless we're in binary transfer mode, in which case
         use WWW_UNKNOWN to force dump to local file. */
      else if (binary_transfer)
        fmt = WWW_UNKNOWN;
      else
        fmt = def;
    } else {
      fmt = (binary_transfer ? WWW_UNKNOWN : def);	
    } 
    
    if (fmt == WWW_COMPRESSED || fmt == WWW_GZIPPED)
      {
        int cmp;
        char *newf = NULL;
        StrAllocCopy (newf, filename);
        newf[strlen (newf) - 2] = '\0';
        
        fmt2 = HTFileFormat (newf, def, &cmp);
        /* If cmp is 1 here, something very weird is going on. */

        free (newf);
        
        /* Yes it's compressed; set *compressed to the compression format
           and return the real format. */
        *compressed = fmt;
        return fmt2;
      }
    else
      {
        /* No it's not compressed; return the real format. */
        *compressed = 0;
        return fmt;
      }
}


/*	Determine write access to a file
//	--------------------------------
//
// On exit,
//	return value	YES if file can be accessed and can be written to.
//
// Bugs:
//	1.	No code for non-unix systems.
//	2.	Isn't there a quicker way?
*/

#ifdef vms
#define NO_GROUPS
#endif
#ifdef NO_UNIX_IO
#define NO_GROUPS
#endif
#ifdef PCNFS
#define NO_GROUPS
#endif

#ifndef NGROUPS
#ifdef NGROUPS_MAX
#define NGROUPS NGROUPS_MAX
#endif
#endif

PUBLIC BOOL HTEditable ARGS1 (CONST char *,filename)
{
#ifdef NO_GROUPS
    return NO;		/* Safe answer till we find the correct algorithm */
#else
    int 	groups[NGROUPS];	
    uid_t	myUid;
    int		ngroups;			/* The number of groups  */
    struct stat	fileStatus;
    int		i;
        
    if (stat(filename, &fileStatus))		/* Get details of filename */
    	return NO;				/* Can't even access file! */

    ngroups = getgroups(NGROUPS, groups);	/* Groups to which I belong  */
    myUid = geteuid();				/* Get my user identifier */

    if (TRACE) {
        int i;
	printf("File mode is 0%o, uid=%d, gid=%d. My uid=%d, %d groups (",
    	    fileStatus.st_mode, fileStatus.st_uid, fileStatus.st_gid,
	    myUid, ngroups);
	for (i=0; i<ngroups; i++) printf(" %d", groups[i]);
	printf(")\n");
    }
    
    if (fileStatus.st_mode & 0002)		/* I can write anyway? */
    	return YES;
	
    if ((fileStatus.st_mode & 0200)		/* I can write my own file? */
     && (fileStatus.st_uid == myUid))
    	return YES;

    if (fileStatus.st_mode & 0020)		/* Group I am in can write? */
    {
   	for (i=0; i<ngroups; i++) {
            if (groups[i] == fileStatus.st_gid)
	        return YES;
	}
    }
    if (TRACE) fprintf(stderr, "\tFile is not editable.\n");
    return NO;					/* If no excuse, can't do */
#endif
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
int HTOpenFile
ARGS4
(
 CONST char *,addr,
 HTFormat *,pFormat,
 HTParentAnchor *,anchor,
 int *,compressed
)
{
    char * filename;
    int fd = -1;		/* Unix file descriptor number = INVALID */
    char * nodename = 0;
    char * newname=0;	/* Simplified name of file */

/*	Reduce the filename to a basic form (hopefully unique!)
*/
    StrAllocCopy(newname, addr);
    filename=HTParse(newname, "", PARSE_PATH|PARSE_PUNCTUATION);
    nodename=HTParse(newname, "", PARSE_HOST);
    free(newname);

    *pFormat = HTFileFormat(filename, WWW_PLAINTEXT, compressed);

    
#ifdef vms
/* Assume that the file is in Unix-style syntax if it contains a '/'
   after the leading one @@ */
    {
	char * vmsname = strchr(filename + 1, '/') ?
	  vms_name(nodename, filename) : filename + 1;
	fd = open(vmsname, O_RDONLY, 0);
	
/*	If the file wasn't VMS syntax, then perhaps it is ultrix
*/
	if (fd<0) {
	    char ultrixname[INFINITY];
	    if (TRACE) fprintf(stderr, "HTFile: Can't open as %s\n", vmsname);
	    sprintf(ultrixname, "%s::\"%s\"", nodename, filename);
	    fd = open(ultrixname, O_RDONLY, 0);
	    if (fd<0) {
		if (TRACE) fprintf(stderr, 
				   "HTFile: Can't open as %s\n", ultrixname);
	    }
	}
    }
#else

/*	For unix, we try to translate the name into the name of a transparently
**	mounted file.
**
**	Not allowed in secure (HTClienntHost) situations TBL 921019
*/
#ifndef NO_UNIX_IO
    if (!HTClientHost) {		/* try local unix file system */
	char * localname = HTLocalName(addr);
	
	
#ifdef GOT_READ_DIR
/*
**	Check to see if the 'localname' is in fact a directory.  If it is
**	create a new hypertext object containing a list of files and 
**	subdirectories contained in the directory.  All of these are links
**      to the directories or files listed.
**      NB This assumes the existance of a type 'struct direct', which will
**      hold the directory entry, and a type 'DIR' which is used to point to
**      the current directory being read.
*/
	
	struct stat dir_info;
	
	if (stat(localname,&dir_info) == -1) {     /* get file information */
	                               /* if can't read file information */
	    if (TRACE) fprintf(stderr, "HTFile: can't stat %s\n", localname);

	}  else {		/* Stat was OK */
		

	    if (((dir_info.st_mode) & S_IFMT) == S_IFDIR) {
		/* if localname is a directory */	
		HTChildAnchor * child;
		HText * HT;
		extern HTStyleSheet * styleSheet;
		static HTStyle * DirectoryStyle = 0;
		static HTStyle * H1Style = 0;
		DIR *dp;
		struct direct * dirbuf;
		    
		char * tmpfilename = NULL;
		char * shortfilename = NULL;
		struct stat *file_info =
		  (struct stat *) malloc(sizeof(struct stat));
		if (!file_info) outofmem(__FILE__, "HTOpenFile");
		
		if (TRACE)
		    fprintf(stderr,"%s is a directory\n",localname);
#if 0			
		if (!H1Style)
		    H1Style = HTStyleNamed(styleSheet, "Heading1");
		
		if (!DirectoryStyle)
		    DirectoryStyle = HTStyleNamed(styleSheet, "Dir");
#endif		
		HTAnchor_setTitle(anchor, filename);
		    
		HT = HText_new(anchor);
		HText_beginAppend(HT);
		    
		/* HText_setStyle(HT,H1Style); */
                HText_appendText (HT, "<H1>Directory ");
		shortfilename=strrchr(localname,'/');
		/* put the last part of the path in shortfilename */
		if (shortfilename)
		{
			shortfilename++;          /* get rid of leading '/' */
			if (*shortfilename=='\0')
			    shortfilename--;
		}
		else
		{
			char *tptr;

			tptr = (char *)malloc(strlen(localname) + 3);
			strcpy(tptr, "./");
			strcat(tptr, localname);
			localname = tptr;
			shortfilename=strrchr(localname,'/');
			shortfilename++;          /* get rid of leading '/' */
			if (*shortfilename=='\0')
			    shortfilename--;
		}

                HText_appendText (HT, filename);
		/* HText_appendText(HT,shortfilename); */
		/* HText_setStyle(HT,DirectoryStyle); */
                HText_appendText (HT, "</H1>\n<UL>\n");
		
		dp = opendir(localname);
		if (dp) {                   
		   /* if the directory file is readable */
		    while (dirbuf = readdir(dp)) {
				/* while there are directory entries to be read */
#ifdef USE_FILENO
			if (dirbuf->d_fileno == 0)
#else
			if (dirbuf->d_ino == 0)
#endif
					/* if the entry is not being used, skip it */
			{
			    continue;
			}
			
			if (!strcmp(dirbuf->d_name,"."))
			{
			    continue;      /* skip the entry for this directory */
			}
			    
			if (strcmp(dirbuf->d_name,".."))
				    /* if the current entry is parent directory */
			{
			    if ((*(dirbuf->d_name)=='.') ||
				(*(dirbuf->d_name)==','))
			    {
				continue;    /* skip those files whose name begins
						with '.' or ',' */
			    }
			}
    
			StrAllocCopy(tmpfilename,localname);
			if (strcmp(localname,"/")) 
					    /* if filename is not root directory */
			{
			    StrAllocCat(tmpfilename,"/"); 
			}
			else
			{
			    if (!strcmp(dirbuf->d_name,".."))
			    {
				continue;
			    }
			}
		/* if root directory and current entry is parent
					directory, skip the current entry */
			
			StrAllocCat(tmpfilename,dirbuf->d_name);
			/* append the current entry's filename to the path */
			HTSimplify(tmpfilename);
		
#if 0
			child = HTAnchor_findChildAndLink(
				anchor, 0, tmpfilename, 0);
			HText_beginAnchor(HT, child); 
#endif
                        HText_appendText (HT, "<LI> <A HREF=\"");
                        HText_appendText (HT, tmpfilename);
                        HText_appendText (HT, "\">");
			stat(tmpfilename,file_info);
			
			if (strcmp(dirbuf->d_name,"..")) {
/* if the current entry is not the parent directory then use the file name */
			    HText_appendText(HT,dirbuf->d_name);
			    if (((file_info->st_mode) & S_IFMT) == S_IFDIR) 
				HText_appendCharacter(HT, '/'); 
			}		        
			else {
			/* use name of parent directory */
			    char * endbit = strrchr(tmpfilename, '/');
			    HText_appendText(HT,"Up to ");
			    HText_appendText(HT, endbit?endbit+1:tmpfilename);
			}	
			/* HText_endAnchor(HT); */
                        HText_appendText (HT, "</A>\n");
			    
			/* HText_appendCharacter(HT, '\t'); */
		    } /* end while directory entries left to read */
		    
		    closedir(dp);
		    free(tmpfilename);
		    
		}  else {   /* Directory is not readable */
		    if (TRACE)
			fprintf(stderr,"HTFile.c: directory %s unreadable\n",
				localname);
		    HText_appendText(HT,
			   "Sorry, can't read the contents of this directory - probably read protected\n");
			    
		} /* end if directory not readable */	 

		HText_appendText (HT, "</UL>\n");
		HText_endAppend(HT);
		    
		free(file_info);
		free(filename);  
		free(localname);
		return HT_LOADED;	/* fd not valid, but document loaded anyway */
		
	    } /* end if localname is directory */
	
	} /* end if file stat worked */
	
/* End of directory reading section
*/
#endif

	fd = open(localname, O_RDONLY, 0);
	if(TRACE) printf ("HTAccess: Opening `%s' gives %d\n",
			    localname, fd);
	free(localname);
    }  /* local unix file system */
#endif
#endif

#ifndef DECNET
/*	Now, as transparently mounted access has failed, we try FTP.
*/
    if (fd<0 && nodename)
	if (strcmp(nodename, HTHostName())!=0) {
	    free(filename);
	    return HTFTP_open_file_read(addr, anchor);
    }
#endif

/*	All attempts have failed if fd<0.
*/
    if (fd<0) if (TRACE)
    	printf("Can't open `%s', errno=%d\n", filename, errno);
    free(filename);

    /* BEATS ME WHY THE HELL THIS SHOULD EVER HAPPEN,
       BUT IT DOES!!! */
    if (fd == 0)
      fd = HT_LOADED;
    return fd;

}
