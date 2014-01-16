/*		Manage different file formats			HTFormat.c
**		=============================
**
** Bugs:
**	Not reentrant.
**
**	Assumes the incoming stream is ASCII, rather than a local file
**	format, and so ALWAYS converts from ASCII on non-ASCII machines.
**	Therefore, non-ASCII machines can't read local files.
*/

#include "HTUtils.h"
#include "tcp.h"
#include "HTFormat.h"

#include "HTML.h"
#include "HText.h"
#include "HTStyle.h"

#ifdef MOTOROLA
# include <sys/fcntl.h>    /* to get O_RDONLY */
#endif

extern HTStyleSheet * styleSheet;

#ifdef _NO_PROTO
extern void application_user_feedback ();
extern void rename_binary_file ();
#else
extern void application_user_feedback (char *);
extern void rename_binary_file (char *);
#endif

PUBLIC	BOOL HTOutputSource = NO;	/* Flag: shortcut parser to stdout */

#ifdef _NO_PROTO
extern char *mo_tmpnam ();
#else
extern char *mo_tmpnam (void);
#endif

/*	File buffering
**	--------------
**
**	The input file is read using the macro which can read from
**	a socket or a file.
**	The input buffer size, if large will give greater efficiency and
**	release the server faster, and if small will save space on PCs etc.
*/
#define INPUT_BUFFER_SIZE 4096		/* Tradeoff */
PRIVATE char input_buffer[INPUT_BUFFER_SIZE];
PRIVATE char * input_pointer;
PRIVATE char * input_limit;
PRIVATE int input_file_number;


/*	Set up the buffering
**
**	These routines are public because they are in fact needed by
**	many parsers, and on PCs and Macs we should not duplicate
**	the static buffer area.
*/
PUBLIC void HTInitInput ARGS1 (int,file_number)
{
    input_file_number = file_number;
    input_pointer = input_limit = input_buffer;
}

#if 0
PUBLIC char HTGetChararcter NOARGS
{
    char ch;
    do {
	if (input_pointer >= input_limit) {
	    int status = NETREAD(
		    input_file_number, input_buffer, INPUT_BUFFER_SIZE);
	    if (status <= 0) {
		if (status == 0) return (char)EOF;
		if (TRACE) fprintf(stderr,
		    "HTFormat: File read error %d\n", status);
		return (char)EOF; /* -1 is returned by UCX at end of HTTP link */
	    }
	    input_pointer = input_buffer;
	    input_limit = input_buffer + status;
	}
	ch = *input_pointer++;
    } while (ch == (char) 13); /* Ignore ASCII carriage return */
    
    return FROMASCII(ch);
}
#else /* NCSA version */
PUBLIC char HTGetChararcter NOARGS
{
    char ch;
    if (input_pointer >= input_limit) {
      int status = NETREAD(
                           input_file_number, input_buffer, INPUT_BUFFER_SIZE);
      if (status <= 0) {
        if (status == 0) return (char)EOF;
        if (TRACE) fprintf(stderr,
                           "HTFormat: File read error %d\n", status);
        return (char)EOF; /* -1 is returned by UCX at end of HTTP link */
      }
      input_pointer = input_buffer;
      input_limit = input_buffer + status;
    }
    ch = *input_pointer++;
    
    return FROMASCII(ch);
}
#endif

/*	Stream the data to an ouput file as binary
*/
PUBLIC int HTOutputBinary ARGS2( int, 		input,
				  FILE *, 	output)
{
    do {
	    int status = NETREAD(
		    input, input_buffer, INPUT_BUFFER_SIZE);
	    if (status <= 0) {
		if (status == 0) return 0;
		if (TRACE) fprintf(stderr,
		    "HTFormat: File read error %d\n", status);
		return 2;			/* Error */
	    }
	    fwrite(input_buffer, sizeof(char), status, output);
    } while (YES);
}



/* Create a temp file, output the contents of fileno to that
   file, close the file, fork off the viewer, and create the
   override text. */
