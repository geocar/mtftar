#include "mtf.h"
#include "tar.h"

#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

static void usage(int e)
{
	fprintf(stderr, "Usage: mtftar [-v] [-p] [-s setno] [patterns...] < backup.btf | tar xvf -\n");
	fprintf(stderr, "       mtftar [-v] [-p] [-s setno] -f backup.btf [patterns...] | tar xvf -\n");
	fprintf(stderr, "       mtftar [-v] [-p] [-s setno] -f backup.btf -o output.tar [patterns...]\n");
	fprintf(stderr, "       mtftar [-v] [-p] [-s setno] -o output.tar [patterns...] < backup.btf\n");
	fprintf(stderr, "       mtftar -l [-v] [-s setno] < backup.btf\n");
	fprintf(stderr, "       mtftar -l [-v] [-s setno] -f backup.btf\n");
	fprintf(stderr, "       mtftar -L [-v] [-s setno] < backup.btf\n");
	fprintf(stderr, "       mtftar -L [-v] [-s setno] -f backup.btf\n");
	exit(e);
}

static int seek_arg(const char *a, const char *b)
{
	while (*b == '/') b++;
	if (*b) return b - a;
	return -1;
}
static int check_1arg(int p, const char *filename, const char *spec)
{
	const char *a;
	a = filename;
	while (*spec) {
		if (*spec == '/') {
			while (*filename == '/') filename++;
			while (*spec == '/') spec++;

			/* end of filename, spec matches */
			if (!*filename) return p ? -1 : 0;
			if (!*spec) return p ? seek_arg(a, filename) : 0;

		} else if (tolower(((unsigned int)*spec))
				== tolower(((unsigned int)*filename))) {
			filename++;
			spec++;
			if (!*filename) {
				if (!p && (*spec == '/' || *spec == '\0'))
					return 0;
				return -1;
			}
		} else {
			return -1;
		}
	}
	if (*filename == '/') return p ? seek_arg(a, filename) : 0;
	if (*filename == '\0' && !p) return 0;
	return -1;
}
static int check_argmatch(int p, const char *filename, int n, int argc, char *argv[])
{
	int i, j;
	if (n == argc) return 0;
	for (i = n; i < argc; i++) {
		j = check_1arg(p, filename, argv[i]);
		if (j > -1) return j;
	}
	return -1;
}

