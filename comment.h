#ifndef COMMENT_H
#define COMMENT_H

#include <sys/stat.h>

struct comment_data {
	char filename[1024];
	char localname[256];
	char extension[32];
	struct stat stat;
	char datetext[16];
};

typedef struct comment_data COMMENT;

#endif

