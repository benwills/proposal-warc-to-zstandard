#pragma once

#include "main.h"


//------------------------------------------------------------------------------
void
mmap_print(mmap_st *map)
{
	printf("mmap()d:\n");
	printf("\tfilename : %s\n",        map->fnm        );
	printf("\tst_siz   : %"PRIu64"\n", map->dat.st_size);
	fflush(stdout);
}


//------------------------------------------------------------------------------
//
// this is not a generalized use-case function
// it's only for mapping files you intend on reading sequentially
// i wouldn't suggest using this elsewhere
//
int
mmap_ini(mmap_st *map, char *fnm)
{
	if (strlen(fnm) > PATH_MAX)
		ERR_DIE("%s\n","MMAP: filename longer than PATH_MAX\n");

	int   fd = 0;
	map->fnm = fnm;

	if (-1 == access(map->fnm, F_OK))
		ERR_DIE("%s\n","MMAP: access()");

	if (-1 == (fd = open(map->fnm, O_RDONLY, (S_IRUSR|S_IRGRP|S_IROTH))))
		ERR_DIE("%s\n","MMAP: open()");

	if (-1 == fstat(fd, &map->dat))
		ERR_DIE("%s\n","MMAP: fstat()");

	if (0 == S_ISREG(map->dat.st_mode))
		ERR_DIE("%s\n","MMAP: S_ISREG()");

	map->mem = mmap(0, map->dat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (MAP_FAILED == map->mem)
		ERR_DIE("%s\n","MMAP: mmap()");

	if (0 != posix_madvise(map->mem, map->dat.st_size, POSIX_MADV_SEQUENTIAL))
		ERR_DIE("%s\n","MMAP: posix_madvise()");

	// we can close fd now
	if (fd && -1 == close(fd))
		ERR_DIE("%s\n","MMAP: close()");

	// set up our helpers
	map->beg = map->mem;
	map->end = map->mem + map->dat.st_size - 1; // last byte
	map->byt = map->dat.st_size;

	return R_SCS;
}


//------------------------------------------------------------------------------
int
mmap_fin(mmap_st *map)
{
	munmap(map->mem, map->dat.st_size);
	// TODO: error checking
	return R_SCS;
}
