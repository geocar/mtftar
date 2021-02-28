#include "tar.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

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


/* Calculate the md5 hash of full path and filename,
 * for use in headers for POSIX.1-2001 pax format.
 * This will be used as the filename in headers for
 * files where the full path and filename do not fit.
 */
void tarout_md5(struct tar_filename *tf) {
	md5_state_t state;
	md5_byte_t digest[16];
	md5_init(&state);
	md5_append(&state, (const md5_byte_t *)tf->filename, strlen(tf->filename));
	md5_finish(&state, digest);
	int i;
	for (i = 0; i < 16; i++) {
		sprintf(tf->md5 + (i * 2), "%02x", digest[i]);
	}
}

void tarout_process_filename(const char *filename, struct tar_filename *tf, int pax) {
	/* the whole path and filename */
	tf->filename = filename;
	tf->filename_len = strlen(filename);

	/* by default, the prefix and hash are empty */
	tf->pfn = "";
	tf->pfn_len = 0;
	tf->md5[0] = 0;

	if (strlen(filename) <= 100) {
		/* entire path and filename fits in a 100 char field */
		tf->lfn = filename;
		tf->lfn_len = strlen(filename);
		return;
	}

	/* ooh, not good */
	/* take last 100 chars of path & filename, and look
	 * for first slash in that as a breaking point.
	 */
	int i;
	for (i = strlen(filename) - 100; filename[i]; i++) {
		if (filename[i] == '/') break;
	}
	if (filename[i]) {
		/* the filename and possibly part of the path fit
		 * in 100 chars.  okay, THIS will be lfn
		 */
		tf->lfn = filename+i+1;
		tf->lfn_len = strlen(tf->lfn);
	} else {
		/* the filename alone is more than 100 chars.
		 * okay, simply not enough room. take first 100
		 * chars of last path segment
		 */
		tf->lfn_len = 100;
		for (tf->lfn = 0; i >= 0; i--) {
			if (filename[i] == '/') {
				tf->lfn = filename+i+1;
				break;
			}
		}
		if (!tf->lfn) {
			/* forget it.
			 * no slash was found, so this is just one very long filename
			 */
			tf->lfn = filename;
		}
		if (pax) {
			/* part of filename was lost,
			 * but we get to use POSIX pax headers to save it
			 */
			tarout_md5(tf);
		}
	}
	if (tf->lfn > filename) {
		/* there will be a prefix */
		tf->pfn_len = tf->lfn - filename;
		if (tf->pfn_len <= 155) {
			/* entire prefix fits in a 155 char field */
			tf->pfn = filename;
		} else {
			/* oh geez, this is huge.
			 * take last 155 characters of prefix
			 * then chomp up to first slash
			 */
			for (i = 0; tf->pfn_len > 155; i++) {
				if (filename[i] == '/') {
					tf->pfn_len = tf->lfn - (filename+i+1);
				}
			}
			if (tf->pfn_len > 0) {
				tf->pfn = filename+i;
			}
			if (pax && !tf->md5[0]) {
				/* part of path was lost,
				 * but we get to use POSIX pax headers to save it
				 */
				tarout_md5(tf);
			}
		}
	}
}


/* Each pax metadata line includes its own line length! */
int tarout_pax_line_length(int entry_len) {
	/* Start with length of entry plus leading space and trailing newline. */
	int line_len = entry_len + 2;
	/* Add length of line_len to line_len. */
	line_len += (int)log10(line_len) + 1;
	/* Do it again, in case adding length of line_len increased length of line_len. */
	line_len = ((int)log10(line_len) + 1) + entry_len + 2;
	return line_len;
}


int tarout_write_heading(struct tar_stream *t, int type,
		struct tar_filename *tf,
		int mode, int uid, int gid,
		unsigned long long size, time_t mtime,
		const char *linkname, const char *username)
{
	int i;
	unsigned long sum;

	memset(t->block, 0, 512);
	t->tptr = 0;

	t->hold = 1;
	if (!tarout_nullpad(t, tf->lfn, tf->lfn_len, 100)) return 0;
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
	if (!tarout_nullpad(t, tf->pfn, tf->pfn_len, 155)) return 0;

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


int tarout_heading(struct tar_stream *t, int type,
		const char *filename, int mode, int uid, int gid,
		unsigned long long size, time_t mtime,
		const char *linkname, const char *username, int pax)
{
	struct tar_filename tf;

	if (!filename) filename = "";
	if (!linkname) linkname = "";
	if (!username) username = "";

	tarout_process_filename(filename, &tf, pax);
	char pax_filename[101];
	if (pax) {
		int create_pax = 0;
		/* POSIX pax is enabled. */
		if (tf.md5[0]) {
			/* Part of path or filename was lost. */
			create_pax = 1;
			/* We will use <md5>.meta for the metadata filename, and
			 * <md5>.file for the actual file filename.  This will allow
			 * users of non-compatible tar programs to sort things out manually.
			 */
			tf.pfn = "";
			tf.pfn_len = 0;
			tf.lfn = pax_filename;
			tf.lfn_len = snprintf(pax_filename, 101, "%s.meta", tf.md5);
		}
		if (size > 077777777777) {
			create_pax = 1;
		}
		if (create_pax) {
			/* Create POSIX pax metadata file:
			 * <linelen> path=<filename>\n
			 * <linelen> size=<size>\n
			 */
			int path_len = tarout_pax_line_length(strlen(filename) + 5);	/* path=%s */
			int size_len;
			if (size == 0) {
				/* Avoid log10(0) */
				size_len = 9;	/* 9 size=0\n */
			} else {
				size_len = tarout_pax_line_length(((int)log10(size) + 1) + 5);	/* size=%llu */
			}
			int meta_block_len = path_len + size_len + 1;
			/* Round up to next multiple of 512. */
			int remainder = meta_block_len % 512;
			if (remainder != 0) {
				meta_block_len += (512 - remainder);
			}
			char metadata[meta_block_len];
			memset(metadata, 0, meta_block_len);
			snprintf(metadata, meta_block_len, "%d path=%s\n%d size=%llu\n", path_len, filename, size_len, size);
			int meta_len = strlen(metadata);
			/* Write POSIX pax metadata header. */
			tarout_write_heading(t, 'x', &tf, mode, uid, gid, meta_len, mtime, linkname, username);
			/* Write POSIX pax metadata file. */
			bwrite(t->fd, (unsigned char *)metadata, meta_block_len);
			if (tf.md5[0]) {
				/* Now that the POSIX pax metadata is written using <md5>.meta,
				 * set up the filename for the file using <md5>.file.
				 * (Only if filename was too long above.)
				 */
				tf.lfn_len = snprintf(pax_filename, 101, "%s.file", tf.md5);
			}
			/* Now continue on to write the real header for this file or directory... */
		}
	}

	return tarout_write_heading(t, type, &tf, mode, uid, gid,
		size, mtime, linkname, username);
}


int tarout_dir(struct tar_stream *t, const char *filename,
		int mode, int uid, int gid, time_t mtime, int pax)
{
	return tarout_heading(t, '5', filename, mode, uid, gid,
			0, mtime, 0, 0, pax);
}
int tarout_file(struct tar_stream *t, const char *filename,
		int mode, int uid, int gid, time_t mtime,
		unsigned long long size, int pax)
{
	return tarout_heading(t, '0', filename, mode, uid, gid,
			size, mtime, 0, 0, pax);
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
