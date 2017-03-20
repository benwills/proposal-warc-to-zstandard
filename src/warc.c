#pragma once

#include "main.h"

#include "mmap.c"
#include "timer.c"
#include "zstd.c"


// order of functions below is the actual order of processing.
// you could read this file and understand the most critical
// parts of the process.


//------------------------------------------------------------------------------
void
print_flush(const char *fmt, ...)
{
	printf("\n");
    va_list argptr;
    va_start(argptr, fmt);
    vprintf (fmt, argptr);
    va_end  (argptr);
	fflush  (stdout);
}

//------------------------------------------------------------------------------
void
print_header(const char *fmt, ...)
{
	printf("\n\n================================"
	           "================================\n");
    va_list argptr;
    va_start(argptr, fmt);
    vprintf (fmt, argptr);
    va_end  (argptr);
	fflush  (stdout);
}

//------------------------------------------------------------------------------
int
warc_ini(warc_st *warc)
{
	memset(warc, 0, sizeof(*warc));

	return R_SCS;
}

//------------------------------------------------------------------------------
int
warc_setup(warc_st *warc)
{
	if (DBG)print_header("Setting up warc file:\n\n\t%s\n\n", warc->filename);

	char *fnm_base = basename(warc->filename);

	// create our .zst filename
    if (strlen(fnm_base + strlen(".zst")) > PATH_MAX)
    	ERR_DIE("Filename too long for .zst extension. Check PATH_MAX")
    sprintf(warc->zstd.filename.zst, "%s%s.zst", DIR_ZST, fnm_base);

	// create our .zst.dat filename
    if (strlen(fnm_base + strlen(".zst.dat")) > PATH_MAX)
    	ERR_DIE("Filename too long for .zst.dat extension. Check PATH_MAX")
    sprintf(warc->zstd.filename.dat, "%s%s.zst.dat", DIR_ZST_DAT, fnm_base);

	// load a zstd dictionary, if applicable
    if (1 == warc->zstd.dict.use)
    	zstd_c_dict_load(warc);

    // now map our input file to memory
	mmap_ini(&warc->file, warc->filename);

    if (DBG)mmap_print(&warc->file);

	if (DBG)print_flush("Done\n");

	return R_SCS;
}

//------------------------------------------------------------------------------
int
warc_analyze(warc_st *warc)
{
	if (DBG)print_header("Analyzing warc file:\n\n\t%s\n\n", warc->filename);

	// this is not at all perfect. i'm sure something, somewhere, in
	// some file will screw this up. but since this is a proof-of-concept,
	// it will work for now.

	//--------------------------------------------------------------
	char  *beg = warc->file.beg;
	char  *end = warc->file.end;			// final byte file buffer
	char  *pos = beg;

	size_t byt = 0;							// bytes written
	size_t lft = warc->file.dat.st_size;	// bytes left

	char  *t_c = NULL;						// temp char pointer
	size_t t_b = 0;							// temp byte len


	//--------------------------------------------------------------
	// first, the warcinfo

	// this should be the first bytes of the document and match exactly
	if (0 != memcmp(pos, WARC_STR_BEG_NFO, strlen(WARC_STR_BEG_NFO)))
    	ERR_DIE("WARC File: Invalid warcinfo position:");

    // order is important for do{...}while();
    // need to do this, then jump in...

	t_c = memmem(pos, lft, WARC_STR_BEG_REQ, strlen(WARC_STR_BEG_REQ));
	if (NULL == t_c)
    	ERR_DIE("WARC File: Invalid warcinfo->request position:");

    t_b = (size_t)((ptrdiff_t)(t_c - pos));
	warc->dat.doc.warcinfo.byt = t_b;
	byt += t_b;
	lft -= t_b;
    warc->dat.doc.warcinfo.pos = pos;
	pos  = t_c;


	//--------------------------------------------------------------
	// now the uri request, response, and metadata
	do
	{
		//----------------------------------------------------------
		// pos is now on the start of a new uri request; WARC_STR_BEG_REQ

		++warc->dat.uri.cnt;

		// add a new warc_uri_st to our array
		t_b = warc->dat.uri.cnt * sizeof(*warc->dat.uri.arr);
		warc_uri_st *tmp = realloc(warc->dat.uri.arr, t_b);
		if (NULL == tmp) ERR_DIE("realloc() while adding a new URI");
		warc->dat.uri.arr = tmp;

		// little helper
		warc_uri_st *uri = &warc->dat.uri.arr[warc->dat.uri.cnt - 1];

		// now finish the rest of our parsing...

		//----------------------------------------------------------
		// find response, set request

		t_c = memmem(pos, lft, WARC_STR_BEG_RSP, strlen(WARC_STR_BEG_RSP));
		if (NULL == t_c)
	    	ERR_DIE("WARC File: Invalid request->response position");

	    t_b = (size_t)((ptrdiff_t)(t_c - pos));
		uri->request.byt = t_b;
		byt += t_b;
		lft -= t_b;
	    uri->request.pos = pos;
		pos  = t_c;

		//----------------------------------------------------------
		// set uri (since we now have request block size)
	    // !!! don't change (byt|lft|pos) here

		t_c = memmem(uri->request.pos, uri->request.byt,
		             WARC_STR_HDR_URI, strlen(WARC_STR_HDR_URI));
		if (NULL == t_c)
	    	ERR_DIE("WARC File: Invalid request->uri position");

	    // set our uri position.
	    uri->uri.pos = t_c;
	    // then move to end of the line
	    t_c += strcspn(t_c, "\r\n");
	    // then calculate our uri's byte length
	    t_b = (size_t)((ptrdiff_t)(t_c - (char*)uri->uri.pos));
		if (0 == t_b)
	    	ERR_DIE("WARC File: Invalid uri length");
		uri->uri.byt = t_b;

		//----------------------------------------------------------
		// find metadata, set response

		t_c = memmem(pos, lft, WARC_STR_BEG_MTA, strlen(WARC_STR_BEG_MTA));
		if (NULL == t_c)
	    	ERR_DIE("WARC File: Invalid response->metadata position");

	    t_b = (size_t)((ptrdiff_t)(t_c - pos));
		uri->response.byt = t_b;
		byt += t_b;
		lft -= t_b;
	    uri->response.pos = pos;
		pos  = t_c;

		//----------------------------------------------------------
		// find request, set metadata

		t_c = memmem(pos, lft, WARC_STR_BEG_REQ, strlen(WARC_STR_BEG_REQ));
		if (NULL == t_c)
		{
			// we would usually just ERR_DIE() here
			// but we might be at the end of the file...
			// for this implementation, we'll just go ahead and assume that.
			// if you want anything more robust, that's a bad assumption.

			// +1 since end = final byte, not the byte after
		    t_b = (size_t)((ptrdiff_t)(end - pos + 1));
			uri->metadata.byt = t_b;
			byt += t_b;
			lft -= t_b;
		    uri->metadata.pos = pos;
			pos  = end;
			break;
		}

	    t_b = (size_t)((ptrdiff_t)(t_c - pos));
		uri->metadata.byt = t_b;
		byt += t_b;
		lft -= t_b;
	    uri->metadata.pos = pos;
		pos  = t_c;

	} while(pos <= end);

	//--------------------------------------------------------------
	if (DBG)print_flush("Done\n");

	return R_SCS;
}

