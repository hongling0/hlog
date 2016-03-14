#include "hlog.h"

extern struct hlog_opt hfilelog_sizerotate_opt;

struct hlog_opt *HLOG_OPT_LIST[]=
{
	&hfilelog_sizerotate_opt
};
