#include	<stdio.h>
#include	<string.h>
#include	<sys/types.h>
#include	<dtm.h>

#include	"dmf.h"

#ifdef NeXT
#include <sys/dirent.h>
#include <sys/dir.h>
#else
#include <dirent.h>
#endif

static char	*buf = NULL;


/*
** DMFsendFile - utility routine used to transfer a file verbatim.
**
** Args
**	port	- DTM output port used to write request
**	path	- path of file, maybe NULL
**	filename	- filename, maybe NULL or contain some of the path
**
** NOTE: the filename is sent in the header.  DMFrecvFile will store
**	the file in filename which may contain non-existant directories.
**	These will be created as needed.
*/
int DMFsendFile(port, path, filename, fileformat)
	int		port;
	char	*path, *filename;
	int		fileformat;
{
	int		len;
	char	header[DTM_MAX_HEADER];
	char	fullname[256];
	FILE	*fptr;


	/* allocate a buffer if necessary */
	if (buf == NULL)
		if ((buf = (char *)malloc(DMF_BUFSIZE)) == NULL)
			return DMFERROR;

	/* concatentate filename and path */
	if (path != NULL)  {
		strcpy(fullname, path);
		if (filename != NULL)
			strcat(fullname, filename);
	}

	/* null path, fullname is simply filename */
	else if (filename != NULL)  {
		strcpy(fullname, filename);
	}

	/* null path and filename, error */
	else
		return DMFERROR;

	/* open the file and begin transfer */
	if ((fptr = fopen(fullname, "r")) == NULL)
		return DMFERROR;

	dtm_set_class(header, FILEclass);
	dtm_set_char(header, "NAME", filename);
	dtm_set_int(header, "FORMAT", fileformat);

	if (DTMbeginWrite(port, header, DTMHL(header)) == DTMERROR)
		return DMFERROR;

	while((len = fread(buf, sizeof (char), DMF_BUFSIZE, fptr)) > 0)
		DTMwriteDataset(port, buf, len, DTM_CHAR);

	DTMendWrite(port);
	close(fptr);
}


/*
** create_dirs - utility routine which ensures all the directories
**	in a path exist.  If a non-existant directory is encountered, it
**	is created.
*/
static int create_dirs(path)
	char	*path;
{
	int		len;
	char	*lpath, *cptr;
	DIR 	*dptr;


	len = strlen(path);
	if ((lpath = (char *)malloc(len + 1)) == NULL)
		return DMFERROR;
	memset(lpath, '\0', len + 1);

	cptr = (*path == '/') ? path+1 : path;
	while ((cptr = strchr(cptr, '/')) != NULL)  {
		memcpy(lpath, path, cptr - path);
		if ((dptr = opendir(lpath)) == NULL)  {
			if (mkdir(lpath, 0777) == -1)
				return DMFERROR;
		}
		else {
			closedir(dptr);
		}
		cptr += 1;
	}

	return DMFOK;
}


/*
** checkfile - utility routine, take two pathnames and concatenates
**	them together.  Then calls create_dirs to make sure all the
**	subdirectories exist.
*/
static char *checkfile(basepath, filepath, mode)
	char	*basepath, *filepath, *mode;
{
	int		len;
	char	file[1024];


	/* copy basepath to file, append '/' if necessary */
	if (basepath != NULL)  {
		strcpy(file, basepath);
		if ( *(basepath + strlen(basepath) - 1) != '/')
			strcat(file, "/");
	}

	/* if basepath == NULL, start with current directory */
	else  {
		strcpy(file, "./");
	}

	/* append filepath to basepath, remove beginning '/' if necessary */
	/* if filepath is NULL assume the file came in on the basepath */
	if (filepath != NULL)  {
		if (*filepath == '/')
			filepath += 1;
		strcat(file, filepath);
	}

	/* make sure all the directories exist in the path */
	if (create_dirs(file) == DMFERROR)  {
		return NULL;
	}

	/* finally ready -- open the file */
	return strdup(file);
}


/*
** DMFrecvFile - handle the receiving and writing, uncompressing, and
**	untarring of the files.
**
** Args
**	port	- dtm port
**	header	- the message header
**	basepath - base path for saving file
**	filepath - filename to where the file was stored
*/
int DMFrecvFile(port, header, basepath, filepath)
	int		port;
	char	*header, *basepath, **filepath;
{
	int		len, format;
	char	name[256], *cptr, *filename;
	FILE	*outfile;


	/* allocate a buffer if necessary */
	if (buf == NULL)
		if ((buf = (char *)malloc(DMF_BUFSIZE)) == NULL)
			return DMFERROR;

	/* get the filename from header */
	dtm_get_char(header, "NAME", name, sizeof name);
	dtm_get_int(header, "format", &format);

	/* open the filename as <basepath>/<name> */
	if ((filename = checkfile(basepath, name)) == NULL)
		return DMFERROR;
	
	if ((outfile = fopen(filename, "w")) == NULL)
		return DMFERROR;

	while ((len = DTMreadDataset(port, buf, DMF_BUFSIZE, DTM_CHAR)) > 0)
		fwrite(buf, sizeof (char), len, outfile);

	fclose(outfile);
	free(filename);

	return DMFOK;
}


