/*		Parse HyperText Document Address		HTParse.c
**		================================
*/

#include "HTUtils.h"
#include "HTParse.h"
#include "tcp.h"

struct struct_parts {
	char * access;
	char * host;
	char * absolute;
	char * relative;
/*	char * search;		no - treated as part of path */
	char * anchor;
};


/*	Strip white space off a string
**	------------------------------
**
** On exit,
**	Return value points to first non-white character, or to 0 if none.
**	All trailing white space is OVERWRITTEN with zero.
*/

#ifdef __STDC__
char * HTStrip(char * s)
#else
char * HTStrip(s)
	char *s;
#endif
{
#define SPACE(c) ((c==' ')||(c=='\t')||(c=='\n')) 
    char * p=s;
    for(p=s;*p;p++);		        /* Find end of string */
    for(p--;p>=s;p--) {
    	if(SPACE(*p)) *p=0;	/* Zap trailing blanks */
	else break;
    }
    while(SPACE(*s))s++;	/* Strip leading blanks */
    return s;
}


/*	Scan a filename for its consituents
**	-----------------------------------
**
** On entry,
**	name	points to a document name which may be incomplete.
** On exit,
**      absolute or relative may be nonzero (but not both).
**	host, anchor and access may be nonzero if they were specified.
**	Any which are nonzero point to zero terminated strings.
*/
#ifdef __STDC__
PRIVATE void scan(char * name, struct struct_parts *parts)
#else
PRIVATE void scan(name, parts)
    char * name;
    struct struct_parts *parts;
#endif
{
    char * after_access;
    char * p;
    int length;
    
    if (name && *name)
      length = strlen(name);
    else
      length = 0;
    
    parts->access = 0;
    parts->host = 0;
    parts->absolute = 0;
    parts->relative = 0;
    parts->anchor = 0;

    /* AAAAAAAAAAAAAAAAARGH! */
    if (!length)
      return;
    
    after_access = name;
    for(p=name; *p; p++) {
	if (*p==':') {
		*p = 0;
		parts->access = name;	/* Access name has been specified */
		after_access = p+1;
	}
	if (*p=='/') break;
	if (*p=='#') break;
    }
    
    for(p=name+length-1; p>=name; p--) {
	if (*p =='#') {
	    parts->anchor=p+1;
	    *p=0;				/* terminate the rest */
	}
    }
    p = after_access;
    if (*p=='/'){
	if (p[1]=='/') {
	    parts->host = p+2;		/* host has been specified 	*/
	    *p=0;			/* Terminate access 		*/
	    p=strchr(parts->host,'/');	/* look for end of host name if any */
	    if(p) {
	        *p=0;			/* Terminate host */
	        parts->absolute = p+1;		/* Root has been found */
	    }
	} else {
	    parts->absolute = p+1;		/* Root found but no host */
	}	    
    } else {
        parts->relative = (*after_access) ? after_access : 0;	/* zero for "" */
    }

    /* Access specified but no host: the anchor was not really one
       e.g. news:j462#36487@foo.bar -- JFG 10/7/92, from bug report */
    if (parts->access && ! parts->host && parts->anchor) {
      *(parts->anchor - 1) = '#';  /* Restore the '#' in the address */
      parts->anchor = 0;
    }

#ifdef NOT_DEFINED	/* search is just treated as part of path */
    {
        char *p = relative ? relative : absolute;
	if (p) {
	    char * q = strchr(p, '?');	/* Any search string? */
	    if (q) {
	    	*q = 0;			/* If so, chop that off. */
		parts->search = q+1;
	    }
	}
    }
#endif
} /*scan */    


/*	Parse a Name relative to another name
**	-------------------------------------
**
**	This returns those parts of a name which are given (and requested)
**	substituting bits from the related name where necessary.
**
** On entry,
**	aName		A filename given
**      relatedName     A name relative to which aName is to be parsed
**      wanted          A mask for the bits which are wanted.
**
** On exit,
**	returns		A pointer to a malloc'd string which MUST BE FREED
*/
#ifdef __STDC__
char * HTParse(const char * aName, const char * relatedName, int wanted)
#else
char * HTParse(aName, relatedName, wanted)
    char * aName;
    char * relatedName;
    int wanted;
#endif

