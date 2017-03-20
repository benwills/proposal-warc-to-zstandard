#include "main.h"
#include "warc.c"
#include "zstd.c"


//------------------------------------------------------------------------------
void
opts_usage()
{
	printf("\n================================================\n");
	printf("USAGE:\n");
	printf("\t$ ./cc_zstd_extract -o OFFSET -b BYTES -I ZSTD_file"
		   "[-D dictionary]\n");
	printf("\n");
	printf("Required Arguments:\n");
	printf("\t-I ZSTD_file\n");
	printf("\t-o offset from where to begin\n");
	printf("\t-b bytes to extract\n");
	printf("\n");
	printf("Optional Arguments:\n");
	printf("\t-D dictionary_file\n");
	printf("\n");
	printf("\n------------------------------------------------\n");
	printf("\n");
	printf("\n");
	fflush(stdout);
}

//--------------------------------------
void
opts_usage_exit()
{
	opts_usage();
	exit(EXIT_FAILURE);
}

//------------------------------------------------------------------------------
int
opts_get(warc_st *warc, int argc, char *argv[])
{
	int opt;
	while ((opt = getopt(argc, argv, "o:b:I:D:")) != -1)
	{
		switch (opt)
		{
			case 'o':
				warc->zstd.slice.off = atoi(optarg);
				break;
			case 'b':
				warc->zstd.slice.byt = atoi(optarg);
				break;
			case 'I':
				strncpy(warc->zstd.filename.zst, optarg, strlen(optarg));
				break;
			case 'D':
				strncpy(warc->zstd.filename.dict, optarg, strlen(optarg));
				warc->zstd.dict.use = 1;
				break;
			default:
				opts_usage_exit();
		}
	}

	//--------------------------------------------------------------
	// offset and bytes
	// offset might be zero bytes...
	// if (0 == warc->zstd.slice.off)
	//	 ERR_DIE("must enter an offset: \n\t%zu\n",
	//	          warc->zstd.slice.off);
	if (0 == warc->zstd.slice.byt)
		ERR_DIE("must enter an byte length: \n\t%zu\n",
				warc->zstd.slice.byt);

	// zstd input
	if (0 == strlen(warc->zstd.filename.zst))
		opts_usage_exit();
	if (-1 == access(warc->zstd.filename.zst, F_OK))
		ERR_DIE("cannot access ZSTD file: \n\t%s\n",
				warc->zstd.filename.zst);

	// dictionary
	if (0 != strlen(warc->zstd.filename.dict))
		if (-1 == access(warc->zstd.filename.dict, F_OK))
			ERR_DIE("cannot access ZSTD Dictionary: \n\t%s\n",
					warc->zstd.filename.dict);

	//----------------------------------
	printf("Using Settings:\n\n");
	printf("\tFile  : %s\n",  warc->zstd.filename.zst);
	printf("\tDict  : %s\n",  warc->zstd.filename.dict);
	printf("\tOffset: %zu\n", warc->zstd.slice.off);
	printf("\tBytes : %zu\n", warc->zstd.slice.byt);
	fflush(stdout);

	return R_SCS;
}


//------------------------------------------------------------------------------
int
main(int argc, char *argv[])
{
	warc_st warc = {{0}};
	warc_ini(&warc);

	opts_get(&warc, argc, argv);

	mmap_ini(&warc.file, warc.zstd.filename.zst);

	zstd_decompress_ini(&warc);

	zstd_decompress	(&warc);

	printf("\n\nEXTRACTED %zu BYTES:\n\n%s\n\n",
		   warc.zstd.out.fin, (char*)warc.zstd.out.buf);
	fflush(stdout);

	zstd_fin(&warc);

	printf("done.\n");

	return 0;
}
