#include "hlog.h"

extern struct hlog_opt sizerotatefilelog_opt;
extern struct hlog_opt timerotatefilelog_opt;

struct hlog_opt *HLOG_OPT_LIST[]=
{
	&sizerotatefilelog_opt,
	&timerotatefilelog_opt
};