{
    char * result = 0;
    char * return_value = 0;
    int len;
    char * name = 0;
    char * rel = 0;
    char * p;
    struct struct_parts given, related;
    
    /* Make working copies of input strings to cut up:
    */
    len = strlen(aName)+strlen(relatedName)+10;
    result=(char *)malloc(len);		/* Lots of space: more than enough */
    if (result == NULL) outofmem(__FILE__, "HTParse");
    
    StrAllocCopy(name, aName);
    StrAllocCopy(rel, relatedName);
    
    scan(name, &given);
    scan(rel,  &related); 
    result[0]=0;		/* Clear string  */
    if (wanted & PARSE_ACCESS)
        if (given.access|| related.access) {
	    strcat(result, given.access ? given.access : related.access);
	    if(wanted & PARSE_PUNCTUATION) strcat(result, ":");
	}
	
    if (given.access && related.access)	/* If different, inherit nothing. */
        if (strcmp(given.access, related.access)!=0) {
	    related.host=0;
	    related.absolute=0;
	    related.relative=0;
	    related.anchor=0;
	}
	
    if (wanted & PARSE_HOST)
        if(given.host || related.host) {
	    if(wanted & PARSE_PUNCTUATION) strcat(result, "//");
	    strcat(result, given.host ? given.host : related.host);
	}
	
    if (given.host && related.host)  /* If different hosts, inherit no path. */
        if (strcmp(given.host, related.host)!=0) {
	    related.absolute=0;
	    related.relative=0;
	    related.anchor=0;
	}
	
    if (wanted & PARSE_PATH) {
        if(given.absolute) {				/* All is given */
	    if(wanted & PARSE_PUNCTUATION) strcat(result, "/");
	    strcat(result, given.absolute);
	} else if(related.absolute) {	/* Adopt path not name */
	    strcat(result, "/");
	    strcat(result, related.absolute);
	    if (given.relative) {
		p = strchr(result, '?');	/* Search part? */
		if (!p) p=result+strlen(result)-1;
		for (; *p!='/'; p--);	/* last / */
		p[1]=0;					/* Remove filename */
		strcat(result, given.relative);		/* Add given one */
		HTSimplify (result);
	    }
	} else if(given.relative) {
	    strcat(result, given.relative);		/* what we've got */
	} else if(related.relative) {
	    strcat(result, related.relative);
	} else {  /* No inheritance */
	    strcat(result, "/");
	}
    }
		
    if (wanted & PARSE_ANCHOR)
        if(given.anchor || related.anchor) {
	    if(wanted & PARSE_PUNCTUATION) strcat(result, "#");
	    strcat(result, given.anchor ? given.anchor : related.anchor);
	}
    if (rel && *rel)
      free(rel);
    if (name && *name)
      free(name);
    
    StrAllocCopy(return_value, result);
    if (result && *result)
      free(result);
    return return_value;		/* exactly the right length */
}

/*	        Simplify a filename
//		-------------------
//
// A unix-style file is allowed to contain the seqeunce xxx/../ which may be
// replaced by "" , and the seqeunce "/./" which may be replaced by "/".
// Simplification helps us recognize duplicate filenames.
//
//	Thus, 	/etc/junk/../fred 	becomes	/etc/fred
//		/etc/junk/./fred	becomes	/etc/junk/fred
*/
#ifdef __STDC__
void HTSimplify(char * filename)
#else
void HTSimplify(filename)
    char * filename;
#endif

{
    char * p;
    char * q;
    for(p=filename+2; *p; p++) {
        if (*p=='/') {
	    if ((p[1]=='.') && (p[2]=='.') && (p[3]=='/' || !p[3] )) {
		for (q=p-1; (q>filename) && (*q!='/'); q--); /* prev slash */
		if (*q=='/') {
	            strcpy(q, p+3);	/* Remove  /xxx/..	*/
		    if (!*filename) strcpy(filename, "/");
		    p = q-1;		/* Start again with prev slash 	*/
		} else {			/*   xxx/..	error?	*/
		    strcpy(filename, p[3] ? p+4 : p+3); /* rm  xxx/../	*/
		    p = filename;		/* Start again */
		}
	    } else if ((p[1]=='.') && (p[2]=='/' || !p[2])) {
	        strcpy(p, p+2);			/* Remove a slash and a dot */
	    }
	}
    }
}


/*		Make Relative Name
**		------------------
**
** This function creates and returns a string which gives an expression of
** one address as related to another. Where there is no relation, an absolute
** address is retured.
**
**  On entry,
**	Both names must be absolute, fully qualified names of nodes
**	(no anchor bits)
**
**  On exit,
**	The return result points to a newly allocated name which, if
**	parsed by HTParse relative to relatedName, will yield aName.
**	The caller is responsible for freeing the resulting name later.
**
*/
#ifdef __STDC__
char * HTRelative(const char * aName, const char *relatedName)
#else
char * HTRelative(aName, relatedName)
   char * aName;
   char * relatedName;
#endif
{
    char * result = 0;
    CONST char *p = aName;
    CONST char *q = relatedName;
    CONST char * after_access = 0;
    CONST char * path = 0;
    CONST char * last_slash = 0;
    int slashes = 0;
    
    for(;*p; p++, q++) {	/* Find extent of match */
    	if (*p!=*q) break;
	if (*p==':') after_access = p+1;
	if (*p=='/') {
	    last_slash = p;
	    slashes++;
	    if (slashes==3) path=p;
	}
    }
    
    /* q, p point to the first non-matching character or zero */
    
    if (!after_access) {			/* Different access */
        StrAllocCopy(result, aName);
    } else if (slashes<3){			/* Different nodes */
    	StrAllocCopy(result, after_access);
    } else if (slashes==3){			/* Same node, different path */
        StrAllocCopy(result, path);
    } else {					/* Some path in common */
        int levels= 0;
        for(; *q && (*q!='#'); q++)  if (*q=='/') levels++;
	result = (char *)malloc(3*levels + strlen(last_slash) + 1);
      if (result == NULL) outofmem(__FILE__, "HTRelative");
	result[0]=0;
	for(;levels; levels--)strcat(result, "../");
	strcat(result, last_slash+1);
    }
    if (TRACE) fprintf(stderr, "HT: `%s' expressed relative to\n    `%s' is\n   `%s'.",
    		aName, relatedName, result);
    return result;
}
