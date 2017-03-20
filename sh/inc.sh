#!/bin/bash

#
# !!!!! modify this as needed
# DO NOT INCLUDE THE "https://"
# this is so WARC_FILE can just pull the `basename`
# when using curl or wget, just add the https:// scheme prefix there
#
WARC_URL="commoncrawl.s3.amazonaws.com/crawl-data/CC-MAIN-2017-09/segments/1487501174157.36/warc/CC-MAIN-20170219104614-00154-ip-10-171-10-108.ec2.internal.warc.gz"
WARC_FILE="$(basename ${WARC_URL} .gz)"

################################################################################
#
# you shouldn't modify anything below here
#

ZSTD_LEVEL_MIN=8
ZSTD_LEVEL_MAX=8

APP_DIR_SRC="../src/"
APP_DIR_BIN="../bin/"
APP_DIR_OUT="../out/"
APP_BIN="${APP_DIR_BIN}cc_warc"

DICT_DIR="../dict/"
DICT_SEED_DIR="../dict/seed/"
DICT_FILE="zstd.warc.dict"
DICT_SEED_GZ="dict.seed.gz"

WARC_DIR="../warc/"
WARC_GZ_DIR="../warc_gz/"
WARC_ZST_DIR="../warc_zst/"
WARC_ZST_DAT_DIR="../warc_zst_dat/"
WARC_ZST_WARC_DIR="../warc_zst_warc/"

DICT="${DICT_DIR}${DICT_FILE}"
WARC="${WARC_DIR}${WARC_FILE}"

WARC_GZ="${WARC_GZ_DIR}${WARC_FILE}.gz"					# gzip compressed warc
WARC_ZST="${WARC_ZST_DIR}${WARC_FILE}.zst"				# zstd compressed warc
WARC_ZST_DAT="${WARC_ZST_DAT_DIR}${WARC_FILE}.zst.dat"	# index: urls+off+len
WARC_ZST_WARC="${WARC_GZ_DIR}${WARC_FILE}.zst.warc"		# warc from zst unzip


################################################################################
#
# confirm our variables
#

INC_PRINT_VARS()
{
	echo ""
	echo "################################################################"
	echo ""
	echo "inc.sh VARS:"
	echo ""
	echo "	WARC_URL:          ${WARC_URL}"
	echo "	WARC_FILE:         ${WARC_FILE}"
	echo ""
	echo "	ZSTD_LEVEL_MIN:    ${ZSTD_LEVEL_MIN}"
	echo "	ZSTD_LEVEL_MAX:    ${ZSTD_LEVEL_MAX}"
	echo ""
	echo "	APP_DIR_SRC:       ${APP_DIR_SRC}"
	echo "	APP_DIR_BIN:       ${APP_DIR_BIN}"
	echo "	APP_DIR_OUT:       ${APP_DIR_OUT}"
	echo "	APP_BIN:           ${APP_BIN}"
	echo ""
	echo "	DICT_DIR:          ${DICT_DIR}"
	echo "	DICT_SEED_DIR:     ${DICT_SEED_DIR}"
	echo "	DICT_FILE:         ${DICT_FILE}"
	echo "	DICT_SEED_GZ:      ${DICT_SEED_GZ}"
	echo ""
	echo "	WARC_DIR:          ${WARC_DIR}"
	echo "	WARC_GZ_DIR:       ${WARC_GZ_DIR}"
	echo "	WARC_ZST_DIR:      ${WARC_ZST_DIR}"
	echo "	WARC_ZST_DAT_DIR:  ${WARC_ZST_DAT_DIR}"
	echo "	WARC_ZST_WARC_DIR: ${WARC_ZST_WARC_DIR}"
	echo ""
	echo "	DICT:              ${DICT}"
	echo "	WARC:              ${WARC}"
	echo ""
	echo "	WARC_GZ:           ${WARC_GZ}"
	echo "	WARC_ZST:          ${WARC_ZST}"
	echo "	WARC_ZST_DAT:      ${WARC_ZST_DAT}"
	echo "	WARC_ZST_WARC:     ${WARC_ZST_WARC}"
	echo ""
	echo "################################################################"
	echo ""
	echo ""
}
