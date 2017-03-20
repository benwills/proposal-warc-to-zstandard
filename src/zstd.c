#pragma once

#include "main.h"

//
// TODO: rewrite compress to remove variables passed to it
//       get vars from warc_st
//

//------------------------------------------------------------------------------
void
zstd_ret_chk_die(size_t ret)
{
    if (ZSTD_isError(ret))
    {
        fprintf(stderr, "ZSTD: Compression: Error Code: %s \n",
                ZSTD_getErrorName(ret));
        ERR_DIE("ZSTD: Compression:");
    }
}

//------------------------------------------------------------------------------
// dictionary for compression
int
zstd_c_dict_load(warc_st *warc)
{
    if (0 == warc->zstd.dict.use)
        return R_SCS;

    if (DBG)printf("loading dictionary %s...\n", warc->zstd.filename.dict);

    struct stat st;
    if (0 != stat(warc->zstd.filename.dict, &st)) ERR_DIE("stat(c.dict)");

    FILE *fp = fopen(warc->zstd.filename.dict, "r");
    if (NULL == fp) ERR_DIE("stat(c.dict)");

    void *buf = malloc(st.st_size);
    if (NULL == buf) ERR_DIE("stat(c.dict)");

    size_t byt = fread(buf, 1, st.st_size, fp);
    if (byt != st.st_size) ERR_DIE("fread()");

    fclose(fp);

    warc->zstd.dict.c = ZSTD_createCDict(buf, st.st_size,
                                         warc->zstd.level);
    if (NULL == warc->zstd.dict.c) ERR_DIE("ZSTD_createCDict()");
    free(buf);

    warc->zstd.ctx.c = ZSTD_createCCtx();
    if (NULL == warc->zstd.ctx.c) ERR_DIE("ZSTD_createCCtx()");

    return R_SCS;
}

//------------------------------------------------------------------------------
// dictionary for decompression
int
zstd_d_dict_load(warc_st *warc)
{
    if (0 == warc->zstd.dict.use)
        return R_SCS;

    if (DBG)printf("loading dictionary %s...\n", warc->zstd.filename.dict);

    struct stat st;
    if (0 != stat(warc->zstd.filename.dict, &st)) ERR_DIE("stat(d.dict)");

    FILE *fp = fopen(warc->zstd.filename.dict, "r");
    if (NULL == fp) ERR_DIE("stat(d.dict)");

    void *buf = malloc(st.st_size);
    if (NULL == buf) ERR_DIE("stat(d.dict)");

    size_t byt = fread(buf, 1, st.st_size, fp);
    if (byt != st.st_size) ERR_DIE("fread()");
    fclose(fp);

    warc->zstd.dict.d = ZSTD_createDDict(buf, st.st_size);
    if (NULL == warc->zstd.dict.d) ERR_DIE("ZSTD_createDDict()");
    free(buf);

    warc->zstd.ctx.d = ZSTD_createDCtx();
    if (NULL == warc->zstd.ctx.d) ERR_DIE("ZSTD_createDCtx()");

    return R_SCS;
}

//------------------------------------------------------------------------------
size_t
zstd_compress(warc_st *warc, void *z_buf, size_t z_byt,
                             void *i_buf, size_t i_byt)
{
    size_t z_wrt = 0;

    if (warc->zstd.dict.use)
        z_wrt = ZSTD_compress_usingCDict(warc->zstd.ctx.c, z_buf, z_byt,
                                         i_buf, i_byt, warc->zstd.dict.c);
    else
        z_wrt = ZSTD_compress(z_buf, z_byt, i_buf, i_byt, warc->zstd.level);

    zstd_ret_chk_die(z_wrt);

    return z_wrt;
}

//------------------------------------------------------------------------------
size_t
zstd_decompress_ini(warc_st *warc)
{
    unsigned long long o_siz = 0;

    //------------------------------------------------------
    if (warc->zstd.dict.use)
        zstd_d_dict_load(warc);

    // may have off of zero. check byt.
    if (warc->zstd.slice.byt > 0)
    {
        warc->zstd.slice.mem = warc->file.mem + warc->zstd.slice.off;
        o_siz = ZSTD_findDecompressedSize(warc->zstd.slice.mem,
                                          warc->zstd.slice.byt);
    }
    else
    {
        o_siz = ZSTD_findDecompressedSize(warc->file.mem, warc->file.byt);
    }

    if (o_siz == ZSTD_CONTENTSIZE_ERROR)
        ERR_DIE("Not compressed with zstd");
    if (o_siz == ZSTD_CONTENTSIZE_UNKNOWN)
        ERR_DIE("Original content size is unknown");

    void *tmp = calloc(1, o_siz);
    if (NULL == tmp)
        ERR_DIE("zstd_decompress_ini: calloc");

    warc->zstd.out.buf = tmp;
    warc->zstd.out.byt = o_siz;

    return warc->zstd.out.byt;
}

//--------------------------------------
size_t
zstd_decompress(warc_st *warc)
{
    size_t o_siz = 0;
    size_t z_byt = 0;
    void  *z_buf = NULL;

    //------------------------------------------------------
    // may have off of zero. check byt.
    if (warc->zstd.slice.byt > 0)
    {
        z_buf = warc->zstd.slice.mem;
        z_byt = warc->zstd.slice.byt;
    }
    else
    {
        z_buf = warc->file.mem;
        z_byt = warc->file.byt;
    }

    //------------------------------------------------------
    if (warc->zstd.dict.use)
    {
        o_siz = ZSTD_decompress_usingDDict(warc->zstd.ctx.d,
                                           warc->zstd.out.buf,
                                           warc->zstd.out.byt,
                                           z_buf, z_byt,
                                           warc->zstd.dict.d);
    }
    else
    {
        o_siz = ZSTD_decompress(warc->zstd.out.buf,
                                warc->zstd.out.byt,
                                z_buf, z_byt);
    }

    zstd_ret_chk_die(o_siz);

    warc->zstd.out.fin = o_siz;

    return warc->zstd.out.fin;
}

//------------------------------------------------------------------------------
int
zstd_fin(warc_st *warc)
{
    if (           warc->zstd.dict.use
        && NULL != warc->zstd.dict.c)
        ZSTD_freeCDict(warc->zstd.dict.c);

    if (warc->zstd.ctx.c)
        ZSTD_freeCCtx(warc->zstd.ctx.c);

    if (           warc->zstd.dict.use
        && NULL != warc->zstd.dict.d)
        ZSTD_freeDDict(warc->zstd.dict.d);

    if (warc->zstd.ctx.d)
        ZSTD_freeDCtx(warc->zstd.ctx.d);

    return R_SCS;
}
