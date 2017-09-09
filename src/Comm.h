#ifndef __COMM_H__
#define __COMM_H__

#include <stdio.h>

#define dbg(fmt, ...) do {printf("[DEBUG %s:%s:%d] " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);} while(0)
#define info(fmt, ...) do {printf("[INFO  %s:%s:%d] " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);} while(0)
#define warn(fmt, ...) do {printf("[WARN  %s:%s:%d] " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);} while(0)
#define err(fmt, ...) do {printf("[ERROR %s:%s:%d] " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);} while(0)

#if defined(_WIN32)
#define usleep(_x) Sleep((_x)/1000)
#endif

#endif
