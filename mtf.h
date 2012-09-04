#ifndef __mtf_h
#define __mtf_h

/* for off_t */
#include <features.h>
#include <sys/types.h>
#include <unistd.h>

/* for time_t */
#include <time.h>

struct mtf_tape_pos {
	unsigned int size;
	unsigned int pos;
};
struct mtf_stream {
	unsigned char buffer[65536];
	unsigned char *header;
	unsigned int blksize;
	unsigned int flbread;
	unsigned int flbsize;
	unsigned int stream;
	off64_t streamdid;
	int laststream;

	int fd;
	int stringtype;
	unsigned int ready;

	off64_t abspos;
};

/* for memcmp() */
#include <string.h>

/* the common descriptor block is 48 octets long */
#define mtfdb_type(x)		((x)->header)
#define mtfdb_attr(x)		mtf_header_uint32(x, 4)
#define mtfdb_off(x)		mtf_header_uint16(x, 8)
#define mtfdb_osid(x)		mtf_header_uint8(x, 10)
#define mtfdb_osver(x)		mtf_header_uint8(x, 11)
#define mtfdb_size(x)		mtf_header_offset(x, 12)
#define mtfdb_fla(x)		mtf_header_offset(x, 20)
#define mtfdb_cbid(x)		mtf_header_uint32(x, 36)
#define mtfdb_osdata(x)		mtf_header_tapepos(x, 40)
#define mtfdb_strtype(x)	mtf_header_uint8(x, 44)
#define mtfdb_checksum(x)	mtf_header_uint16(x, 46)
/* for a tape-descriptor "TAPE" */
#define mtfdb_tape_type(x)	(!memcmp(mtfdb_type(x), "TAPE", 4))
#define mtfdb_tape_mfmid(x)	mtf_header_uint32(x, 48)
#define mtfdb_tape_attr(x)	mtf_header_uint32(x, 54)
#define mtfdb_tape_seq(x)	mtf_header_uint16(x, 58)
#define mtfdb_tape_encrypt(x)	mtf_header_uint16(x, 60)
#define mtfdb_tape_sfmsize(x)	mtf_header_uint16(x, 62)
#define mtfdb_tape_cattype(x)	mtf_header_uint16(x, 64)
#define mtfdb_tape_name(x)	mtf_header_tapepos(x, 68)
#define mtfdb_tape_label(x)	mtf_header_tapepos(x, 72)
#define mtfdb_tape_passwd(x)	mtf_header_tapepos(x, 76)
#define mtfdb_tape_software(x)	mtf_header_tapepos(x, 80)
#define mtfdb_tape_flbsize(x)	mtf_header_uint16(x, 84)
#define mtfdb_tape_vendorid(x)	mtf_header_uint16(x, 86)
#define mtfdb_tape_ctime(x)	mtf_header_datetime(x, 88)
#define mtfdb_tape_version(x)	mtf_header_uint8(x, 93)
#define mtfdb_tape_sizeof	94
/* for a start-of-data-set "SSET" */
#define mtfdb_sset_type(x)	(!memcmp(mtfdb_type(x), "SSET", 4))
#define mtfdb_sset_attr(x)	mtf_header_uint32(x, 52)
#define mtfdb_sset_encrypt(x)	mtf_header_uint16(x, 56)
#define mtfdb_sset_compress(x)	mtf_header_uint16(x, 58)
#define mtfdb_sset_vendor(x)	mtf_header_uint16(x, 60)
#define mtfdb_sset_num(x)	mtf_header_uint16(x, 62)
#define mtfdb_sset_name(x)	mtf_header_tapepos(x, 64)
#define mtfdb_sset_label(x)	mtf_header_tapepos(x, 68)
#define mtfdb_sset_passwd(x)	mtf_header_tapepos(x, 72)
#define mtfdb_sset_user(x)	mtf_header_tapepos(x, 76)
#define mtfdb_sset_pba(x)	mtf_header_offset(x, 80)
#define mtfdb_sset_ctime(x)	mtf_header_datetime(x, 88)
#define mtfdb_sset_major(x)	mtf_header_uint8(x, 93)
#define mtfdb_sset_minor(x)	mtf_header_uint8(x, 94)
#define mtfdb_sset_tz(x)	mtf_header_int8(x, 95)
#define mtfdb_sset_ver(x)	mtf_header_uint8(x, 96)
#define mtfdb_sset_catver(x)	mtf_header_uint8(x, 97)
#define mtfdb_sset_sizeof	98
/* for a volume "VOLB" */
#define mtfdb_volb_type(x)	(!memcmp(mtfdb_type(x), "VOLB", 4))
#define mtfdb_volb_attr(x)	mtf_header_uint32(x, 52)
#define mtfdb_volb_device(x)	mtf_header_tapepos(x, 56)
#define mtfdb_volb_volume(x)	mtf_header_tapepos(x, 60)
#define mtfdb_volb_machine(x)	mtf_header_tapepos(x, 64)
#define mtfdb_volb_ctime(x)	mtf_header_datetime(x, 68)
#define mtfdb_volb_sizeof	71
/* for a directory block "DIRB" */
#define mtfdb_dirb_type(x)	(!memcmp(mtfdb_type(x), "DIRB", 4))
#define mtfdb_dirb_attr(x)	mtf_header_uint32(x, 52)
#define mtfdb_dirb_mtime(x)	mtf_header_datetime(x, 56)
#define mtfdb_dirb_ctime(x)	mtf_header_datetime(x, 61)
#define mtfdb_dirb_btime(x)	mtf_header_datetime(x, 66)
#define mtfdb_dirb_atime(x)	mtf_header_datetime(x, 71)
#define mtfdb_dirb_id(x)	mtf_header_uint32(x, 76)
#define mtfdb_dirb_name(x)	mtf_header_tapepos(x, 80)
#define mtfdb_dirb_sizeof	84
/* for a file block "FILE" */
#define mtfdb_file_type(x)	(!memcmp(mtfdb_type(x), "FILE", 4))
#define mtfdb_file_attr(x)	mtf_header_uint32(x, 52)
#define mtfdb_file_mtime(x)	mtf_header_datetime(x, 56)
#define mtfdb_file_ctime(x)	mtf_header_datetime(x, 61)
#define mtfdb_file_btime(x)	mtf_header_datetime(x, 66)
#define mtfdb_file_atime(x)	mtf_header_datetime(x, 71)
#define mtfdb_file_dirid(x)	mtf_header_uint32(x, 76)
#define mtfdb_file_id(x)	mtf_header_uint32(x, 80)
#define mtfdb_file_name(x)	mtf_header_tapepos(x, 84)
#define mtfdb_file_sizeof	88
/* for a corrupt object block "CFIL" */
#define mtfdb_cfil_type(x)	(!memcmp(mtfdb_type(x), "CFIL", 4))
#define mtfdb_cfil_attr(x)	mtf_header_uint32(x, 52)
#define mtfdb_cfil_offset(x)	mtf_header_offset(x, 56)
#define mtfdb_cfil_num(x)	mtf_header_offset(x, 64)
#define mtfdb_cfil_sizeof	70
/* for a end-of-set padding block "ESPB" */
#define mtfdb_espb_type(x)	(!memcmp(mtfdb_type(x), "ESPB", 4))
#define mtfdb_espb_sizeof	48
/* for a end-of-data-set block "ESET" */
#define mtfdb_eset_type(x)	(!memcmp(mtfdb_type(x), "ESET", 4))
#define mtfdb_eset_attr(x)	mtf_header_uint32(x, 52)
#define mtfdb_eset_corrupt(x)	mtf_header_uint32(x, 56)
#define mtfdb_eset_seq(x)	mtf_header_uint16(x, 76)
#define mtfdb_eset_set(x)	mtf_header_uint16(x, 78)
#define mtfdb_eset_ctime(x)	mtf_header_datetime(x, 80)
#define mtfdb_eset_sizeof	85
/* for a end-of-tape block "EOTM" */
#define mtfdb_eotm_type(x)	(!memcmp(mtfdb_type(x), "EOTM", 4))
#define mtfdb_eotm_lasteset(x)	mtf_header_offset(x, 52)
#define mtfdb_eotm_sizeof	60
/* for a soft filemark "SFMB" */
#define mtfdb_sfmb_type(x)	(!memcmp(mtfdb_type(x), "SFMB", 4))
#define mtfdb_sfmb_marks(x)	mtf_header_uint32(x, 52)
#define mtfdb_sfmb_used(x)	mtf_header_uint32(x, 56)
#define mtfdb_sfmb_sizeof	60
/* for an empty/spacer block */
#define mtfdb_dead_type(x)	(!memcmp(mtfdb_type(x), "\0\0\0\0", 4))