/*
** DMFrecvFileData - receives the file data into a buffer.  It
**	returns the filename, format, data and length.
**
** Args
**	port	- dtmport
**	header	- message header
**	fname	- return value, filename (possibly with subdirectories)
**	format	- return value, format of file, tarred, compressed, etc
**	data	- return value, location of the data
**	length	- return value, length of the data buffer
*/
int DMFrecvFileData(port, header, fname, format, data, length)
	int		port, *format, *length;
	char	*header, **fname, **data;
{
	int		len, dlen, dsize;
	char	name[256], *cptr;
	char	*dbuf, *dptr;


	/* allocate a buffer if necessary */
	if (buf == NULL)
		if ((buf = (char *)malloc(DMF_BUFSIZE)) == NULL)
			return DMFERROR;

	/* get the file name, strip of original path and append to new path */
	dtm_get_char(header, "NAME", buf, DMF_BUFSIZE);
	*fname = strdup(buf);
	*format = 0;
	dtm_get_int(header, "FORMAT", format);

	/* read file data into buffer, grow buffer as needed */
	dlen = 0;
	dsize = 0;
	dbuf = NULL;
	while ((len = DTMreadDataset(port, buf, DMF_BUFSIZE, DTM_CHAR)) > 0)
	{
		if (dbuf == NULL)
		{
			if ((dbuf = (char *)malloc(DMF_BUFSIZE)) == NULL)
			{
				return DMFERROR;
			}
			dsize = DMF_BUFSIZE;
			bcopy(buf, dbuf, len);
			dlen = len;
		}
		else
		{
			if ((dlen + len) > dsize)
			{
				if ((dptr = (char *)malloc(dsize * 2)) == NULL)
				{
					free(dbuf);
					return DMFERROR;
				}
				dsize *= 2;
				bcopy(dbuf, dptr, dlen);
				free(dbuf);
				dbuf = dptr;
			}

			bcopy(buf, (char *)(dbuf + dlen), len);
			dlen += len;
		}
	}

	*data = dbuf;
	*length = dlen;
	return DMFOK;
}


static int unpackfile(filename, mode)
	char	*filename;
	int		mode;
{
	char	*path, *file, *cptr;
	char	cmd[1024];
	char	current_dir[1024];

	if ((mode & (DMF_FF_TARRED|DMF_FF_COMPRESSED)) == 0)
		return(DMFOK);

	if ((path = strdup(filename)) == NULL)
		return DMFERROR;

	/* if filename contains a path, get the path and chdir to it */
    if ((cptr = strrchr(path, '/')) != NULL)  {
		*cptr = '\0';
		file = cptr + 1;
	}

	/* no path in filename */
	else  {
		file = path;
		path = NULL;
	}

	/* Save the current directory so we can return to it later */
	if (path != NULL) {
		getcwd(current_dir, 1024);
	}

	
	/* fork a child process to uncompress and untar if necessary */
	if (mode & DMF_FF_BACKGROUND)  {

		switch (fork())  {

		  /* error - could not fork */
		  case -1:
			return DMFERROR;

		  /* child process, handle file and exit */
		  case 0:
			if (path != NULL)
				chdir(path);

			if ((mode & DMF_FF_COMPRESSED) && (mode & DMF_FF_TARRED))  {
				sprintf(cmd, "zcat %s | tar -xf -", file);
				system(cmd);
				unlink(file);
			}

			else if (mode & DMF_FF_COMPRESSED)  {
				sprintf(cmd, "uncompress %s", file);
				system(cmd);
			}

			else if (mode & DMF_FF_TARRED)  {
				sprintf(cmd, "tar -xf %s", file);
				system(cmd);
				unlink(file);
			}
			exit(0);

		  /* parent process */
		  /* NOTE: should check for old children */
		  default:
			(path != NULL) ? free(path) : free(file);
			return DMFOK;
		}
	}

	/* call system to uncompress and untar if necessary */
	else  {
		if (path != NULL)
		  chdir(path);
		
		if ((mode & DMF_FF_COMPRESSED) && (mode & DMF_FF_TARRED))  {
			sprintf(cmd, "zcat %s | tar -xf -", file);
			system(cmd);
			unlink(file);
		}
		
		else if (mode & DMF_FF_COMPRESSED)  {
			sprintf(cmd, "uncompress %s", file);
			system(cmd);
		}
		
		else if (mode & DMF_FF_TARRED)  {
			sprintf(cmd, "tar -xf %s", file);
			system(cmd);
			unlink(file);
		}

		(path != NULL) ? free(path) : free(file);
	}

	/* return to the original current directory */
	if (path != NULL)  {
		chdir(current_dir);
	}

	return(DMFOK);
}


/*
** DMFsaveFileData - this function complements the the DMFrecvFileData.
**	It takes the data a received and saves it to a file.  If the mode
**	bits are set, the file can be uncompressed and untarred.
**
** Args
**	basepath - the base of the path, may be NULL
**	filepath - the filepath starting at basepath, may be NULL
**	format	 - control automatic decompress and untarring
**	buffer 	 - buffer to be written to the file
**	length	 - the length of the buffer
*/
DMFsaveFileData(basepath, filepath, format, buffer, length)
	char	*basepath, *filepath, *buffer;
	int		format, length;
{
	char	*filename;
	FILE	*outfile;


	/* get the complete filename */
	if ((filename = checkfile(basepath, filepath)) == NULL)
		return DMFERROR;

	/* open the filename as <basepath>/<filepath> */
	if ((outfile = fopen(filename, "w")) == NULL)
		return DMFERROR;

	/* write the file to disk */
	fwrite(buffer, sizeof (char), length, outfile);
	fclose(outfile);

	unpackfile(filename, format);
}
