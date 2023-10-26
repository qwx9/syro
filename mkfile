</$objtype/mkfile
BIN=/$objtype/bin/audio
TARG=syroenc

OFILES=\
	korg_syro_comp.$O\
	korg_syro_func.$O\
	korg_syro_volcasample.$O\
	volcasample_pattern.$O\
	syro.$O\

HFILES=\
	korg_syro_comp.h\
	korg_syro_func.h\
	korg_syro_type.h\
	korg_syro_volcasample.h\
	volcasample_pattern.h\

</sys/src/cmd/mkone

CFLAGS=$CFLAGS -p -D__plan9__ -D__${objtype}__ -I/sys/include/npe \
	 -D_POSIX_SOURCE
