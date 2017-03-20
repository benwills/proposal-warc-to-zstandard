#!/bin/bash

source inc.sh
INC_PRINT_VARS


#
# TODO: optargs? ...use dictionary, benchmark min and max, etc...
#


################################################################################
#
# helpers
#

NEW_ITERATION()
{
	if [ -f ${WARC_ZST} ]; then
		rm ${WARC_ZST}
	fi

	if [ -f ${WARC_ZST_DAT} ]; then
		rm ${WARC_ZST_DAT}
	fi

	if [ -f ${WARC_ZST_WARC} ]; then
		rm ${WARC_ZST_WARC}
	fi

	echo ""
	echo ""
	echo "================================================================"
	echo ""
	echo ""
}

NEW_SECTION()
{
	echo ""
	echo ""
	echo "----------------------------------------------------------------"
	echo $1
	echo ""
}


################################################################################
#
# check and build our zstd dictionary
#
# can take a while...see notes in dict.train.sh
#
# TODO: if a dictionary hasn't been passed as an optarg here, disregard
#

if [ ! -f ${DICT} ]; then
	NEW_SECTION "DICTIONARY FILE DOES NOT EXIST"

	echo "There is not currently a dictionary file."
	echo "Generating one now with ./dict.train.sh"
	echo "This could take a while..."
	echo ""
	echo ""

	./dict.train.sh
fi


################################################################################
#
# might as well compile.
#

./make.sh


################################################################################
#
# run our tests
#

for ZSTD_LEVEL in `seq ${ZSTD_LEVEL_MIN} ${ZSTD_LEVEL_MAX}`
do

	########################################################
	NEW_ITERATION

	echo "ZSTD_LEVEL: ${ZSTD_LEVEL}"
	echo ""


	########################################################
	NEW_SECTION "Running draft C code to properly compress WARC files with ZSTD..."

	#
	# TODO: add output file argument
	#

	${APP_BIN} -c -I ${WARC} -L ${ZSTD_LEVEL} -D ${DICT}

	echo "zstd C compression code is finished."


	########################################################
	NEW_SECTION "Decompressing..."

	time zstd -q -f -d ${WARC_ZST} -o ${WARC_ZST_WARC} -D ${DICT}


	########################################################
	NEW_SECTION "Checking original and newly-decompressed file sizes..."

	BYT_INI=$(stat -c%s ${WARC})
	BYT_NEW=$(stat -c%s ${WARC_ZST_WARC})

	echo "	Original Size: ${BYT_INI}"
	echo "	New      Size: ${BYT_NEW}"

	if [ ${BYT_INI} -eq ${BYT_NEW} ]; then
		echo "SUCCESS: File sizes are equal"
	else
		echo "ERROR: Rile sizes are NOT EQUAL"
	fi


	########################################################
	NEW_SECTION "Checking original and newly-decompressed SHA-1 hashes..."

	SHA_INI=$(sha1sum ${WARC}          | cut -d ' ' -f 1)
	SHA_NEW=$(sha1sum ${WARC_ZST_WARC} | cut -d ' ' -f 1)

	echo "	Original SHA-1: ${SHA_INI}"
	echo "	New      SHA-1: ${SHA_NEW}"

	if [ "${SHA_INI}" == "${SHA_NEW}" ]; then
		echo "SUCCESS: Hashes are equal: "
	else
		echo "ERROR: Hashes NOT equal: "
	fi


	########################################################
	NEW_SECTION "Checking gzip vs zstd file sizes..."

	BYT_GZP=$(stat -c%s ${WARC_GZ})
	BYT_ZST=$(stat -c%s ${WARC_ZST})

	echo "	gzip: ${BYT_GZP}"
	echo "	zstd: ${BYT_ZST}"

	if [ ${BYT_GZP} -gt ${BYT_ZST} ]; then
		echo "GZIP file is LARGER."
	elif [ ${BYT_GZP} -lt ${BYT_ZST} ]; then
		echo "ZSTD file is LARGER."
	else
		echo "GZIP & ZSTD are the same byte size."
	fi


	########################################################
	echo ""
	echo ""
	echo ""
	echo ""
	echo ""
	echo ""
	echo ""
	echo ""

done

