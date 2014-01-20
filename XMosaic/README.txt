NCSA XMosaic 1.2 for the Macintosh

This is working version of NCSA XMosaic 1.2 originally released by NCSA
in 1993.  I think that it only works on 10.6 and later. 
See the Copyright file for copyright details.

Pre-Requisites
==============

You need X-Windows and OpenMotif installed for this to work:

http://xquartz.macosforge.org/landing/

http://www.ist-inc.com/motif/download/index.html

Once these are installed you can start xmosaic.

Web Pages to Visit
==================

The default home page is the original NSCA page.  Another great page 
is the first page on the web:

http://info.cern.ch/

Source Code
===========

The source code is available at:

https://github.com/csev/xmosaic-1.2

I started with the official release of XMosaic and then started patching it
first to get it to compile and then to add a few features to make it
so we could surf a bit more of the web:

- It uses HTTP/1.1 instead of HTTP 1.0 so it can see virtual hosted sites

- It uses the the OS/X sips command line tool to convert all images to gif 
to give best chance of seeing them.  Somehow modern JPGs were even choking
XMosaic and the web uses a lot of PNGs.

- It ignores ampersand hex values as those were not invented yet

- It hides content between script and style tags as those were not invented yet

The idea is not to modernize the code.  Each of these changes is very surgical
without major re-work.   I want to stay as close as I can to the original code.

Have Fun
========

Thanks for taking a look.  Let me know if you have any issues.  Feel free to 
send me patches :)

Charles Severance
www.dr-chuck.com
Sun Jan 19 20:12:16 EST 2014

