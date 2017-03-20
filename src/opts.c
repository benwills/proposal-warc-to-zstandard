#pragma once

#include "main.h"


//------------------------------------------------------------------------------
void
opts_usage()
{
    printf("\n================================================\n");
    printf("USAGE:\n");
    printf("\t$ ./cc_zstd [-c -d] -I input_file"
           "[-D dictionary -L compression level]\n");
    printf("\n");
    printf("Required Arguments:\n");
    printf("\t-I input_filename\n");
    printf("\t-c OR -d\n");
    printf("\t\t * takes no values\n");
    printf("\t\t-c = compress\n");
    printf("\t\t-d = decompress\n");
    printf("\n");
    printf("Optional Arguments:\n");
    printf("\t-D dictionary_file\n");
    printf("\t-L zstd_level ... defaults to 5\n");
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
    while ((opt = getopt(argc, argv, "cdI:D:L:")) != -1)
    {
        switch (opt)
        {
            case 'c':
                if (WARC_RUN__NULL != warc->run)
                    opts_usage_exit();
                warc->run = WARC_RUN_COMPRESS;
                break;
            case 'd':
                if (WARC_RUN__NULL != warc->run)
                    opts_usage_exit();
                warc->run = WARC_RUN_DECOMPRES;
                break;
            case 'I':
                strncpy(warc->filename, optarg, strlen(optarg));
                break;
            case 'D':
                strncpy(warc->zstd.filename.dict, optarg, strlen(optarg));
                warc->zstd.dict.use = 1;
                break;
            case 'L':
                warc->zstd.level = atoi(optarg);
                break;
            default:
                opts_usage_exit();
        }
    }

    //--------------------------------------------------------------
    // check files

    // must set (de)/compress option
    if (WARC_RUN__NULL == warc->run)
        opts_usage_exit();

    // warc input
    if (0 == strlen(warc->filename))
        opts_usage_exit();
    if (-1 == access(warc->filename, F_OK))
        ERR_DIE("cannot access WARC file: \n\t%s\n",
                warc->filename);

    // dictionary
    if (0 != strlen(warc->zstd.filename.dict))
        if (-1 == access(warc->zstd.filename.dict, F_OK))
            ERR_DIE("cannot access ZSTD Dictionary: \n\t%s\n",
                    warc->zstd.filename.dict);

    // default zstd compression level 5
    if (0 == warc->zstd.level)
        warc->zstd.level = ZSTD_DEFAULT_LEVEL;

/*
    //--------------------------------------------------------------
    // confirm with user they want to continue
    // TODO: only trigger if .zst and .zst.dat.txt files exist
    char usr_key = '\0';
    printf("\nPress [Enter] to overwrite any zstd files "
             "associated with this file\n");
    printf("\nOR press any other key to exit.\n");

    if (0 > scanf ("%c", &usr_key))
        ERR_DIE("scanf() on user key capture");

    if (   '\r' != usr_key
        && '\n' != usr_key)
    {
        printf("Did not press [Enter].\nExiting.\n\n");
        exit(EXIT_FAILURE);
    }
*/

    return R_SCS;
}
