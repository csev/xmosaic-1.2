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


#include "netP.h"

extern	int	NetSetASync();
extern	int	NetRegisterModule();
extern	NetPort *NetCreateInPort();
extern	NetPort *NetCreateOutPort();
extern	int	NetInit();
extern	void	NetPollAndRead();
extern	void	NetTryResend();
extern	int	NetSendDisconnect();
extern	void	NetFreeDataCB();
extern	int	NetSendDoodle();
extern	int	NetSendPointSelect();
extern	int	NetSendLineSelect();
extern	int	NetSendAreaSelect();
extern	int	NetSendClearDoodle();
extern	int	NetSendSetDoodle();
extern	int	NetSendDoodleText();
extern	int	NetSendEraseDoodle();
extern	int	NetSendEraseBlockDoodle();
extern	int	NetSendTextSelection();
extern	int	NetSendPalette8();
extern	int	NetSendRaster8();
extern	int	NetSendRaster8Group();
extern	int	NetGetListOfUsers();