PRIVATE char *pipe_to_reader ARGS3(
	HTParentAnchor *,anchor,
	int,filenum,
	char *,reader)
{
  HText *text;
  char *fnam, *cmd;
  FILE *fp;

  fnam = mo_tmpnam();

  /* We should be stripping the <PLAINTEXT> bullshit off of
     the remote file right here. */
  fp = fopen (fnam, "w");
  if (!fp)
    goto oops;
  HTOutputBinary (filenum, fp);
  fclose (fp);

  /* If we have a reader thing, fork it off. */
  if (reader && strcmp (reader, "dump"))
    {
      cmd = (char *)malloc ((strlen (reader) + 64 + 2*strlen(fnam)) 
                            * sizeof (char));
      sprintf (cmd, "(%s %s ; /bin/rm -f %s) &", reader, fnam, fnam);
      if ((system (cmd)) != 0)
        {
          char *msg = (char *)malloc (64 + strlen (reader));
          sprintf (msg, "Unable to fork external viewer %s\0", reader);
          application_user_feedback (msg);
          free (msg);
        }
      free (cmd);
    }
  else
    {
      /* Give the user a chance to rename the binary file,
         right now. */
      rename_binary_file (fnam);
    }
  
 oops:
  text = HText_new(anchor);
  HText_beginAppend(text);
  HText_appendText (text, "<mosaic-access-override>\n");
  HText_endAppend(text);
  
  return fnam;
}



