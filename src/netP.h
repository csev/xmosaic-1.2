/*
 * Copyright (C) 1992, Board of Trustees of the University of Illinois.
 *
 * Permission is granted to copy and distribute source with out fee.
 * Commercialization of this product requires prior licensing
 * from the National Center for Supercomputing Applications of the
 * University of Illinois.  Commercialization includes the integration of this 
 * code in part or whole into a product for resale.  Free distribution of 
 * unmodified source and use of NCSA software is not considered 
 * commercialization.
 *
 */

#include <sys/types.h>
#include "netdata.h"

#define PORTNAMESIZE	25

#define	NET_UNDEF	0
#define	NET_IN		1
#define	NET_OUT		2


typedef enum { NETRIS8, NETSDS, NETPAL, NETTXT, NETSRV, NETCOL, NETDTM,
		NETANIM, NETVDATA, NETSDL, NETCOM, NETEXEC, NETMSG} NetType;

typedef struct {
	int	port;
	char	portName[PORTNAMESIZE];
	char	open;	/*boolean*/
	int	type;
	time_t	queueTime;
	} NetPort;


typedef struct {
	char	*title;
	char	*id;
	int 	selLeft;	/* selection Left */
	int 	selRight;	/* selection Right */
	int 	insertPt;	/* insertion point */
        int     numReplace;     /* number to replace */
	int	replaceAll;	/* boolean should reaplace All text */
	int 	dim;		/* dimensions */
	char	*textString;
	} Text;

typedef struct {
	char	*title;
	char	*id;
	char	*func;
	int	selType;
	int	width;
	int	dim;
	struct	COL_TRIPLET *data;
	} Col;

typedef enum { AF_NO_FUNC, AF_STOP, AF_FPLAY, AF_RPLAY 
		} AnimFunc;

typedef enum { ART_NONE, ART_SINGLE, ART_CONT, ART_BOUNCE
                } AnimRunType;

typedef struct {
	char		*title;
	char		*id;
	AnimFunc 	func;
	int		frameNumber;
	AnimRunType	runType;
	Data 		*data;
	} AnimMesg;

typedef struct {
	char 	id[80];
	char	inPort[80];
	int	func;
	NetPort	*netPort;
	} Server;

typedef struct {
	char *id;
	char *domain;
	char *mesg;
	} Com;

/****************************/

typedef struct {
	float load1;
	float load5;
	float load15;
	int   numUsers;
	} ExecHostStatusReturn;

typedef union {
	ExecHostStatusReturn	hsReturn; 
	} ExecRetInfo;

typedef struct {
	char *id;
	char *retAddress;
	char *authentication;
	char *timeStamp;
	int  type;
	ExecRetInfo info;  /* addition info depending on type */
	} Exec;
/****************************/

int	NetRegisterModule();
NetPort *NetCreateInPort();
NetPort *NetCreateOutPort();
int	NetInit();
void	NetPollAndRead();
int	NetSendDisconnect();
void	NetFreeDataCB();
int	NetSendDoodle();
int	NetSendPointSelect();
int	NetSendLineSelect();
int	NetSendAreaSelect();
int	NetSendClearDoodle();
int	NetSendSetDoodle();
int	NetSendDoodleText();
int	NetSendEraseDoodle();
int	NetSendEraseBlockDoodle();
int	NetSendTextSelection();
int	NetSendPalette8();
int	NetSendRaster8();
int	NetSendRaster8Group();