/* streams */
int mtf_stream_start(struct mtf_stream *s);
#define mtf_stream_type(x)	mtf_header_uint32(x, (x)->stream)
#define mtf_stream_sysattr(x)	mtf_header_uint16(x, (x)->stream+4)
#define mtf_stream_mediaattr(x)	mtf_header_uint16(x, (x)->stream+6)
#define mtf_stream_length(x)	mtf_header_offset(x, (x)->stream+8)
#define mtf_stream_encrypt(x)	mtf_header_uint16(x, (x)->stream+16)
#define mtf_stream_compress(x)	mtf_header_uint16(x, (x)->stream+18)
#define mtf_stream_checksum(x)	mtf_header_uint16(x, (x)->stream+20)
#define mtf_stream_sizeof	22

int mtf_stream_copy(struct mtf_stream *s, int outfd);
int mtf_stream_next(struct mtf_stream *s);
int mtf_stream_eset(struct mtf_stream *s);
int mtf_stream_eof(struct mtf_stream *s);
int mtf_stream_read(struct mtf_stream *s, unsigned char *buf, int len);


/* platform-independant stream data types */
#define mtfst_stan	0x4E415453	/* standard */
#define mtfst_pnam	0x4D414E50	/* path */
#define mtfst_fnam	0x4D414E46	/* file name */
#define mtfst_csum	0x4D555343	/* checksum */
#define mtfst_crpt	0x54505243	/* corrupt */
#define mtfst_spad	0x44415053	/* pad */
#define mtfst_spar	0x52415053	/* sparse */
#define mtfst_tsmp	0x504D5354	/* set map, media based catalog, type 1 */
#define mtfst_tfdd	0x44444654	/* fdd, media based catalog, type 1 */
#define mtfst_map2	0x3250414D	/* set map, media based catalog, type 2 */
#define mtfst_fdd2	0x32444446	/* fdd, media based catalog, type 2 */

