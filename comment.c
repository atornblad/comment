/**
 * @file comment.c
 * @brief main program for comment tool
 * @author Anders Tornblad
 * @date 2017-08-24
 */

/* IDEA:
 * comment --config global name "Anders Tornblad"
 * comment --config global dateformat %F
 * comment --config name "anto1700"
 * comment --config dateformat %y%m%d%H%M
 * comment --help
 */

#define _XOPEN_SOURCE
#define _XOPEN_SOURCE_EXTENDED
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include "comment.h"
#include "comment_c.h"
#include "comment_makefile.h"
#include "comment_tex.h"
#include "comment_sh.h"

static int comment(const char *filename);
static int setConfig(int argc, const char **args);
static void readConfig(int globalOnly);

static char author[256];
static int authorIsGlobal;
static int authorIsDefault;

static char dateformat[256];
static int dateformatIsGlobal;
static int dateformatIsDefault;

int main(int argc, char *argv[]) {
	if (argc >= 2 && strcmp("--config", argv[1]) == 0) {
		return setConfig(argc - 2, (const char **)&argv[2]);
	}
	else {
		readConfig(0);

		for (int i = 1; i < argc; ++i) {
			int result = comment(argv[i]);
			if (result) return result;
		}

		return 0;
	}
}

static void readConfigFile(const char *filename, int global) {
	FILE *f = fopen(filename, "r");
	if (!f) return;

	char key[256];
	char value[256];

	while (fgets(key, 256, f) != NULL) {
		int len = strlen(key);
		if (key[len - 1] == '\n') key[len - 1] = '\0';
		
		if (fgets(value, 256, f) != NULL) {
			len = strlen(value);
			if (value[len - 1] == '\n') value[len - 1] = '\0';

			if (strcmp("author", key) == 0) {
				strcpy(author, value);
				authorIsGlobal = global;
				authorIsDefault = 0;
			}
			else if (strcmp("dateformat", key) == 0) {
				strcpy(dateformat, value);
				dateformatIsGlobal = global;
				dateformatIsDefault = 0;
			}
		}
		else {
			break;
		}
	}

	fclose(f);
}

static char GLOBAL_CONFIG_FILENAME[1024];
static const char * const LOCAL_CONFIG_FILENAME = "./.comment-data";

static void prepareConfig() {
	strcpy(GLOBAL_CONFIG_FILENAME, getenv("HOME"));
	strcat(GLOBAL_CONFIG_FILENAME, "/.config/comment-data");
}

static void readConfig(int globalOnly) {
	prepareConfig();

	cuserid(author);
	authorIsGlobal = 0;
	authorIsDefault = 1;

	strcpy(dateformat, "%F");
	dateformatIsGlobal = 0;
	dateformatIsDefault = 1;
	
	readConfigFile((const char *)GLOBAL_CONFIG_FILENAME, 1);

	if (!globalOnly) {
		readConfigFile(LOCAL_CONFIG_FILENAME, 0);
	}
}

static int setConfigFileValue(const char *filename, const char *setting, const char *value) {
	char tempname[40];
	strcpy(tempname, "/tmp/comment-data-XXXXXX");
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

	FILE *input = fopen(filename, "r");
	if (input) {
		/* The file already exists - read key/value pairs and only copy those that do NOT match setting */
		char key[256];
		char value[256];
		while (fgets(key, 256, input) != NULL) {
			if (key[strlen(key) - 1] == '\n') key[strlen(key) - 1] = '\0';
			if (fgets(value, 256, input) != NULL) {
				if (value[strlen(value) - 1] == '\n') value[strlen(value) - 1] = '\0';
				if (strcmp(key, setting) != 0) {
					if (strlen(key) >= 1 && strlen(setting) >= 1) {
						fprintf(temp, "%s\n%s\n", key, value);
					}
				}
			}
			else {
				break;
			}
		}
		fclose(input);
	}

	if (value != NULL) {
		fprintf(temp, "%s\n", setting);
		fprintf(temp, "%s\n", value);
	}
	fflush(temp);

	/* rewind temp file and start copying it */
	fseek(temp, 0, SEEK_SET);

	FILE *output = fopen(filename, "w");
	if (!output) {
		fprintf(stderr, "Could not open config file '%s' for writing: %s\n", filename, strerror(errno));
		fclose(temp);
		return 2;
	}

	char line[256];
	while (fgets(line, 256, temp)) {
		fputs(line, output);
	}
	fflush(output);
	fclose(output);
	fclose(temp);
	
	return 1;
}

static int setConfigValue(int global, const char *setting, const char *value) {
	prepareConfig();

	return setConfigFileValue((global ? ((const char *)GLOBAL_CONFIG_FILENAME) : LOCAL_CONFIG_FILENAME), setting, value);
}

static int showConfig(int global) {
	readConfig(global);

	fprintf(stdout, "author: '%s' (%s)\n", author,
		(authorIsDefault ? "default" : (authorIsGlobal ? "global" : "local")));
	fprintf(stdout, "dateformat: '%s' (%s)\n", dateformat,
		(dateformatIsDefault ? "default" : (dateformatIsGlobal ? "global" : "local")));
	return 0;
}

static int setConfig(int argc, const char **args) {
	int global = 0;

	if (argc >= 1 && strcmp("global", *args) == 0) {
		global = 1;
		--argc;
		args++;
	}

	if (argc >= 2) {
		return setConfigValue(global, args[0], args[1]);
	}
	else if (argc == 1) {
		return setConfigValue(global, args[0], NULL);
	}
	else {
		return showConfig(global);
	}
}

static int analyzeAndComment(COMMENT *data) {
	if (strcmp("makefile", data->localname) == 0) {
		return commentMakefile(data);
	}
	else if (strcmp("Makefile", data->localname) == 0) {
		return commentMakefile(data);
	}
	else if (strcmp(".c", data->extension) == 0) {
		return commentC(data);
	}
	else if(strcmp(".h", data->extension) == 0) {
		return commentC(data);
	}
	else if(strcmp(".mk", data->extension) == 0) {
		return commentMakefile(data);
	}
	else if(strcmp(".tex", data->extension) == 0) {
		return commentTex(data);
	}
	else if(strcmp(".sh", data->extension) == 0) {
		return commentSh(data);
	}
	else {
		fprintf(stderr, "Cannot add comment to: '%s'\nDon't know what type of file it is.\n", data->filename);
		return 1;
	}
}

static int comment(const char *filename) {
	if (filename == NULL) {
		return 0;
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
	strcpy(data.author, author);
	stat(filename, &data.stat);
	if (strftime(data.datetext, 256, dateformat, localtime(&data.stat.st_mtime)) == 0) {
		fprintf(stderr, "Could not format date correctly! Please run comment --config dateformat \"FORMAT\"\n");
		return 2;
	}

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

	return analyzeAndComment(&data);
}

