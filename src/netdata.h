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


#ifndef HAS_NET_DATA_DOT_H_BEEN_INCLUDED_BEFORE
#define HAS_NET_DATA_DOT_H_BEEN_INCLUDED_BEFORE
#include <libdtm/vdata.h>

/* Entity type */
#define ENT_Internal	1
#define ENT_File	2
#define ENT_Network	3
#define ENT_Interactive 4

/* Data Object Type */
#define DOT_Unknown	100
#define DOT_VData	101
#define DOT_Array	102
#define DOT_Text	103
#define DOT_Palette8	104
#define DOT_SDL		105

/* Data Object SubType */
#define DOST_Float	200
#define DOST_Char	201
#define DOST_Int16	202
#define DOST_Int32	203
#define DOST_Double	204


#define MAX_ARRAY_DIM	50

/*
defined in vdata.h
typedef struct {
	int nodeID;
	char *nodeName;
	} VdataPathElement;
*/

typedef struct DATA {
	char *label;				/* data object label*/
	int entity;				/* entity type */
	int dot;				/* Data Object Type */
	int dost;				/* Data Object Subtype */
	int dim[MAX_ARRAY_DIM];			/* array of dimensions */
	int rank;				/* number of dimensions */
	char *data;				/* data */

	VdataPathElement **magicPath;		/* Vdata path */
	int  pathLength;			/* do we want this Marc? */
	int  nodeID;				/* this Vdata's ID */
	char *nodeName;				/* this Vdata's name */
	char *fields;				/* ? */

	float	expandX;			/* expand X image */
	float	expandY;			/* expand Y image */
	struct DATA *group;			/* group with any */
	} Data;

extern Data *DataNew();
extern void DataDestroy();
extern int InitData();
extern void DataAddEntry();
extern Data *DataSearchByLabel();
extern Data *DataSearchByLabelAndDOT();
extern Data *DataSearchByLabelAndDOTAndDOST();
extern int DataInList();

#endif 
