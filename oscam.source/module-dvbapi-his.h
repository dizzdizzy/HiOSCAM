#ifndef _MODULE_HIAPI_H_
#define _MODULE_HIAPI_H_

#define DEMUX_MAINADAPTER		3
#define DEMUX_SECONDADAPTER	2

/* Local hiapi functions */
int32_t 	hidemuxapi_Init(void);
void    	hidemuxapi_Deinit(void);
int32_t 	hidemuxapi_AddFilters(int32_t adapter, int32_t dmuxid, int32_t type, int32_t n,int32_t pid, uchar *flt, uchar *mask);
int32_t 	hidemuxapi_RemoveFilters(int32_t adapter, int32_t dmuxid, int32_t type, int32_t n, int32_t pid);

#endif
