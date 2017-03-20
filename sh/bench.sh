#!/bin/bash

source inc.sh


echo ""
echo ""
echo "NOTE: "
echo "      the zstd benchmark appears to"
echo "      only test the first ~2 GB of a file."
echo ""
echo "      if you get an error below that says:"
echo "          'Not enough memory; testing 2709 MB only...'"
echo "      remember that it's not testing the entire input file"
echo ""
echo ""

zstd -f ${FILE} -D ${DICT} -b1 -e8
