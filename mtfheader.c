#include "mtf.h"

#include <stdio.h>

/* MTF is completely little-endian */

time_t mtf_header_datetime(struct mtf_stream *s, int pos)
{
	struct tm t;
	unsigned char x[5];

	if (!mtfscan_ready(s, pos+6)) return 0;

	memcpy(x, ((unsigned char *)&s->header[pos]), 5);
	memset(&t, 0, sizeof(t));
	t.tm_sec = x[4] & 0x3F;
	t.tm_min = (((x[3] & 15) << 2) | ((x[4] & 0xC0) >> 6)) & 0xFF;
	t.tm_hour = (((x[2] & 0x01) << 4) | ((x[3] & 0xF0) >> 4)) & 0xFF;
	t.tm_mday = (((x[2] & 0x3E) >> 1) & 0xFF);
	t.tm_mon = ((((x[1] & 0x03) << 2) | ((x[2] & 0xC0) >> 6)) & 0xFF) - 1;
	t.tm_year = (((x[0] << 6) | (x[1] >> 2)) & 0xFFFF) - 1900;
	return mktime(&t);
}

struct mtf_tape_pos mtf_header_tapepos(struct mtf_stream *s, int pos)
{
	struct mtf_tape_pos q;
	q.size = mtf_header_uint16(s, pos);
	q.pos = mtf_header_uint16(s, pos+2);
	return q;
}
unsigned long long mtf_header_offset(struct mtf_stream *s, int pos)
{
	unsigned int u2[2];

	u2[0] = mtf_header_uint32(s, pos);
	u2[1] = mtf_header_uint32(s, pos+4);

	return (unsigned long long)(((unsigned long long)u2[0])
		|(((unsigned long long)u2[1]) << 32));
}

unsigned int mtf_header_uint8(struct mtf_stream *s, int pos)
{
	unsigned char *p;
	unsigned int res;
	if (!mtfscan_ready(s, pos+1)) return 0;
	p = ((unsigned char *)&s->header[pos]);
	res = *p++;
	return res;
}
unsigned int mtf_header_uint16(struct mtf_stream *s, int pos)
{
	unsigned char *p;
	unsigned int res;
	if (!mtfscan_ready(s, pos+3)) return 0;
	p = ((unsigned char *)&s->header[pos]);
	res = *p++;
	res |= (*p++ << 8);
	return res;
}
unsigned int mtf_header_uint32(struct mtf_stream *s, int pos)
{
	unsigned char *p;
	unsigned int res;
	if (!mtfscan_ready(s, pos+5)) return 0;
	p = ((unsigned char *)&s->header[pos]);
	res = *p++;
	res |= (*p++ << 8);
	res |= (*p++ << 16);
	res |= (*p++ << 24);
	return res;
}

int mtf_header_int8(struct mtf_stream *s, int pos)
{
	unsigned char *p;
	int res;
	if (!mtfscan_ready(s, pos+1)) return 0;
	p = ((unsigned char *)&s->header[pos]);
	res = *p++;
	return res;
}
int mtf_header_int16(struct mtf_stream *s, int pos)
{
	unsigned char *p;
	int res;
	if (!mtfscan_ready(s, pos+3)) return 0;
	p = ((unsigned char *)&s->header[pos]);
	res = *p++;
	res |= (*p++ << 8);
	return res;
}
int mtf_header_int32(struct mtf_stream *s, int pos)
{
	unsigned char *p;
	int res;
	if (!mtfscan_ready(s, pos+5)) return 0;
	p = ((unsigned char *)&s->header[pos]);
	res = *p++;
	res |= (*p++ << 8);
	res |= (*p++ << 16);
	res |= (*p++ << 24);
	return res;
}
