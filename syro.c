#include <u.h>
#include <libc.h>
#include <bio.h>
#include "korg_syro_volcasample.h"

int
parsepath(char *spec, char **path, int *id)
{
	int n;
	char *p, *q;

	if((p = strchr(spec, ':')) != nil && p != spec){
		if((n = strtol(spec, &q, 10)) == 0 || q != p++)
			goto error;
		if(n < 0 || n >= VOLCASAMPLE_NUM_OF_SAMPLE)
			goto error;
		*id = n;
	}else
		p = spec;
	if(*p == 0)
		goto error;
	*path = p;
	return 0;
error:
	werrstr("invalid format");
	return -1;
}

void
usage(void)
{
	fprint(2, "usage: %s [-e] [ID:]FILE [[ID:]FILE..]\n", argv0);
	exits("usage");
}

/* audio/wavdec < in.wav > in.pcm
 * audio/syro *.pcm >/dev/audio → sync in
 */
void
threadmain(int argc, char **argv)
{
	int pfd[2], type, id;
	char *path;
	uchar s[4], *buf;
	s16int l, r;
	u32int nf;
	vlong n, off, nbuf;
	SyroHandle sh;
	SyroData dat[VOLCASAMPLE_NUM_OF_SAMPLE], *dp;
	Biobuf *bf;

	type = DataType_Sample_Compress;
	ARGBEGIN{
	case 'e': type = DataType_Sample_Erase; break;
	case 'l': type = DataType_Sample_Liner; break;	/* uncompressed? */
	default: usage();
	}ARGEND
	if(*argv == nil)
		usage();
	for(dp=dat; *argv!=nil && dp<dat+nelem(dat); dp++, argv++){
		id = dp - dat;
		if(parsepath(*argv, &path, &id) < 0)
			sysfatal("parsepath: %r");
		if(pipe(pfd) < 0)
			sysfatal("pipe: %r");
		if(rfork(RFPROC|RFFDG) == 0){
			close(pfd[1]);
			close(0);
			if(open(path, OREAD) < 0)
				sysfatal("open: %r");
			dup(pfd[0], 1);
			close(pfd[0]);
			execl("/bin/audio/pcmconv", "pcmconv", "-i", "r44100c2s16",
				"-o", "r31250c1s16", nil);
			sysfatal("execl: %r");
		}
		close(pfd[0]);
		off = 0;
		nbuf = IOUNIT * 4;
		if((buf = malloc(nbuf)) == nil)
			sysfatal("malloc: %r");
		while((n = readn(pfd[1], buf+off, nbuf-off)) > 0){
			off += n;
			if((buf = realloc(buf, nbuf*2)) == nil)
				sysfatal("realloc: %r");
			nbuf *= 2;
		}
		if(n < 0)
			sysfatal("read: %r");
		close(pfd[1]);
		if((buf = realloc(buf, off)) == nil)
			sysfatal("realloc: %r");
		dp->Size = off;
		dp->pData = buf;
		dp->DataType = type;
		dp->Number = id;
		dp->Quality = 16;
		dp->Fs = 31250;
		dp->SampleEndian = LittleEndian;
		fprint(2, "[%zd] %s size %lld → sample %d\n", dp-dat, path, off, id);
	}
	if((n = SyroVolcaSample_Start(&sh, dat, dp - dat, 0, &nf)) != Status_Success)
		sysfatal("SyroVolcaSample_Start %lld", n);
	fprint(2, "%ud total samples, %ud total size\n", nf, nf*4);
	if((bf = Bfdopen(1, OWRITE)) == nil)
		sysfatal("Bfdopen: %r");
	while(nf-- > 0){
		SyroVolcaSample_GetSample(sh, &l, &r);
		s[0] = l;
		s[1] = l >> 8;
		s[2] = r;
		s[3] = r >> 8;
		Bwrite(bf, s, sizeof s);
	}
	Bterm(bf);
	SyroVolcaSample_End(sh);
	exits(nil);
}
