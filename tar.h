#ifndef __tar_h
#define __tar_h

#include <time.h>

struct tar_stream {
	unsigned char block[512];
	unsigned int cksum;
	unsigned int needpad;
	int tptr;
	int hold;

	int fd;
};

/* tar outputing code */
int tarout_addch(struct tar_stream *t, int ch);
int tarout_nullpad(struct tar_stream *t, const char *in, int inlen, int pad);
int tarout_octal(struct tar_stream *t, unsigned long long o, int digits, int size);

int tarout_heading(struct tar_stream *t, int type,
		const char *filename, int mode, int uid, int gid,
		unsigned long long size, time_t mtime,
		const char *linkname, const char *username);

int tarout_init(struct tar_stream *t, int fd);
int tarout_dir(struct tar_stream *t, const char *filename,
		int mode, int uid, int gid, time_t mtime);
int tarout_file(struct tar_stream *t, const char *filename,
		int mode, int uid, int gid, time_t mtime,
		unsigned long long size);
int tarout_tail(struct tar_stream *t);

#endif
