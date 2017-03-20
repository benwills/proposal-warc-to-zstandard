#pragma once

#ifndef _LARGEFILE64_SOURCE
	#define _LARGEFILE64_SOURCE
#endif

#ifndef _GNU_SOURCE
	#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <inttypes.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stddef.h>
#include <time.h>
#include <errno.h>
#include <linux/limits.h>			// PATH_MAX
#include <pthread.h>

#define ZSTD_STATIC_LINKING_ONLY	// ZSTD_findDecompressedSize
#include <zstd.h>     	 			// presumes zstd library is installed


//------------------------------------------------------------------------------
//
// constants
//

//--------------------------------------
//
// settings
//

// consider as an optarg...
#define DIR_ZST     "../warc_zst/"
#define DIR_ZST_DAT "../warc_zst_dat/"


//--------------------------------------
//
// debug
//

#ifndef DBG
	#define DBG 1
#endif

#define DBG_FFL do{if(DBG){printf("DBG: FFL: [%s] [%s] [%d]\n",        \
				__FILE__, __func__, __LINE__);fflush(stdout);}}while(0);

//--------------------------------------
//
// return constants
//

#ifndef R_SCS
	#define R_SCS  0
#endif
#ifndef R_ERR
	#define R_ERR -1
#endif

#ifndef R_TRU
	#define R_TRU 1
#endif
#ifndef R_FLS
	#define R_FLS 0
#endif

//--------------------------------------
//
// errors
//

#define ERR_RST() (errno == 0 ? "None" : strerror(errno))

#define ERR_DIE(MSG, ...)                                                      \
        do{                                                                    \
            fprintf(stderr, "FATAL: (%s:%d:%s: errno: %s)\n\t" MSG "\n",       \
                    __FILE__, __LINE__, __func__, ERR_RST(), ##__VA_ARGS__);   \
            exit(EXIT_FAILURE);                                                \
        }while(0);

//--------------------------------------
//
// zstd
//

#define ZSTD_DEFAULT_LEVEL 1


//------------------------------------------------------------------------------
//
// structs
//

//--------------------------------------
//
// timer
//

typedef struct
timer_st
{
	clock_t start;
	clock_t stop;
} timer_st;

//--------------------------------------
//
// mmap()d warc file
//

typedef struct
mmap_st
{
    char       *fnm;					// must point to null-termed string
    struct stat dat;					// fnm file data
    void       *mem;					// mmap()ed pointer

										// helpers...
    size_t      byt;
    void       *beg;
    void       *end;					// last byte
} mmap_st;

//--------------------------------------
//
// zstd elements
//

typedef struct 							// zstd memory block
zstd_blk_st
{
	off_t  off;
	size_t byt;
} zstd_blk_st;

typedef struct 							// zstd uri data
zstd_uri_st
{
	zstd_blk_st request;
	zstd_blk_st response;
	zstd_blk_st metadata;
} zstd_uri_st;

//--------------------------------------
//
// warc elements
//

typedef struct 							// warc memory block
warc_blk_st
{
	void  *pos;
	size_t byt;
} warc_blk_st;

typedef struct 							// warc uri data
warc_uri_st
{
	#define WARC_STR_BEG_REQ     		"WARC/1.0\r\nWARC-Type: request\r\n"
	warc_blk_st request;				// warc request header

	#define WARC_STR_BEG_RSP     		"WARC/1.0\r\nWARC-Type: response\r\n"
	warc_blk_st response;				// warc response header + HTTP data

	#define WARC_STR_BEG_MTA     		"WARC/1.0\r\nWARC-Type: metadata\r\n"
	warc_blk_st metadata;				// warc metadata header

	#define WARC_STR_HDR_URI 			"WARC-Target-URI: "
	warc_blk_st uri;					// sub-block of request
} warc_uri_st;

//--------------------------------------
//
// master warc data
//

typedef enum
warc_run_et
{
	WARC_RUN__NULL = 0,
	WARC_RUN_COMPRESS,
	WARC_RUN_DECOMPRES,
} warc_run_et;

typedef struct 							// master warc struct
warc_st
{
	char filename[PATH_MAX + 1];		// from user input; option -I
	mmap_st     file;
	warc_run_et run;					// (de)/compress; user input; -c/-d

	struct {
		struct {
			#define WARC_STR_BEG_NFO	"WARC/1.0\r\nWARC-Type: warcinfo\r\n"
			warc_blk_st warcinfo;		// warc warcinfo/file header
		} doc;

		struct {
			warc_uri_st *arr;
			int          cnt;
		} uri;
	} dat;

	struct {
		timer_st timer;
		int      level;					// compression level; user input; -L

		struct {
			int         use;
			ZSTD_CDict *c;				// compression dictionary
			ZSTD_DDict *d;				// decompression dictionary
		} dict;

		struct {
			ZSTD_CCtx *c;				// compression context
			ZSTD_DCtx *d;				// decompression context
		} ctx;

		zstd_blk_st  warcinfo;
		zstd_uri_st *uri;				// array; size == warc.dat.uri.cnt

		struct {						// used in both compress and decompress
			void  *buf;
			size_t byt;
			size_t fin;
		} out;

		struct {
			char dict[PATH_MAX + 1];	// dictionary; user input; option -D
			char zst [PATH_MAX + 1];	// .zst
			char dat [PATH_MAX + 1];	// .zst.stat.txt
		} filename;

		struct {
			void  *mem;					// warc.file.mem + warc.zstd.slice.off
			off_t  off;					// user input; - o
			size_t byt;					// user input; - b
		} slice;

	} zstd;

} warc_st;
