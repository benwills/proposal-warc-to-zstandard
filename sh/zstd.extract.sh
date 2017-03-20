#!/bin/bash


# ./zstd.extract.sh -f /media/src/c/common_crawl/dat/CC-MAIN-20170219104609-00001-ip-10-171-10-108.ec2.internal.warc.zst -o 314602148 -b 8059 -d /media/src/c/common_crawl/dat/zstd.warc.dict

usage()
{
	echo "Usage: $0 -f <zst filename> -o <offset> -b <bytes> [-d <zst dictionary filename>]" 1>&2;
	exit 1;
}

while getopts ":f:o:b:d:" opt; do
    case "${opt}" in
        f)
            f=${OPTARG}
            ;;
        o)
            o=${OPTARG}
            ;;
        b)
            b=${OPTARG}
            ;;
        d)
            d=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

if [ -z "${f}" ] || [ -z "${o}" ] || [ -z "${b}" ]; then
    usage
fi

echo "f = ${f}"
echo "o = ${o}"
echo "b = ${b}"
echo "d = ${d}"





zstd -f -d ${f} -D ${d} | dd skip=${o} count=${b} iflag=skip_bytes,count_bytes > $(pwd)/zst.warc.extract.txt





