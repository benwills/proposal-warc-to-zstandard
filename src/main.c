#include "main.h"

// not usually a good idea to include .c files.
// but we're keeping it simple since this isn't
// actually a full-on library and only for testing.
#include "opts.c"
#include "warc.c"



//------------------------------------------------------------------------------
int
main(int argc, char *argv[])
{
	warc_st warc = {{0}};
	warc_ini(&warc);					// zero out.

	opts_get(&warc, argc, argv);		// will exit() if inputs don't pass

	warc_setup(&warc);					// mmap() file, etc

	warc_analyze(&warc);				// identify & label warc/uri blocks

	warc_zstd_compress(&warc);			// compress by uri block

	warc_zstd_write_files(&warc);		// write .zst and .zst.dat files

	warc_print_finish(&warc);			// print some stats

	warc_fin(&warc);					// free()

	return 0;
}
