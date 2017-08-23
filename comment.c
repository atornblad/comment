/**
 * @file comment.c
 * @brief main program for comment tool
 * @author Anders Tornblad
 * @date 2017-08-23
 */

/* IDEA:
 * comment --config "name=Anders Tornblad"
 * comment --config "dateformat=yyyy-mm-dd"
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "comment.h"
#include "comment_c.h"
#include "comment_makefile.h"
#include "comment_tex.h"
#include "comment_sh.h"

static void comment(const char *filename);

int main(int argc, char *argv[]) {
	for (int i = 1; i < argc; ++i) {
		comment(argv[i]);
	}
}

static void analyzeAndComment(COMMENT *data) {
	fprintf(stderr, "analyzeAndComment({localname=\"%s\", extension=\"%s\", st_mtime=%ld})\n",
		data->localname, data->extension, data->stat.st_mtime);
	if (strcmp("makefile", data->localname) == 0) {
		commentMakefile(data);
	}
	else if (strcmp("Makefile", data->localname) == 0) {
		commentMakefile(data);
	}
	else if (strcmp(".c", data->extension) == 0) {
		commentC(data);
	}
	else if(strcmp(".h", data->extension) == 0) {
		commentC(data);
	}
	else if(strcmp(".mk", data->extension) == 0) {
		commentMakefile(data);
	}
	else if(strcmp(".tex", data->extension) == 0) {
		commentTex(data);
	}
	else if(strcmp(".sh", data->extension) == 0) {
		commentSh(data);
	}
	else {
		fprintf(stderr, "Cannot add comment to: '%s'\nDon't know what type of file it is.\n", data->filename);
	}
}

static void comment(const char *filename) {
	fprintf(stderr, "comment(\"%s\")\n", filename);

	if (filename == NULL) {
		return;
	}

	char *lastSlash = strrchr(filename, '/');
	const char *local;
	if (lastSlash == NULL) {
		local = filename;
	} else {
		local = lastSlash + 1;
	}

	COMMENT data;
	strcpy(data.filename, filename);
	strcpy(data.localname, local);
	stat(filename, &data.stat);
	strftime(data.datetext, 15, "%F", gmtime(&data.stat.st_mtime));

	char *lastDot = strrchr(filename, '.');
	if (lastDot == NULL) {
		data.extension[0] = '\0';
	}
	else {
		int localIndex = (int)(local - filename);
		int dotIndex = (int)(lastDot - filename);
		if (localIndex > dotIndex) {
			data.extension[0] = '\0';
		} else {
			strcpy(data.extension, lastDot);
		}
	}

	analyzeAndComment(&data);
}

