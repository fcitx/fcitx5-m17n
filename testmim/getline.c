#include "getline.h"
#include <stdio.h>

#ifdef WITH_READLINE
#include <stdlib.h>
#include <readline/readline.h>
CFUNC char *mygetline(const char *prompt) {
	static char *line = NULL;
	if (line) {
		free(line);
	}
	return line = readline(prompt);
}
#else
#include <string.h>
enum { NLINE = 1024 };
CFUNC char *mygetline(const char *prompt) {
	static char line[NLINE];
	printf("%s", prompt);
	if (fgets(line, NLINE, stdin)) {
		int n = strlen(line);
		if (line[n-1] == '\n') {
			line[n-1] = '\0';
		}
		return line;
	} else {
		return NULL;
	}
}
#endif
