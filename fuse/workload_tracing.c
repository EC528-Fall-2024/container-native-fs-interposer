
#define FUSE_USE_VERSION 31

#define _GNU_SOURCE

#include <fuse.h>
#include <string.h>
#include <stdio.h>
#include "passthrough/passthrough.h"

static int read(const char *path, char *buf, size_t size, off_t offset,
            	struct fuse_file_info *fi) {
	FILE *file = fopen("output.txt", "w");
	fprintf(file, "HERE!");
	return xmp_read(path, buf, size, offset, fi);
}


// Workload tracing
int main(int argc, char *argv[])
{
	struct fuse_operations tracing_file_op = xmp_oper;
	//memcpy(&xmp_oper, &tracing_file_op, sizeof xmp_oper);
	tracing_file_op.read = read;


	enum { MAX_ARGS = 10 };
	int i,new_argc;
	char *new_argv[MAX_ARGS];

	umask(0);
			/* Process the "--plus" option apart */
	for (i=0, new_argc=0; (i<argc) && (new_argc<MAX_ARGS); i++) {
		if (!strcmp(argv[i], "--plus")) {
			fill_dir_plus = FUSE_FILL_DIR_PLUS;
		} else {
			new_argv[new_argc++] = argv[i];
		}
	}
	return fuse_main(new_argc, new_argv, &tracing_file_op, NULL);
}
