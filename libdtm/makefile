#
#		Makefile for libdtm
#
#	USAGE:
#		make all - if you have the $ARCH variable set
#					setenv ARCH .`arch`
#		make sun - if you want to build for the sun
#		make XXX - for any machine in particular
#		make spec - to build a version libdtmXXX.a  where XXX is $ARCH
#

.SUFFIXES:	$(ARCH).o

DTMINC = .

OBJ=dtminit.o dtm.o dtmmisc.o socket.o rwrtns.o fatal.o sds.o sdl.o ris.o \
	dtmnserv.o ninit.o nmsg.o callback.o vdata.o
OBJ_SPEC=dtminit$(ARCH).o dtm$(ARCH).o dtmmisc$(ARCH).o socket$(ARCH).o \
	rwrtns$(ARCH).o fatal$(ARCH).o sds$(ARCH).o sdl$(ARCH).o ris$(ARCH).o \
	dtmnserv$(ARCH).o ninit$(ARCH).o nmsg$(ARCH).o callback$(ARCH).o

DEPENDS = make.depend$(ARCH)

CFT = cft77
FOBJS = dtmf77.o crayinit.o

dtmlib	: dtm_$(ARCH)
all		: dtm_$(ARCH)
spec	: $(ARCH)
all_spec: $(ARCH)

dtm_.sun4 : sun
dtm_.sgi : sgi
dtm_.next : next
dtm_.cray : cray
dtm_.ibm : ibm
dtm_.convex : convex
dtm_.dec : dec
dtm_.vax : vax

dtm_	::
	@echo Either set up the ARCH environment or specify a machine type
	@echo "		ex: make sun"

sun4	: sun
sun		: 
	make internal "CFLAGS = -DSUN -target $(TARGET_ARCH:-%=%)" \
				  "OBJECTS = $(OBJ) convert.o mdd.o"  \
				  "ARCHIVE = ar cr"  
	ranlib libdtm.a 
.sun4	:
	make internal_spec "CFLAGS = -g -DSUN -target $(TARGET_ARCH:-%=%)" \
				  "OBJECTS = $(OBJ_SPEC) convert$(ARCH).o mdd$(ARCH).o" \
				  "ARCHIVE = ar cr" 
	ranlib libdtm$(ARCH).a
next		: 
	make internal "CFLAGS = -DNEXT " \
				  "OBJECTS = $(OBJ) convert.o mdd.o"  \
				  "ARCHIVE = ar cr"  
	ranlib libdtm.a 
sgi		: 
	make internal "CFLAGS = -DSGI -cckr -I/usr/include/sun -I/usr/include/bsd" \
				  "OBJECTS = $(OBJ) convert.o" \
				  "ARCHIVE = ar cr" 
.sgi	:
	make internal_spec "CFLAGS = -DSGI -I/usr/include/sun -I/usr/include/bsd" \
				  "OBJECTS = $(OBJ_SPEC) convert$(ARCH).o" \
				  "ARCHIVE = ar cr"
cray	: 
	make internal "CFLAGS = -DCRAY" "CFT = cft77" \
		"OBJECTS = $(OBJ) craycvt.o crayrtns.o" \
		"ARCHIVE = bld q"  
	make libdtmf77.a "CFLAGS = -DCRAY" "CFT = cft77" \
		"OBJECTS = $(OBJ) craycvt.o crayrtns.o sdsF.o" \
		"ARCHIVE = bld q" 
convex	: 
	make internal "CFLAGS = -DCONVEX -fi" \
		"OBJECTS = $(OBJ) mdd.o convert.o" "ARCHIVE = ar cr"
	ranlib libdtm.a
ibm		: 
	make internal "CFLAGS = -DRS6000" "OBJECTS = $(OBJ) mdd.o convert.o" \
			"ARCHIVE = ar cr"
	ranlib libdtm.a
dec		: 
	make internal "CFLAGS = -DDEC" "OBJECTS = $(OBJ) mdd.o deccvt.o" \
		"ARCHIVE = ar cr"
	ar ts libdtm.a
vax		: 
	make internal "CFLAGS = -DVAX" "OBJECTS = $(OBJ) mdd.o vaxcvt.o" \
		"ARCHIVE = ar cr" 

%$(ARCH).o: %.c
	$(CC) $(CFLAGS) $(DTMCFLAGS) -c $< -o $@

.c$(ARCH).o : $$
	$(CC) $(CFLAGS) $(DTMCFLAGS) -c $< -o $@

.c.o : $$
	$(CC) $(CFLAGS) $(DTMCFLAGS) $(CPPFLAGS) -c $<

internal : $(OBJECTS)
	  	  $(ARCHIVE) libdtm.a `lorder $(OBJECTS) | tsort`

internal_spec : $(OBJECTS)
	  	  $(ARCHIVE) libdtm$(ARCH).a `lorder $(OBJECTS) | tsort`

libdtm$(ARCH).a	: $(OBJ_SPEC)
	  	  $(ARCHIVE) libdtm.a `lorder $(OBJ_SPEC) | tsort`

libdtmf77.a	: $(OBJECTS) $(FOBJS)
		  $(ARCHIVE) libdtmf77.a `lorder $(OBJECTS) $(FOBJS) | tsort`

depend::
	makedepend -f $(DEPENDS) -I$(DTMINC) *.c *.f
	sed -e "s/\.o/$(ARCH).o/1w .depend.temp" -e "d" < $(DEPENDS)
	cat .depend.temp >> $(DEPENDS)

clean::
	-rm -f *.o *.a

install::
	-mv $(DIR)/lib/libdtm.a $(DIR)/lib/libdtm.bak
	cp *.a $(DIR)/lib 
	-ranlib $(DIR)/lib/libdtm.a
	-rm -f $(DIR)/include/dtm.h $(DIR)/include/sds.h
	-rm -f $(DIR)/include/ris.h $(DIR)/include/db.h
	-rm -f $(DIR)/include/mdd.h $(DIR)/include/sdl.h
	-rm -f $(DIR)/include/debug.h $(DIR)/include/arch.h
	cp dtm.h debug.h sds.h ris.h db.h mdd.h sdl.h arch.h $(DIR)/include	

include $(DEPENDS) 