//------------------------------------------------------------------------------
int
warc_zstd_compress(warc_st *warc)
{
	if (DBG)print_header("Compressing warc file with zstd, "
	                     "compression level: %d...", warc->zstd.level);

	//--------------------------------------------------------------
	// party time
	timer_start(&warc->zstd.timer);

	//--------------------------------------------------------------
	// calculate and allocate memory needed for zstd output buffer
	// * see README.md for why it's done this way
	warc->zstd.out.byt  = 0;
	warc->zstd.out.byt += ZSTD_compressBound(warc->dat.doc.warcinfo.byt);
	for (int i = 0; i < warc->dat.uri.cnt; ++i)
	{
		warc_uri_st   *uri  = &warc->dat.uri.arr[i];
		warc->zstd.out.byt += ZSTD_compressBound(uri->request.byt);
		warc->zstd.out.byt += ZSTD_compressBound(uri->response.byt);
		warc->zstd.out.byt += ZSTD_compressBound(uri->metadata.byt);
	}

	warc->zstd.out.buf = calloc(1, warc->zstd.out.byt);
	if (NULL == warc->zstd.out.buf)
		ERR_DIE("ZSTD: calloc() output buffer");

	//--------------------------------------------------------------
	void  *z_buf = warc->zstd.out.buf;	// output buffer
	size_t z_lft = warc->zstd.out.byt;	// bytes left in z_buf
	size_t z_off = 0;					// z_buf offset
	size_t z_wrt = 0;					// bytes written by most recent compress

	//--------------------------------------------------------------
	// first, compress and write our warcinfo data
	z_wrt = zstd_compress(warc, z_buf, z_lft,
		                  warc->dat.doc.warcinfo.pos,
		                  warc->dat.doc.warcinfo.byt);

	// compressed. now set warcinfo vals
	warc->zstd.warcinfo.off = 0;	// since it's the first block written
	warc->zstd.warcinfo.byt = z_wrt;

	// adjust our zstd output vars
	z_off += z_wrt;
	z_buf += z_wrt;
	z_lft -= z_wrt;

	//--------------------------------------------------------------
	// now loop through all of the urls and compress

	//----------------------------------
	// first, allocate our zstd_uri_st array
	warc->zstd.uri = calloc(warc->dat.uri.cnt, sizeof(*warc->zstd.uri));
	if (NULL == warc->zstd.uri)
		ERR_DIE("ZSTD: calloc() uri array");

	//----------------------------------
	// now loop the uri data
	for (int i = 0; i < warc->dat.uri.cnt; ++i)
	{
		warc_uri_st *u = &warc->dat.uri.arr[i];
		zstd_uri_st *z = &warc->zstd.uri[i];

		//------------------------------
		// first, the request
		z_wrt = zstd_compress(warc, z_buf, z_lft,
		                      u->request.pos, u->request.byt);
		z->request.off = z_off;
		z->request.byt = z_wrt;
		z_off         += z_wrt;
		z_buf         += z_wrt;
		z_lft         -= z_wrt;

		//------------------------------
		// next, the response
		z_wrt = zstd_compress(warc, z_buf, z_lft,
		                      u->response.pos, u->response.byt);
		z->response.off = z_off;
		z->response.byt = z_wrt;
		z_off          += z_wrt;
		z_buf          += z_wrt;
		z_lft          -= z_wrt;

		//------------------------------
		// finally, the metadata
		z_wrt = zstd_compress(warc, z_buf, z_lft,
		                      u->metadata.pos, u->metadata.byt);
		z->metadata.off = z_off;
		z->metadata.byt = z_wrt;
		z_off          += z_wrt;
		z_buf          += z_wrt;
		z_lft          -= z_wrt;
	}

	warc->zstd.out.byt = z_off;

	//--------------------------------------------------------------
	// party's over
	timer_stop(&warc->zstd.timer);

	if (DBG)print_flush("Done\n");

	return R_SCS;
}

