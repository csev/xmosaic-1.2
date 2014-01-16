/*	Include file for WorldWideWeb project-wide definitions
*/

/*	Formats:
*/

#ifndef WWW_H
#define WWW_H

/*	Bit fields describing the capabilities for a node:
*/
#define WWW_READ					1
#define WWW_WRITE					2
#define WWW_LINK_TO_NODE		4
#define WWW_LINK_TO_PART		8
#define WWW_LINK_FROM_NODE	  16
#define WWW_LINK_FROM_PART	  32
#define WWW_DO_ANYTHING	     63

#endif /* WWW_H */
