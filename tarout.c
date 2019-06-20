#include "tar.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

int tarout_init(struct tar_stream *t, int fd)
{
	t->cksum = 0;
	t->needpad = 0;
	t->tptr = 0;
	t->hold = 0;
	t->fd = fd;
	return 1;
}

int tarout_addch(struct tar_stream *t, int ch)
{
	int r;
	t->block[t->tptr++] = ch;
	if (!t->hold && t->tptr == 512) {
		r = bwrite(t->fd, t->block, 512);
		t->tptr = 0;
		return r;
	}
	return 1;
}

int tarout_nullpad(struct tar_stream *t, const char *in, int inlen, int pad)
{
	while (inlen > 0 && pad > 0) {
		if (!tarout_addch(t, *in)) return 0;
		in++;
		inlen--;
		pad--;
	}
	while (pad > 0) {
		if (!tarout_addch(t, 0)) return 0;
		pad--;
	}
	return 1;
}
int tarout_octal(struct tar_stream *t, unsigned long long o, int digits, int size)
{
	char mask[32], data[64];
	sprintf(mask, "%%%dllo ", digits);
	return tarout_nullpad(t, data,
			sprintf(data, mask, (unsigned long long)o), size);
}

int tarout_heading(struct tar_stream *t, int type,
		const char *filename, int mode, int uid, int gid,
		unsigned long long size, time_t mtime,
		const char *linkname, const char *username)
{
	int i;
	unsigned long sum;
	const char *lfn;
	const char *pfn;
	unsigned int lfn_len;
	unsigned int pfn_len;

	memset(t->block, 0, 512);
	t->tptr = 0;

	if (!filename) filename = "";
	if (!linkname) linkname = "";
	if (!username) username = "";

	pfn = "";
	pfn_len = 0;

	if (strlen(filename) > 100) {
		/* ooh, not good. */
		for (i = strlen(filename) - 100; filename[i]; i++) {
			if (filename[i] == '/') break;
		}
		if (filename[i]) {
			/* okay, THIS will be lfn */
			lfn = filename+i+1;
			lfn_len = strlen(lfn);
		} else {
			/* okay, simply not enough room. take first 100
			 * chars of last path segment
			 */
			lfn_len = 100;
			for (lfn = 0; i >= 0; i--) {
				if (filename[i] == '/') {
					lfn = filename+i+1;
					break;
				}
			}
			if (!lfn) {
				/* forget it */
				lfn = filename;
			}
		}
		if (lfn > filename) {
			i = lfn - filename;
			if (i > 155) {
				/* oh geez, this is huge.
				 * take last 155 characters of path
				 * then chomp up to slash
				 */
				pfn_len = i;
				for (i = 0; pfn_len > 155; i++) {
					if (filename[i] == '/') {
						pfn_len = lfn - (filename + i + 1);
					}
				}
				if (pfn_len > 0) {
					pfn = filename+i;
				}
			} else {
				pfn = filename;
				pfn_len = i;
			}
		}
	} else {
		lfn = filename;
		lfn_len = strlen(filename);
	}

	t->hold = 1;
	if (!tarout_nullpad(t, lfn, lfn_len, 100)) return 0;
	if (!tarout_octal(t, (unsigned long long)mode, 6, 8)) return 0;
	if (!tarout_octal(t, (unsigned long long)uid, 6, 8)) return 0;
	if (!tarout_octal(t, (unsigned long long)gid, 6, 8)) return 0;
	if (!tarout_octal(t, (unsigned long long)size, 11, 12)) return 0;
	if (!tarout_octal(t, (unsigned long long)mtime, 11, 12)) return 0;
	if (!tarout_nullpad(t, "        ", 8, 8)) return 0; /* checksum */
	if (!tarout_addch(t, type)) return 0;
	if (!tarout_nullpad(t, linkname, strlen(linkname), 100)) return 0;
	if (!tarout_nullpad(t, "ustar", 5, 6)) return 0; /* ustar\0 */
	if (!tarout_nullpad(t, "00", 2, 2)) return 0;
	if (!tarout_nullpad(t, username, strlen(username), 32)) return 0;
	if (!tarout_nullpad(t, "", 0, 32)) return 0;
	if (!tarout_nullpad(t, "0", 1, 8)) return 0;
	if (!tarout_nullpad(t, "0", 1, 8)) return 0;
	if (!tarout_nullpad(t, pfn, pfn_len, 155)) return 0;

	/* calculate checksum (skipping 8byte chunk) */
	for (i = 0, sum = 0; i < 512; i++) {
		sum += t->block[i];
	}

	t->tptr = 148; /* checksum location */
	if (!tarout_octal(t, (unsigned long long)sum, 6, 8)) return 0;

	/* write out block */
	if (!bwrite(t->fd, t->block, 512)) return 0;

	/* adjust pointer in case someone else wants to use it */
	t->tptr = 0;
	t->hold = 0;

	/* set up padding requirements for tail */
	t->needpad = 512 - (size % 512);
	if (t->needpad == 512) t->needpad = 0;

	return 1;
}


int tarout_dir(struct tar_stream *t, const char *filename,
		int mode, int uid, int gid, time_t mtime)
{
	return tarout_heading(t, '5', filename, mode, uid, gid,
			0, mtime, 0, 0);
}
int tarout_file(struct tar_stream *t, const char *filename,
		int mode, int uid, int gid, time_t mtime,
		unsigned long long size)
{
	return tarout_heading(t, '0', filename, mode, uid, gid,
			size, mtime, 0, 0);
}

int tarout_tail(struct tar_stream *t)
{
	int r;
	if (t->needpad) {
		memset(t->block, 0, t->needpad);
		r = bwrite(t->fd, t->block, t->needpad);
		t->needpad = 0;
		return r;
	}
	return 1;
}
