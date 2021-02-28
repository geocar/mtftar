#ifndef __tar_h
#define __tar_h

#include <time.h>
#include "md5/md5.h"

/*
 * Documentation for tar file format, and POSIX.1-2001 pax header extensions:
 * https://en.wikipedia.org/wiki/Tar_(computing)
 * https://www.manpagez.com/man/5/tar/
 * https://www.systutorials.com/docs/linux/man/5-star/
 * https://pubs.opengroup.org/onlinepubs/9699919799/utilities/pax.html
 */
struct tar_stream {
	unsigned char block[512];
	unsigned int cksum;
	unsigned int needpad;
	int tptr;
	int hold;

	int fd;
};

struct tar_filename {
	const char *filename;
	unsigned int filename_len;
	const char *lfn;
	unsigned int lfn_len;
	const char *pfn;
	unsigned int pfn_len;
	char md5[33];
};

/* tar outputing code */
int tarout_addch(struct tar_stream *t, int ch);
int tarout_nullpad(struct tar_stream *t, const char *in, int inlen, int pad);
int tarout_octal(struct tar_stream *t, unsigned long long o, int digits, int size);

int tarout_heading(struct tar_stream *t, int type,
		const char *filename, int mode, int uid, int gid,
		unsigned long long size, time_t mtime,
		const char *linkname, const char *username, int pax);

int tarout_init(struct tar_stream *t, int fd);
int tarout_dir(struct tar_stream *t, const char *filename,
		int mode, int uid, int gid, time_t mtime, int pax);
int tarout_file(struct tar_stream *t, const char *filename,
		int mode, int uid, int gid, time_t mtime,
		unsigned long long size, int pax);
int tarout_tail(struct tar_stream *t);

#endif
