#!/bin/bash

: <<"eom"
echo ""
echo ""
echo "	You must specify where your gzipped warc files are"
echo "		in the line below, starting with 'for F in ...'"
echo ""
echo ""
echo "	When you do, you may remove this message"
echo "		and the subsequent exit call, then run this script"
echo ""
echo ""
echo "exiting..."
echo ""
echo ""
exit
eom

################################################################################

source inc.sh

C=0

mkdir ${DICT_SEED_DIR}

for DISK in {0..3}
do
	for F in /media/8tb_0${DISK}/cc/*.warc.gz*
	do
		echo "Processing  file..: ${F}";

		# zstd will read only the first 128k,
		#   per entry file, when generating the dictionary.
		gzip -cd ${F} | dd ibs=1024 count=128 > ${DICT_SEED_DIR}/seed.${C}

		C=$((C+1))
	done
done

echo ""
echo ""
echo "Files sampled: ${C}"

echo ""
echo ""
echo "gzipping...(not used by zstd. for convenenience only)"
echo ""
gzip -c ${DICT_SEED_DIR}/* ${DICT_SEED_GZ}


echo ""
echo ""
echo "now training the zstd dictionary. this can take some time..."
echo ""
