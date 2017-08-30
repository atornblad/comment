/**
 * @file comment_makefile.c
 * @author Anders Tornblad
 * @date 2017-08-30
 */
#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "comment_makefile.h"

static int addNewMakefileComment(COMMENT *data);
static int modifyMakefileComment(COMMENT *data, int indexOfDateLine, int extraSpace, int upperCaseFirst);

int commentMakefile(COMMENT *data) {
	FILE *f = fopen(data->filename, "r");
	if (!f) {
		fprintf(stderr, "Could not open file '%s'\n", data->filename);
		return 1;
	}

	int dateLineIndex = -1;
	int lineIndex = 0;
	char line[1024];
	int firstPart = 1;
	int extraSpace = 0;
	int upperCaseFirst = 0;

	while (dateLineIndex == -1 && fgets(line, 1024, f) != NULL) {
		int lineContinues;

		if (line[strlen(line) - 1] == '\n') {
			lineContinues = 0;
			line[strlen(line) - 1] = '\0';
		}
		else {
			lineContinues = 1;
		}

		if (firstPart) {
			if (strncmp("# Date:", line, 7) == 0) {
				dateLineIndex = lineIndex;
				extraSpace = 1;
				upperCaseFirst = 1;
			}
			else if (strncmp("#Date:", line, 6) == 0) {
				dateLineIndex = lineIndex;
				extraSpace = 0;
				upperCaseFirst = 1;
			}
			else if (strncmp("# date:", line, 7) == 0) {
				dateLineIndex = lineIndex;
				extraSpace = 1;
				upperCaseFirst = 0;
			}
			else if (strncmp("#date:", line, 6) == 0) {
				dateLineIndex = lineIndex;
				extraSpace = 0;
				upperCaseFirst = 0;
			}

			++lineIndex;
		}

		firstPart = !lineContinues;
	}

	if (dateLineIndex >= 0) {
		return modifyMakefileComment(data, dateLineIndex, extraSpace, upperCaseFirst);
	}
	else {
		return addNewMakefileComment(data);
	}
}

static int modifyMakefileComment(COMMENT *data, int dateLineIndex, int extraSpace, int upperCaseFirst) {
	/* Create a new temp file */
	char tempname[512];
	sprintf(tempname, "/tmp/comment-%s-XXXXXX", data->localname);
	int tempfd = mkstemp(tempname);
	if (tempfd < 1) {
		fprintf(stderr, "Could not create temporary file: %s\n", strerror(errno));
		return 2;
	}

	unlink(tempname);
	FILE *temp = fdopen(tempfd, "w+");
	if (!temp) {
		fprintf(stderr, "Could not create temporary file\n");
		return 2;
	}

	FILE *input = fopen(data->filename, "r");
	if (!input) {
		fprintf(stderr, "When copying existing parts of original file, adding a new Makefile comment,\n"
				"the original file could not be opened: %s\n", strerror(errno));
		fflush(temp);
		fclose(temp);
		close(tempfd);
		return 2;
	}

	int ch;
	int lineIndex = 0;
	char buffer[1024];
	/* Copy everything until existing date, skipping the existing line */
	while (lineIndex <= dateLineIndex) {
		fgets(buffer, 1024, input);
		if (lineIndex != dateLineIndex) {
			fputs(buffer, temp);
		}

		if (buffer[strlen(buffer) - 1] == '\n') {
			++lineIndex;
		}
	}

	/* Write date line */
	fprintf(temp, "#%s%cate: %s\n",
		(extraSpace ? " " : ""),
		(upperCaseFirst ? 'D' : 'd'),
		data->datetext);

	/* Copy everything after */
	while (fgets(buffer, 1024, input) != NULL) {
		fputs(buffer, temp);
	}
	fflush(temp);
	fclose(input);

	/* Copy the temp file to the original filename */
	fseek(temp, 0, SEEK_SET);
	FILE *output = fopen(data->filename, "w");
	if (!output) {
		fprintf(stderr, "When copying temporary file over the original file, adding a new\n"
				"Makefile comment, the original file could not be overwritten: %s\n", strerror(errno));
		fclose(temp);
		close(tempfd);
		return 2;
	}

	while ((ch = fgetc(temp)) != EOF) {
		fputc(ch, output);
	}
	fflush(output);
	fclose(output);
	fclose(temp);

	/* Touch the file */
	struct utimbuf utb;
	utb.actime = data->stat.st_atime;
	utb.modtime = data->stat.st_mtime;
	utime(data->filename, &utb);

	return 0;
}

static int addNewMakefileComment(COMMENT *data) {
	/* Create a new temp file, containing a new comment, and the full makefile */
	char tempname[512];
	sprintf(tempname, "/tmp/comment-%s-XXXXXX", data->localname);
	int tempfd = mkstemp(tempname);
	if (tempfd < 1) {
		fprintf(stderr, "Could not create temporary file: %s\n", strerror(errno));
		return 2;
	}

	unlink(tempname);
	FILE *temp = fdopen(tempfd, "w+");
	if (!temp) {
		fprintf(stderr, "Could not create temporary file\n");
		return 2;
	}

	fprintf(temp, "# Makefile\n");
	fprintf(temp, "# Author: %s\n", data->author);
	fprintf(temp, "# Date: %s\n", data->datetext);
	fprintf(temp, "\n");

	FILE *input = fopen(data->filename, "r");
	if (!input) {
		fprintf(stderr, "When copying existing parts of original file, modifying a Makefile comment,\n"
				"the original file could not be opened: %s\n", strerror(errno));
		fflush(temp);
		fclose(temp);
		close(tempfd);
		return 2;
	}

	int ch;
	while ((ch = fgetc(input)) != EOF) {
		fputc(ch, temp);
	}
	fclose(input);
	fflush(temp);

	/* Copy the temp file to the original filename */
	fseek(temp, 0, SEEK_SET);
	FILE *output = fopen(data->filename, "w");
	if (!output) {
		fprintf(stderr, "When copying temporary file over the original file, modifying a\n"
				"Makefile comment, the original file could not be overwritten: %s\n", strerror(errno));
		fclose(temp);
		close(tempfd);
		return 2;
	}

	while ((ch = fgetc(temp)) != EOF) {
		fputc(ch, output);
	}
	fflush(output);
	fclose(output);
	fclose(temp);

	/* Touch the file! */
	struct utimbuf utb;
	utb.actime = data->stat.st_atime;
	utb.modtime = data->stat.st_mtime;
	utime(data->filename, &utb);

	return 0;
}



