#include	<stdio.h>
#include	<string.h>
#include	<dtm.h>

#include	"dmf.h"


int DMFsendOption(port, label, value)
	int		port;
	char	*label, *value;
{
	char	header[DTM_MAX_HEADER];

	dtm_set_class(header, OPTIONclass);
	dtm_set_char(header, "LABEL", label);
	dtm_set_char(header, "VALUE", value);

	DTMwriteMsg(port, header, DTMHL(header), NULL, 0, DTM_CHAR);
}


int DMFrecvOption(port, header, label, value)
	int		port;
	char	*header, **label, **value;
{
	char	s[DTM_MAX_HEADER];

	dtm_get_char(header, "LABEL", s, sizeof s);
	*label = strdup(s);

	dtm_get_char(header, "VALUE", s, sizeof s);
	*value = strdup(s);
}


int DMFsendToggle(port, label)
	int		port;
	char	*label;
{
	char	header[DTM_MAX_HEADER];


	dtm_set_class(header, OPTIONclass);
	dtm_set_char(header, "LABEL", label);

	DTMwriteMsg(port, header, DTMHL(header), NULL, 0, DTM_CHAR);
}


int DMFrecvToggle(port, header, label)
	int		port;
	char	*header, **label;
{
	char	s[DTM_MAX_HEADER];

	dtm_get_char(header, "LABEL", s, sizeof s);
	*label = strdup(s);
}
