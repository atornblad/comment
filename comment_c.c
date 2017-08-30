/**
 * @file comment_c.c
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
#include "comment_c.h"

static int addNewCComment(COMMENT *data);
static int modifyCComment(COMMENT *data, int indexOfDate, int indexOfDateEnd);

/* BETTER SOLUTION:
 * This could be done much simpler by checking line by line:
 * First look for (/)! (*)! (*)!
 * Then look for (backspace)? (*)? (backspace)? @date (backspace)! (whatever)
 * Then look for (whatever) (*)! (/)! (backspace)?
 *
 * Keep track of line index instead of byte index
 */

/* STATES
 *  0 : Looking for /                                   -> 1 / 0
 *  1 : Looking for *                                   -> 2 / unget,0
 *  2 : Looking for *                                   -> 3 / unget,12
 *  3 : Looking for @ or *                              -> 5 / 4 / 3
 *  4 : Looking for / (after state 3 found *)           -> 0 / unget,3
 *  5 : Looking for d                                   -> 6 / unget,3
 *  6 : Looking for a                                   -> 7 / unget,3
 *  7 : Looking for t                                   -> 8 / unget,3
 *  8 : Looking for e                                   -> 9 / unget,3
 *  9 : Looking for \n or other-non-whitespace          -> 11 / 10 / 9
 * 10 : Looking for \n                                  -> 11 / 10
 * 11 : Done!
 * 12 : Looking for * (inside non-doxy comment)         -> 13
 * 13 : Looking for / or *                              -> 0 / 13 / 12
 * If (10) happens, save the point of transition from 9->10
 * If (11) happens, save the point of transition from (9/10)->11, and stop the scan
 * If (11) happens but not (10), use the transition from 9->11 for both points
 * If (11) hasn't happened after a full scan, there is no date comment in the file
 */

int commentC(COMMENT *data) {
	FILE *f = fopen(data->filename, "r");
	if (!f) {
		fprintf(stderr, "Could not open file '%s'\n", data->filename);
		return 1;
	}

	int state = 0;
	int index = 0;
	int indexOfDate = -1;
	int indexOfDateEnd = -1;
	int ch;

	while (state != 11 && (ch = fgetc(f)) != EOF) {
		int newState = state;

		if (state == 0 && ch == '/') newState = 1;

		else if (state == 1 && ch == '*') newState = 2;
		else if (state == 1) {ungetc(ch,f); newState = 0;}

		else if (state == 2 && ch == '*') newState = 3;
		else if (state == 2) {ungetc(ch,f); newState = 12;}

		else if (state == 3 && ch == '@') newState = 5;
		else if (state == 3 && ch == '*') newState = 4;

		else if (state == 4 && ch == '/') newState = 0;
		else if (state == 4) {ungetc(ch,f); newState = 3;}

		else if (state == 5 && ch == 'd') newState = 6;
		else if (state == 5) {ungetc(ch,f); newState = 3;}

		else if (state == 6 && ch == 'a') newState = 7;
		else if (state == 6) {ungetc(ch,f); newState = 3;}

		else if (state == 7 && ch == 't') newState = 8;
		else if (state == 7) {ungetc(ch,f); newState = 3;}

		else if (state == 8 && ch == 'e') newState = 9;
		else if (state == 8) {ungetc(ch,f); newState = 3;}

		else if (state == 9 && ch == '\n') newState = 11;
		else if (state == 9 && ch != ' ' && ch != '\t') newState = 10;

		else if (state == 10 && ch == '\n') newState = 11;

		else if (state == 12 && ch == '*') newState = 13;

		else if (state == 13 && ch == '/') newState = 0;
		else if (state == 13 && ch == '*') newState = 13;
		else if (state == 13) newState = 12;

		if (state != newState) {
			if (newState == 10) {
				indexOfDate = index;
			}
			else if (newState == 11) {
				indexOfDateEnd = index;
			}
		}

		state = newState;
		index = ftell(f);
	}

	fclose(f);

	if (indexOfDateEnd == -1) {
		return addNewCComment(data);
	}
	else {
		if (indexOfDate == -1) indexOfDate = indexOfDateEnd;
		return modifyCComment(data, indexOfDate, indexOfDateEnd);
	}
}

static int addNewCComment(COMMENT *data) {
	/* Create a new temp file, containing a new doxygen comment, and the full source file */
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
	fprintf(temp, "/" "**\n");
	fprintf(temp, " * @file %s\n", data->localname);
	fprintf(temp, " * @author %s\n", data->author);
	fprintf(temp, " * @date %s\n", data->datetext);
	fprintf(temp, " */\n");
	FILE *input = fopen(data->filename, "r");
	if (!input) {
		fprintf(stderr, "When copying existing parts of original file, adding a new C comment,\n"
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
		fprintf(stderr, "When copying temporary file over the original file, adding a new\n"
				"C comment, the original file could not be overwritten: %s\n", strerror(errno));
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
	close(tempfd);

	/* Touch the file! */
	struct utimbuf utb;
	utb.actime = data->stat.st_atime;
	utb.modtime = data->stat.st_mtime;
	utime(data->filename, &utb);

	return 0;
}

static int modifyCComment(COMMENT *data, int indexOfDate, int indexOfDateEnd) {
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
		fprintf(stderr, "When copying existing parts of original file, modifying a C comment,\n"
				"the original file could not be opened: %s\n", strerror(errno));
		fflush(temp);
		fclose(temp);
		close(tempfd);
		return 2;
	}

	int ch;
	int index = 0;
	/* Copy everything until existing date */
	while (index < indexOfDate) {
		ch = fgetc(input);
		fputc(ch, temp);
		index = ftell(input);
	}
	/* Copy date text */
	const char *date = &data->datetext[0];
	while (*date) {
		fputc(*date, temp);
		++date;
	}
	/* Skip existing date */
	while (index < indexOfDateEnd) {
		ch = fgetc(input);
		index = ftell(input);
	}
	/* Copy everything after */
	while ((ch = fgetc(input)) != EOF) {
		fputc(ch, temp);
	}
	fflush(temp);
	fclose(input);

	/* Copy the temp file to the original filename */
	fseek(temp, 0, SEEK_SET);
	FILE *output = fopen(data->filename, "w");
	if (!output) {
		fprintf(stderr, "When copying temporary file over the original file, modifying a\n"
				"C comment, the original file could not be overwritten: %s\n", strerror(errno));
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
