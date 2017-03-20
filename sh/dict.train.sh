#!/bin/bash

source inc.sh


#
# time for  15141 files of total size 1866 MB :
# 	real: 44m57.503s
# 	sys :  0m19.312s
#
# resulting dictionary size:
#	8,234,153 bytes
#


# can take a while...
if [ ! -f ${DICT_DIR} ]; then
	time \
	zstd --train \
		${DICT_SEED_DIR}/* \
		-o ${DICT_DIR} \
		--maxdict 10000000
fi
