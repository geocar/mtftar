#include "util.h"

#include <errno.h>
#include <unistd.h>

int bwrite(int fd, unsigned char *buf, int len)
{
	int r;
	while (len > 0) {
		do {
			r = write(fd, buf, len);
		} while (r == -1 && errno == EINTR);
		if (r < 1) return 0;
		buf += r;
		len -= r;
	}
	return 1;
}