/* NT/2K/XP stream data types */
#define mtfst_adat 0x54414441
#define mtfst_ntea 0x4145544E
#define mtfst_nacl 0x4C43414E
#define mtfst_nted 0x4445544E
#define mtfst_ntqu 0x5551544E
#define mtfst_ntpr 0x5250544E
#define mtfst_ntoi 0x494F544E

/* Windows 95/98/ME stream data types */
#define mtfst_gerc 0x43524547

/* Netware stream data types */
#define mtfst_n386 0x3638334E
#define mtfst_nbnd 0x444E424E
#define mtfst_smsd 0x44534D53

/* OS/2 stream data types */
#define mtfst_oacl 0x4C43414F

/* Macintosh stream data types */
#define mtfst_mrsc 0x4353524D
#define mtfst_mprv 0x5652504D
#define mtfst_minf 0x464E494D


/* header parsers */
time_t mtf_header_datetime(struct mtf_stream *s, int pos);
unsigned long long mtf_header_offset(struct mtf_stream *s, int pos);
struct mtf_tape_pos mtf_header_tapepos(struct mtf_stream *s, int pos);
unsigned int mtf_header_uint32(struct mtf_stream *s, int pos);
unsigned int mtf_header_uint16(struct mtf_stream *s, int pos);
unsigned int mtf_header_uint8(struct mtf_stream *s, int pos);
int mtf_header_int32(struct mtf_stream *s, int pos);
int mtf_header_int16(struct mtf_stream *s, int pos);
int mtf_header_int8(struct mtf_stream *s, int pos);

/* scanner */
int mtfscan_init(struct mtf_stream *s, int fd);
int mtfscan_ready(struct mtf_stream *s, unsigned int need);
int mtfscan_readyplus(struct mtf_stream *s, unsigned int more);
int mtfscan_read(struct mtf_stream *s, unsigned char *buf, int chunk);
int mtfscan_skip(struct mtf_stream *s);
int mtfscan_start(struct mtf_stream *s);
int mtfscan_next(struct mtf_stream *s);
unsigned char *mtfscan_string(struct mtf_stream *s, struct mtf_tape_pos q, int sz);

#endif