int main(int argc, char *argv[])
{
	struct mtf_stream s;
	struct tar_stream t;
	unsigned int skip_flb;
	int c, fd, setno, listonly, listindex;
	int outfd, verbose;
	int patching;
	off64_t idxpos;


	char *volume = 0;

	char *cwd = 0;
	int cwd_check = 0;

	char *fn, *dfn;
	char *str;

	time_t mtime;
	int r;

	fd = 0;
	setno = 0;
	listonly = listindex = 0;
	verbose = 0;
	outfd = 1;
	patching = 0;
	dfn = 0;
	while ((c = getopt(argc, argv, "pf:s:o:LlvX:h")) != -1) {
		switch (c) {
		case 'p':
			patching = 1;
			break;
		case 'f':
			fd = open(optarg, O_RDONLY|O_NOCTTY
#ifdef O_LARGEFILE
					| O_LARGEFILE
#endif
					);
			if (fd == -1) {
				perror(optarg);
				exit(1);
			}
			break;
		case 'o':
			outfd = open(optarg, O_WRONLY|O_CREAT|O_NOCTTY, 0666);
			if (outfd == -1) {
				perror(optarg);
				exit(1);
			}
			break;
		case 'L':
			listonly = listindex = 1;
			break;
		case 'l':
			listonly = 1;
			break;
		case '?':
			usage(1);
			break;
		case 'h':
			usage(0);
			break;
		case 'v':
			verbose++;
			break;
		case 'X':
			if (sizeof(idxpos) == sizeof(int)) {
				r = sscanf(optarg, "%u,%u,%ld,%*s",
						&skip_flb,
						(int *)(void*)&idxpos, &mtime);
			} else if (sizeof(idxpos) == sizeof(long)) {
				r = sscanf(optarg, "%u,%lu,%ld,%*s",
						&skip_flb,
						(long *)(void*)&idxpos, &mtime);
			} else {
				r = sscanf(optarg, "%u,%llu,%ld,%*s",
						&skip_flb,
						(unsigned long long *)&idxpos, &mtime);
			}
			dfn = strchr(optarg, ',');
			if (dfn) dfn = strchr(dfn+1, ',');
			if (dfn) dfn = strchr(dfn+1, ',');
			if (r != 3 || !dfn) {
				fputs("-X option is invalid (must come from -L liistindex mode)\n", stderr);
				usage(1);
			}
			
			dfn++;
			dfn = (char *)strdup(dfn);
			if (!dfn) {
				perror("strdup");
				exit(1);
			}
			break;
		case 's':
			setno = atoi(optarg);
			if (setno < 1 || setno > 65535) {
				fputs("Set number out of range (1-65535)\n",stderr);
				usage(1);
			}
			break;
		};
	}
	if (fd == 0 && isatty(0)) usage(1);
	mtfscan_init(&s, fd);

	if (!listonly) tarout_init(&t, outfd);

	if (dfn && skip_flb) {
		if (listindex) {
			fputs("-L and -X are mutually exclusive\n", stderr);
			exit(1);
		}
		if (listonly) {
			fputs("-l and -X are mutually exclusive\n", stderr);
			exit(1);
		}
		if (lseek64(fd, idxpos, SEEK_SET) != idxpos) {
			perror("lseek");
			exit(1);
		}
		/* okay, we're pointing back at the beginning of the stream */
		s.flbsize = skip_flb;
		if (!mtfscan_start(&s)) {
			perror("scan-start");
			exit(1);
		}
		while (!mtfdb_file_type(&s)) {
			fprintf(stderr, "[%c%c%c%c] unknown section type=%d\n",
					s.header[0],
					s.header[1],
					s.header[2],
					s.header[3], s.stringtype);
			if (!mtfscan_next(&s) || !mtfscan_start(&s)) {
				perror("scan-ready");
				exit(1);
			}
			
		}

		mtf_stream_start(&s);
		while (mtf_stream_type(&s) != mtfst_stan
				&& mtf_stream_type(&s) != mtfst_spad
				&& !mtf_stream_eset(&s)) {
			mtf_stream_next(&s);
		}

		if (mtf_stream_type(&s) == mtfst_stan) {

			if (((c = check_argmatch(patching, dfn, optind, argc, argv)) > -1)) {
				/* write out TAR header */
				tarout_file(&t, dfn+c, 0666, 0, 0, mtime,
						mtf_stream_length(&s));
				mtf_stream_copy(&s, outfd);
				tarout_tail(&t);
			}
			while (!mtf_stream_eset(&s)) {
				mtf_stream_next(&s);
			}
		} else {
			while (!mtf_stream_eset(&s)) {
				mtf_stream_next(&s);
			}
		}
		exit(0);
	}

	for (;;) {
		if (!mtfscan_start(&s)) {
			/* could be an eof */
			break;
		}
		if (mtfdb_tape_type(&s)) {
			/* have TAPE header/characteristics */
			if (verbose) {
				str = mtfscan_string(&s, mtfdb_tape_software(&s), '/');
				fprintf(stderr, "MFT Generator: %s\n", str);
				free(str);

				str = mtfscan_string(&s, mtfdb_tape_name(&s), '/');
				fprintf(stderr, "Tape Name: %s\n", str);
				free(str);

				str = mtfscan_string(&s, mtfdb_tape_label(&s), '/');
				fprintf(stderr, "Tape Label: %s\n", str);
				free(str);

				fputs("\n", stderr);
			}
		} else if (mtfdb_sfmb_type(&s)) {
			/* soft file markers */
#ifdef DEBUG
			fprintf(stderr, "fm cc: %u\n", mtfdb_sfmb_used(&s));
			fprintf(stderr, "fm tt: %u\n", mtfdb_sfmb_marks(&s));
			fputs("\n", stderr);
#endif
		} else if (mtfdb_sset_type(&s)) {
			/* set begin */
			if (verbose) {
				fprintf(stderr, "-- Set#%d\n", mtfdb_sset_num(&s));

				str = mtfscan_string(&s, mtfdb_sset_name(&s), '/');
				fprintf(stderr, "Set Name: %s\n", str);
				free(str);

				str = mtfscan_string(&s, mtfdb_sset_label(&s), '/');
				fprintf(stderr, "Set Label: %s\n", str);
				free(str);

				str = mtfscan_string(&s, mtfdb_sset_user(&s), '/');
				fprintf(stderr, "Set Owner: %s\n", str);
				free(str);

				fputs("\n", stderr);
			}
		} else if (mtfdb_volb_type(&s)) {
			/* volume block */
			free(volume);
			volume = mtfscan_string(&s, mtfdb_volb_device(&s), '/');

			mtime = mtfdb_volb_ctime(&s);
			if (!listonly && (c = check_argmatch(patching, volume, optind, argc, argv)) > -1) {
				tarout_dir(&t, volume+c, 0777, 0, 0, mtime);
				tarout_tail(&t);
			}
#ifdef DEBUG
			fprintf(stderr, "  dev: %s\n", volume);
			fprintf(stderr, "  vol: %s\n", mtfscan_string(&s, mtfdb_volb_volume(&s), '/'));
			fprintf(stderr, "machi: %s\n", mtfscan_string(&s, mtfdb_volb_machine(&s), '/'));
#endif

			free(cwd);
			cwd = 0;
			cwd_check = 0;

		} else if (mtfdb_dirb_type(&s)) {
			if (mtfdb_dirb_attr(&s) & 0x20000) {
				/* path is encoded inside stream, don't handle */
			} else {
				free(cwd);

				cwd_check = mtfdb_dirb_id(&s);
				cwd = mtfscan_string(&s, mtfdb_dirb_name(&s), '/');
			}
			mtime = mtfdb_dirb_mtime(&s);
			dfn  = malloc(strlen(volume) + strlen(cwd) + 2);
			if (!dfn) {
				perror("malloc");
				exit(1);
			}
			sprintf(dfn, "%s/%s", volume, cwd);
			if (!listonly && (c = check_argmatch(patching, dfn, optind, argc, argv)) > -1) {
				tarout_dir(&t, dfn+c, 0777, 0, 0, mtime);
				tarout_tail(&t);
			}
			free(dfn);

		} else if (mtfdb_file_type(&s)) {
			/* file chunk */
			idxpos = (s.abspos - s.ready);
			fn = mtfscan_string(&s, mtfdb_file_name(&s), '/');
			c = mtfdb_file_dirid(&s);
			if (cwd_check != c) {
				fprintf(stderr, "MTF out of order (files before directory saw %d while expecting limit of %d)\n",
						c, cwd_check);
			}
			dfn  = malloc(strlen(volume) + strlen(fn) + strlen(cwd) + 3);
			if (!dfn) {
				perror("malloc");
				exit(1);
			}
			sprintf(dfn, "%s/%s/%s", volume, cwd, fn);

			mtime = mtfdb_file_mtime(&s);

			if (listindex) {
				/* this is kludgy at-best */
				printf("%u,", s.flbsize);
				if (sizeof(idxpos) == sizeof(int)) {
					printf("%u,%ld,%s\n", (int)idxpos, mtime, dfn);
				} else if (sizeof(idxpos) == sizeof(long)) {
					printf("%lu,%ld,%s\n", (long)idxpos, mtime, dfn);
				} else {
					printf("%llu,%ld,%s\n", (unsigned long long)idxpos, mtime, dfn);
				}

			} else {
				if (verbose > listonly) {
					fprintf(stderr, "File %s\n", dfn);
				}
				if (listonly) {
					puts(dfn);
				}
			}

			mtf_stream_start(&s);
			while (mtf_stream_type(&s) != mtfst_stan
					&& mtf_stream_type(&s) != mtfst_spad
					&& !mtf_stream_eset(&s)) {
				mtf_stream_next(&s);
			}

			if (mtf_stream_type(&s) == mtfst_stan) {

				if (!listonly && ((c = check_argmatch(patching, dfn, optind, argc, argv)) > -1)) {
					/* write out TAR header */
					tarout_file(&t, dfn+c, 0666, 0, 0, mtime,
							mtf_stream_length(&s));
					mtf_stream_copy(&s, outfd);
					tarout_tail(&t);
				}
				while (!mtf_stream_eset(&s)) {
					mtf_stream_next(&s);
				}
			} else {
				while (!mtf_stream_eset(&s)) {
					mtf_stream_next(&s);
				}
			}
#ifdef DEBUG
			fprintf(stderr, "STRAY STREAM\n");
#endif
			free(dfn);

		} else if (mtfdb_eset_type(&s)) {
			if (setno) {
				if (setno == mtfdb_eset_seq(&s)) {
					/* we're all done */
					break;
				}
			}

		} else if (mtfdb_dead_type(&s)) {
			/* do nothing */

		} else if (isalpha(s.header[0])
				&& isalpha(s.header[1])
				&& isalpha(s.header[2])
				&& isalpha(s.header[3])) {
#ifdef DEBUG
			fprintf(stderr, "[%c%c%c%c] unknown section type=%d\n",
					s.header[0],
					s.header[1],
					s.header[2],
					s.header[3], s.stringtype);
#endif
		} else {
#ifdef DEBUG
			fprintf(stderr, "[%c%c%c%c] (%02x%02x%02x%02x) empty section(?) type=%d\n",
					s.header[0],
					s.header[1],
					s.header[2],
					s.header[3],
					s.header[0],
					s.header[1],
					s.header[2],
					s.header[3],
					s.stringtype);
			break;
#endif
		}
		fflush(stderr);
		if (!mtfscan_next(&s)) {
			perror("scan-next");
			exit(1);
		}
	}

	exit(0);

}
