
/* dmf.h,v 1.3 1993/03/25 19:37:26 marca Exp */



#define	DMFERROR	0
#define DMFOK		!DMFERROR

#define DMF_MAX_NUM_FIELDS  40
#define DMF_BUFSIZE			256 * 1024

/* match structure list */
typedef struct  m_struct {
    int 		keyid;
    char 		datatype[32];
    char 		matchblob[1];
} MATCH;

typedef enum { QF_LIST, QF_STRING, QF_TOGGLE, QF_OPTION } QFTYPE;

typedef struct QF {
	QFTYPE		type;
	char		*label;
	char		*help;
	char		*data;
	struct QF	*next;
} QFIELD;

/* commonly used strings supported by DMFsendInquire */
#define	INQ_FULLINFO	"FULLINFO"
#define	INQ_HISTORY	"HISTORY"
#define	INQ_COMMENT	"COMMENT"
#define	INQ_DERIVED	"DERIVED"
#define	INQ_RELATED	"RELATED"

/* file formats supported by file transfer */
#define	DMF_FF_BACKGROUND	128
#define	DMF_FF_COMPRESSED	2
#define DMF_FF_TARRED		1

/* request types current supported */
typedef enum { REQ_FILE, REQ_DATA } REQ_TYPE;

/* message classes used with DMF */
#define	EXECclass				"EXEC"
#define CREQclass				"CREQ"
#define	DATAclass				"DATA"
#define	DBASEclass				"DBASE"
#define	CONFIRMclass			"CONFIRM"
#define	QUERYclass				"QUERY"
#define	MATCHclass				"MATCH"
#define INQUIREclass			"INQUIRE"
#define	INFORMclass				"INFORM"
#define	INFOclass				"INFO"
#define	REQUESTclass			"REQUEST"
#define OPTIONclass				"OPTION"
#define	TOGGLEclass				"TOGGLE"
#define FILEclass				"FILE"
#define COMPLETEclass			"COMPLETE"
#define ERRORclass				"DMFERROR"


/* error message index */
typedef enum {
	DMF_NoError,
	DMF_OutOfMem,				/* insufficent memory */
	DMF_DTMError,				/* DTM error occured */
	DMF_ItemNotFound			/* file could not located */
} DMFERR;


#define	EXECcompareClass(h)		dtm_compare_class(h, EXECclass)
#define	CREQcompareClass(h)		dtm_compare_class(h, CREQclass)
#define	DATAcompareClass(h)		dtm_compare_class(h, DATAclass)
#define DBASEcompareClass(h)	dtm_compare_class(h, DBASEclass)
#define	CONFIRMcompareClass(h)	dtm_compare_class(h, CONFIRMclass)
#define	QUERYcompareClass(h)	dtm_compare_class(h, QUERYclass)
#define	MATCHcompareClass(h)	dtm_compare_class(h, MATCHclass)
#define	INQUIREcompareClass(h)	dtm_compare_class(h, INQUIREclass)
#define	INFORMcompareClass(h)	dtm_compare_class(h, INFORMclass)
#define	INFOcompareClass(h)	    dtm_compare_class(h, INFOclass)
#define	REQUESTcompareClass(h)	dtm_compare_class(h, REQUESTclass)
#define	OPTIONcompareClass(h)	dtm_compare_class(h, OPTIONclass)
#define	TOGGLEcompareClass(h)	dtm_compare_class(h, TOGGLEclass)
#define FILEcompareClass(h)		dtm_compare_class(h, FILEclass)


extern int ESERVsendExecReq();
extern int ESERVrecvExecReq();

extern int ESERVsendDataReq();
extern int ESERVrecvDataReq();

extern int ESERVsendDbaseReq();
extern int ESERVrecvDbaseReq();

extern int DMFsendConfirm();
extern int DMFrecvConfirm();

extern int DMFsendQuery();
extern int DMFrecvQuery();

extern int DMFsendMatch();
extern int DMFrecvMatch();
extern int DMFinitMatch();
extern int DMFdecodeMatch();
extern int DMFappendMatch();
extern int DMFendMatch();
extern int DMFfreeMatch();

extern int DMFsendInquire();
extern int DMFrecvInquire();

extern int DMFsendInform();
extern int DMFrecvInform();

extern int DMFsendInfo();
extern int DMFrecvInfo();

extern int DMFsendRequest();
extern int DMFrecvRequest();

extern int DMFsendOption();
extern int DMFrecvOption();

extern int DMFsendToggle();
extern int DMFrecvToggle();

extern int DMFsendFile();
extern int DMFrecvFile();

extern void DMFfreeFields();

extern int DMFsendComplete();

extern void DMFsendError();
extern void DMFrecvError();

/* added for backward compatibility */
#define		DMFsendExecReq		ESERVsendExecReq
#define		DMFrecvExecreq		ESERVrecvExecReq
#define		DMFsendDataReq		ESERVsendDataReq
#define		DMFrecvDataReq		ESERVrecvDataReq
#define		DMFsendDbase		ESERVsendDbaseReq
#define		DMFrecvDbase		ESERVrecvDbaseReq








