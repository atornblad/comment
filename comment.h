/**
 * @file comment.h
 * @author Anders Tornblad
 * @date 2017-08-24
 */
#ifndef COMMENT_H
#define COMMENT_H

#include <sys/stat.h>

struct comment_data {
	char filename[1024];
	char localname[256];
	char extension[32];
	struct stat stat;
	char datetext[256];
	char author[256];
};

typedef struct comment_data COMMENT;

#endif

