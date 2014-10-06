/*
 * $Header: H:/SRC.SSB/util/buffer.c,v 1.2 1992/02/14 19:00:39 ak Exp $
 *
 *	Pipe buffer.
 *
 * $Log: buffer.c,v $
 * Revision 1.2  1992/02/14  19:00:39  ak
 * *** empty log message ***
 *
 * Revision 1.1.1.1  1991/12/12  21:26:33  ak
 * Initial checkin.
 *
 * Revision 1.1  1991/12/12  21:26:29  ak
 * Initial revision
 *
 */

static char *rcsid = "$Id: buffer.c,v 1.2 1992/02/14 19:00:39 ak Exp $";

/*
 * Pipe buffer.
 * Needs large memory model and float.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#define INCL_DOSCALLS
#define INCL_DOSMEMMGR
#define INCL_DOSPROCESS
#define INCL_DOSSIGNALS
#include <os2.h>
#include <dos.h>


#define BUFSIZE		32768U
#define NBUFFERS	(unsigned)(4L * 1024L * 1024L / BUFSIZE)
#define NBUFS20		(unsigned)(2L * 1024L * 1024L / BUFSIZE)

typedef struct Buffer {
	BYTE *	buf;		/* 32KB buffer */
	USHORT	nbytes;		/* number of bytes in buffer */
} Buffer;

Buffer		buffers [NBUFFERS];	/* the buffers */
int		nbuffers = NBUFFERS;	/* allocated buffers */
int		rx = 0,			/* read index */
		wx = 0;			/* write index */
volatile int	nb = 0;			/* number of filled buffers */

int		r_stack [2048];
volatile USHORT	r_tid;
USHORT		w_tid;

USHORT		bufsize = BUFSIZE;	/* buffer size */
USHORT		reblock = 0;		/* reblock size */

int		ilevel;
int		olevel;

void _far
r_thread(void)
{
	Buffer	*bp;
	USHORT	nreq, nr, nbytes;
	short	eof;

	for (eof = 0; !eof; ) {
		if (nb >= ilevel) {
			DosSuspendThread(r_tid);
			continue;
		}

		while (nb < nbuffers && !eof) {
			bp = &buffers[rx];
			nbytes = 0;
			do {
				if (DosRead(0, bp->buf + nbytes, bufsize - nbytes, &nr)) {
					DosSuspendThread(w_tid);
					fprintf(stderr, "buffer: read error\n");
					exit(1);
				}
				nbytes += nr;
			} while (nr && nbytes < bufsize);

			if (nbytes == 0) {
				eof = 1;
				break;
			}

			if (reblock && (nr = nbytes % reblock) != 0) {
				memset(bp->buf + nbytes, 0, reblock - nr);
				nbytes += reblock - nr;
				eof = 1;
			}
			bp->nbytes = nbytes;

			rx = (rx + 1) % nbuffers;
			++nb;
			DosResumeThread(w_tid);
		}
	}
	r_tid = 0;
	DosResumeThread(w_tid);
}

void
w_thread(void)
{
	Buffer	*bp;
	USHORT	nw;

	while (r_tid || nb) {
		if (r_tid && nb < olevel) {
			DosSuspendThread(w_tid);
			continue;
		}
		while (nb) {
			bp = &buffers[wx];
			if (DosWrite(1, bp->buf, bp->nbytes, &nw)) {
				DosSuspendThread(r_tid);
				fprintf(stderr, "buffer: write error\n");
				exit(1);
			}
			if (nw != bp->nbytes) {
				DosSuspendThread(r_tid);
				fprintf(stderr, "buffer: write short\n");
				exit(1);
			}
			wx = (wx + 1) % nbuffers;
			--nb;
			DosResumeThread(r_tid);
		}
	}
	w_tid = 0;
}

void
setFail(int fd)
{
	USHORT state;
	DosQFHandState(fd, &state);
	state &=~OPEN_FLAGS_RANDOMSEQUENTIAL;
	state |= OPEN_FLAGS_SEQUENTIAL | OPEN_FLAGS_NO_CACHE
			| OPEN_FLAGS_FAIL_ON_ERROR;
	DosSetFHandState(fd, state);
}

main(int argc, char **argv)
{
	PFNSIGHANDLER	oldf;
	USHORT		olda;
	int		i;
	ULONG		size;
	double		it, ot;

	setFail(0);
	setFail(1);

	DosSetSigHandler((PFNSIGHANDLER)0, &oldf, &olda, SIGA_KILL, SIG_CTRLC);
	DosSetSigHandler((PFNSIGHANDLER)0, &oldf, &olda, SIGA_KILL, SIG_CTRLBREAK);

#ifdef __ZTC__
	if (_osmajor == 10) {
#else
	if ((_osversion & 0xFF) == 10) {
#endif
		/* OS/2 1.x */
		DosMemAvail(&size);
		size -= 1024L * 1024L;
		nbuffers = size / BUFSIZE;
		if (nbuffers < 2)
			nbuffers = 2;
		else if (nbuffers > NBUFFERS)
			nbuffers = NBUFFERS;
	} else {
		/* OS/2 2.x */
		char *env = getenv("BUFFER");
		nbuffers = env ? strtoul(env, NULL, 0) * 1024L / BUFSIZE
			       : NBUFS20;
	}

	it = ot = 1.0;

	while ((i = getopt(argc, argv, "s:b:i:o:")) != EOF) {
		switch (i) {
		case 's':
			nbuffers = (strtoul(optarg, NULL, 0) * 1024L) / BUFSIZE;
			if (nbuffers < 2)
				goto usage;
			break;
		case 'b':
			reblock = strtoul(optarg, NULL, 0);
			if (reblock < 1 || reblock > BUFSIZE)
				goto usage;
			bufsize -= bufsize % reblock;
			break;
		case 'i':
			it = atof(optarg) / 100.0;
			break;
		case 'o':
			ot = atof(optarg) / 100.0;
			break;
		default:
		usage:	fprintf(stderr, "buffer [options]\n"
					" -b<n>	buffer size in KB (%u..4096), default=%u\n"
					" -p<n>	blocking size (<= %u)\n"
					" -i<n>	read when buffer <n>%% free, default=100\n"
					" -o<n>	write when buffer <n>%% full, default=100\n",
					BUFSIZE / (1024 / 2),
					nbuffers * 32,
					BUFSIZE);
			exit(2);
		}
	}

	ilevel = (1.0 - it) * nbuffers;
	if (ilevel <= 0)
		ilevel = 1;
	else if (ilevel > nbuffers)
		ilevel = nbuffers;

	olevel = ot * nbuffers;
	if (olevel < 1)
		olevel = 1;
	else if (olevel > nbuffers)
		olevel = nbuffers;

	for (i = 0; i < nbuffers; ++i) {
		USHORT sel;
		if (DosAllocSeg(bufsize, &sel, 0)) {
			if (i < 2) {
				fprintf(stderr, "buffer: Not enough memory\n");
				exit(2);
			}
			nbuffers = i;
			break;
		}
		buffers[i].buf = MAKEP(sel, 0);
	}

	w_tid = 1;
	if (DosCreateThread(r_thread, &r_tid, (PBYTE)&r_stack[2048])) {
		fprintf(stderr, "buffer: No thread\n");
		exit(2);
	}

	w_thread();

	exit(0);
}

