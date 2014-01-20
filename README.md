xmosaic-1.2
===========

A Place for me to play with XMosaic-1.2 - Mostly to try to get it to 
work on a Mac.

Pre-Requisites - Download and install X-Windows and OpenMotif from:

http://xquartz.macosforge.org/landing/

http://www.ist-inc.com/motif/download/index.html

OpenMotif installs into /usr/OpenMotif

Here is a compiled ![Binary Distribution](XMosaic.zip) that should work 
on Max OS/X 10.5 or later (with the pre-requisites installed).

You can look at the NCSA README but here are the quick instructions.

    cd libwww
    make clean all
    cd ../libhtmlw
    make clean all
    cd ../src
    make clean all

From the src directory start xmosaic:

    ./xmosaic &

I have made very few changes to let it browse a bit more of the web
reasonably:

* It uses HTTP/1.1 instead of HTTP 1.0 so it can see virtual hosted sites

* It uses sips to convert all images to gif to give best chance of seeing them.

* It ignores ampersand hex values as those were not invented yet

* It hides content between script and style tags as those were not invented yet

It also is pretty chatty in the console while it is running - I am still 
working on the code and want to see if it breaks.

![XMosaic 1.2 running on Max OSX](XMosaic/xmosaic.jpg)

![XMosaic 1.2 viewing info.cern.ch](XMosaic/info-cern.jpg)