/*	Parse a file given format and file number
**	------------
*/
PUBLIC void HTParseFormat ARGS4(
	HTFormat,format,
	HTParentAnchor *,anchor,
	int,file_number,
        int,compressed)
{
  HText * text;
  extern char *gif_reader, *jpeg_reader, *audio_reader, *aiff_reader,
    *postscript_reader, *dvi_reader, *mpeg_reader, *mime_reader,
    *xwd_reader, *sgimovie_reader, *evlmovie_reader, *rgb_reader,
    *tiff_reader, *cave_reader, *hdf_reader;
  extern char *uncompress_program, *gunzip_program;
  int done = 0, plaintext = 0, impossible = 0;
  /* If this is non-zero, we won't do anything special with the document
     except return the bytestream. */
  extern int bypass_format_handling;
  /* Now we decided the above wouldn't cut it -- it's useless. */
  /* Now we pay attention to... */
  extern int force_dump_to_file;
  extern char *force_dump_filename;
  char *fnam;

  if (compressed)
    {
      /* OK, so it's compressed.  Our strategy is to dump it to a file,
         then open that file (locally) and substitute that file id
         for file_number. */
      char *znam;
      FILE *fp;

      fnam = mo_tmpnam();
      znam = (char *)malloc (sizeof (char) * (strlen (fnam) + 8));

      /* Either compressed or gzipped. */
      if (compressed == WWW_COMPRESSED)
        sprintf (znam, "%s.Z", fnam);
      else
        sprintf (znam, "%s.z", fnam);

      fp = fopen (znam, "w");
      if (!fp)
        goto bad_error;
      HTOutputBinary (file_number, fp);
      fclose (fp);

      /* Now close the original file. */
      close (file_number);
      
      /* Run uncompress. */
      {
        char *cmd;
        
        if (compressed == WWW_COMPRESSED)
          cmd = 
            (char *)malloc (strlen (uncompress_program) + strlen (znam) + 8);
        else
          cmd = 
            (char *)malloc (strlen (gunzip_program) + strlen (znam) + 8);
          
        if (compressed == WWW_COMPRESSED)
          sprintf (cmd, "%s %s\0", uncompress_program, znam);
        else
          sprintf (cmd, "%s %s\0", gunzip_program, znam);

        if ((system (cmd)) != 0)
          {
            application_user_feedback 
              ("Unable to uncompress compressed data;\nresults may be in error.\0");
          }
        free (cmd);
      }
      
      /* Now open the file that we just dumped, with its real name. */
      file_number = open(fnam, O_RDONLY, 0);
      
      /* Clean up. */
      free (znam);
      /* But don't free fnam, yet. */
    }

  if (force_dump_to_file)
    {
      /* Just dump it out. */
      FILE *fp = fopen (force_dump_filename, "w");
      if (!fp)
        goto really_bad_error;
      HTOutputBinary (file_number, fp);
      fclose (fp);

      /* If we have to, clean up our uncompressed stuff. */
      if (compressed)
        {
          char *cmd = (char *)malloc (strlen (fnam) + 32);
          sprintf (cmd, "/bin/rm -f %s\0", fnam);
          /* No error checking -- just let it die. */
          system (cmd);
          free (cmd);
          
          /* Now get rid of the filename. */
          free (fnam);
        }
      
      /* Return here. */
      return;
    }
  
  /* Warning: this is a hack.
     (See the FAQ for more info about why it's a hack.)
     
     For anything but plaintext, we just do the text as normal.
     Then instead of necessarily returning, if it's a non-text
     data format (image or whatever), we pipe to the external program
     right here, and return a string consisting of
     "<mosaic-access-override>" so the calling routine knows not
     to display this file. */

  if (!bypass_format_handling)
    {
      switch (format) 
        {
        case WWW_GIF:
          pipe_to_reader (anchor, file_number, gif_reader);
          done = 1;
          break;
        case WWW_HDF:
          pipe_to_reader (anchor, file_number, hdf_reader);
          done = 1;
          break;
        case WWW_JPEG:
          pipe_to_reader (anchor, file_number, jpeg_reader);
          done = 1;
          break;
        case WWW_TIFF:
          pipe_to_reader (anchor, file_number, tiff_reader);
          done = 1;
          break;
        case WWW_AUDIO:
          pipe_to_reader (anchor, file_number, audio_reader);
          done = 1;
          break;
        case WWW_AIFF:
          pipe_to_reader (anchor, file_number, aiff_reader);
          done = 1;
          break;
        case WWW_POSTSCRIPT:
          pipe_to_reader (anchor, file_number, postscript_reader);
          done = 1;
          break;
        case WWW_DVI:
          pipe_to_reader (anchor, file_number, dvi_reader);
          done = 1;
          break;
        case WWW_MPEG:
          pipe_to_reader (anchor, file_number, mpeg_reader);
          done = 1;
          break;
        case WWW_MIME:
          pipe_to_reader (anchor, file_number, mime_reader);
          done = 1;
          break;
        case WWW_XWD:
          pipe_to_reader (anchor, file_number, xwd_reader);
          done = 1;
          break;
        case WWW_SGIMOVIE:
          pipe_to_reader (anchor, file_number, sgimovie_reader);
          done = 1;
          break;
        case WWW_EVLMOVIE:
          pipe_to_reader (anchor, file_number, evlmovie_reader);
          done = 1;
          break;
        case WWW_RGB:
          pipe_to_reader (anchor, file_number, rgb_reader);
          done = 1;
          break;
        case WWW_CAVE:
          pipe_to_reader (anchor, file_number, cave_reader);
          done = 1;
          break;
        case WWW_UNKNOWN:
        case WWW_TAR:
          /* We get WWW_UNKNOWN when we know we want to just dump
             it to a local file and not view it.  We get WWW_TAR
             whenever the file has a .tar suffix. */
          pipe_to_reader (anchor, file_number, NULL);
          done = 1;
          break;
        case WWW_IMPOSSIBLE:
          /* If it's impossible, there's nothing we can do.  Oh well. */
          impossible = 1;
          done = 1;
          break;
        case WWW_COMPRESSED:
        case WWW_GZIPPED:
          done = 1;
          /* OK, this is a little hairy.  We send the result to a file,
             uncompress it, and then load it back in. */
          break;
        case WWW_HTML:
          break;
        case WWW_PLAINTEXT:
        default:
          /* unknown format -- Parse plain text */
          plaintext = 1;
          break;
        } /* end of switch (format) */
    }
  else
    {
      /* bypass_format_handling is non-zero */
      /* All we do is uncompress, if necessary. */
      switch (format) 
        {
        case WWW_COMPRESSED:
        case WWW_GZIPPED:
          done = 1;
          break;
        }
    }

  /* If none of the above, it must be plaintext. */
  if (!done)
    {
      text = HText_new(anchor);
      
      HTInitInput(file_number);
      
      HText_beginAppend(text);

      if (plaintext)
        HText_appendText (text, "<PLAINTEXT>\n");
      
      for (;;) 
        {
          char character;
          character = HTGetChararcter();
          if (character == (char)EOF) 
            break;
          HText_appendCharacter(text, character);
        }
      
      HText_endAppend(text);
    }
  
  /* If impossible is high, then we return an error message,
     since we've ran into a situation (e.g., a Gopher CSO server)
     that we just can't handle. */
  if (impossible)
    {
      text = HText_new(anchor);
      
      HTInitInput(file_number);
      
      HText_beginAppend(text);

      HText_appendText (text, "<H1>Impossible Situation</H1> Sorry, Mosaic has run into a situation where nothing can be done.<P>");
      
      HText_endAppend(text);
    }

  /* If we have to, clean up our uncompressed stuff. */
  if (compressed)
    {
      char *cmd = (char *)malloc (strlen (fnam) + 32);
      sprintf (cmd, "/bin/rm -f %s\0", fnam);
      /* No error checking -- just let it die. */
      system (cmd);
      free (cmd);

      /* Now get rid of the filename. */
      free (fnam);
    }

  return;

 bad_error:
  text = HText_new(anchor);
  HText_beginAppend(text);
  HText_appendText (text, "<h1>Sorry</h1>Could not load data.");
  HText_endAppend(text);

 really_bad_error:
  return;
}