//------------------------------------------------------------------------------
int
warc_zstd_write_files(warc_st *warc)
{
	FILE  *fp  = NULL;
	size_t byt = 0;
	int    ret = 0;

	//----------------------------------
	if (DBG)print_header("Writing zstd-compressed warc zst "
	                     "file to:\n\t%s\n\t...", warc->zstd.filename.zst);

    fp = fopen(warc->zstd.filename.zst, "w");
    if (NULL == fp)
    	ERR_DIE("Could not fopen() .zst file for writing");

    byt = fwrite(warc->zstd.out.buf, 1, warc->zstd.out.byt, fp);
    if (byt != warc->zstd.out.byt)
    	ERR_DIE("Could not fwrite() .zst file for writing");

    if (fclose(fp))
    	ERR_DIE("Could not fclose() .zst file for writing");

	if (DBG)print_flush("Done\n");

	//----------------------------------
	if (DBG)print_header("Writing zstd-compressed warc dat "
	                     "file to:\n\t%s\n\t...", warc->zstd.filename.dat);

    fp = fopen(warc->zstd.filename.dat, "wb");
    if (NULL == fp)
    	ERR_DIE("Could not fopen() .zst.dat file for writing");

	/*
		output:

		>	uri count \n
		>	warcinfo \t offset \t byte len
		)	for each uri:
			>	uri \n
				>	\t request  \t offset \t bytes \n
				>	\t response \t offset \t bytes \n
				>	\t metadata \t offset \t bytes \n
	*/

    // total urls...
    ret = fprintf(fp, "total urls\t%d\n", warc->dat.uri.cnt);
    if (ret < 0) ERR_DIE("Could not fprintf() .zst.dat");

    // warcinfo
    ret = fprintf(fp, "warcinfo\t%zu\t%zu\n",
                       warc->zstd.warcinfo.off, warc->zstd.warcinfo.byt);
    if (ret < 0) ERR_DIE("Could not fprintf() .zst.dat");

    // loop through urls
    for (int i = 0; i < warc->dat.uri.cnt; ++i)
    {
    	zstd_uri_st *u = &warc->zstd.uri[i];
	    ret = fprintf(fp, "%.*s\n"				// URI
	                  	  "\treq:\t%zu\t%zu\n"	// request;  offset, bytes
	                  	  "\trsp:\t%zu\t%zu\n"	// response; offset, bytes
	                  	  "\tmta:\t%zu\t%zu\n"	// metadata; offset, bytes
	                  	  "\n",
	                       (int  )warc->dat.uri.arr[i].uri.byt,
	                       (char*)warc->dat.uri.arr[i].uri.pos,
	                       u->request.off,  u->request.byt,
	                       u->response.off, u->response.byt,
	                       u->metadata.off, u->metadata.byt);
	    if (ret < 0) ERR_DIE("Could not fprintf() .zst.dat");
    }

    if (fclose(fp))
    	ERR_DIE("Could not fclose() .zst.dat file after writing");

	if (DBG)print_flush("Done\n");

	return R_SCS;
}

//------------------------------------------------------------------------------
int
warc_print_finish(warc_st *warc)
{
	printf("\n----------------------------------------------------\n");
	printf("\n");
	printf("Completed zstd compression:\n");
	printf("\n");
	printf("\tFILE : %s\n",  warc->filename);
	printf("\tSIZE : %zu\n", warc->file.byt);
	printf("\tZSTD : %zu\n", warc->zstd.out.byt);
	printf("\tLEVEL: %d\n",  warc->zstd.level);
	printf("\tTIME : ");
	timer_diff_print(&warc->zstd.timer);
	printf("\n\n");
	fflush(stdout);

	return R_SCS;
}

//------------------------------------------------------------------------------
int
warc_fin(warc_st *warc)
{
	mmap_fin(&warc->file);

    free(warc->zstd.out.buf);
	free(warc->dat.uri.arr);
	free(warc->zstd.uri);

	return R_ERR;
}
