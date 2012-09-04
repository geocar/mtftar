#include "mtf.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#ifdef DEBUG
#include <stdio.h>
#endif

#include "util.h"

int mtf_stream_start(struct mtf_stream *s)
{
	s->laststream = 0;
	s->stream = mtfdb_off(s);
#ifdef DEBUG
	fprintf(stderr, "offset: %llu\n", (unsigned long long)s->stream);
	fflush(stderr);
#endif
	s->streamdid = 0;
	if (!mtfscan_ready(s, s->stream + mtf_stream_sizeof)) return 0;
	return 1;
}
int mtf_stream_read(struct mtf_stream *s, unsigned char *buf, int len)
{
	int r, total;
	total = 0;
	while (len > 0) {
		if (s->streamdid + len > mtf_stream_length(s)) {
			len = (int)(mtf_stream_length(s) - s->streamdid);
		}
		if (len < 1) break;
		do {
			r = read(s->fd, buf, len);
		} while (r == -1 && errno == EINTR);
		if (r < 1) return -1;

		len -= r;
		buf += r;
		s->abspos += r;
		s->streamdid += r;
		s->flbread += r;
		total += r;
		while (s->flbread > s->flbsize) {
			s->flbread -= s->flbsize;
		}
	}
	return total;
}
int mtf_stream_eof(struct mtf_stream *s)
{
	return (mtf_stream_length(s) == s->streamdid) ? 1 :0;
}
int mtf_stream_eset(struct mtf_stream *s)
{
	return s->laststream ? 1 : 0;
}

int mtf_stream_copy(struct mtf_stream *s, int outfd)
{
	unsigned char stump[4096];
	int islaststream;
	int len;

#ifdef DEBUG
	fprintf(stderr, "type of this stream: %x\n",
			mtf_stream_type(s));
	fprintf(stderr, "length of this stream: %llu\n",
			(unsigned long long)mtf_stream_length(s));
#endif
	islaststream = (mtf_stream_type(s) == mtfst_spad ? 1 : 0);

	/* okay, skip this blob */
	while (!mtf_stream_eof(s)) {
		len = mtf_stream_read(s, stump, sizeof(stump));
		if (len < 0) return 0;

		if (outfd > -1) {
			if (!bwrite(outfd, stump, len))
				return 0;
		}
	}

	if ((s->laststream |= islaststream)) {
		/* okay, resync with regular data stream */
#ifdef DEBUG
		fprintf(stderr, "SYNCING: %lu\n", s->flbread);
#endif
		return 1;
	}

	/* load the next stream offset */
	s->ready = 0;
	s->header = s->buffer;
	if ((s->flbread % 4) != 0) {
		/* stream offsets are on a boundary */
		s->stream = 4 - (s->flbread % 4);
	} else {
		s->stream = 0;
	}
#ifdef DEBUG
	fprintf(stderr, "FLBREAD: %lu\n", s->flbread);
#endif
	s->streamdid = 0;
	if (!mtfscan_ready(s, s->stream + mtf_stream_sizeof)) return 0;
	return 1;
}
int mtf_stream_next(struct mtf_stream *s)
{
	return mtf_stream_copy(s, -1);
}
