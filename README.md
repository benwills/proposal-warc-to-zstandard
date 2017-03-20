# WARC: Gzip to Zstandard Compression Proposal

---

## Problem

The [Common Crawl](http://commoncrawl.org/) is a regularly-published collection of billions of HTML documents, HTML link data, and HTML text data.

The most recent, [the February 2017 dataset](http://commoncrawl.org/2017/03/february-2017-crawl-archive-now-available/) has 3.08 billion URIs and totals 55.88 terabytes of WARC files (complete html), 20.67 terabytes of WAT files (link data), and 9.14 terabytes of WET files (plain text).

100Mbit/s connections are able to download about 33 terabytes a month. This means that WARC files would take nearly two months to download, and the additional WAT and WET files would take almost another month as well. Given the (approximately) two month cycle of Common Crawl releases, a new release would be out before someone on a 100Mbit/s connection would be able to finish downloading the previous.

Therefore, the greater the compression ratio used by the Common Crawl, the more accessible it will be.

## Current Compression Methods

The Common Crawl currently references [this WARC specification](https://github.com/iipc/warc-specifications/blob/gh-pages/specifications/warc-format/warc-1.1/index.md#annex-a-informative-compression-recommendations) for compression.

The key compression requirements appear to be:

> For this purpose, the GZIP format with customary "deflate" compression is recommended, as defined in [RFC1950], [RFC1951], and [RFC1952].

and

> Per section 2.2 of the GZIP specification, a valid GZIP file consists of any number of gzip "members", each independently compressed.
>
> Where possible, this property should be exploited to compress each record of a WARC file independently. This results in a valid GZIP file whose per-record subranges also stand alone as valid GZIP files.
>
> External indexes of WARC file content may then be used to record each record's starting position in the GZIP file, allowing for random access of individual records without requiring decompression of all preceding records.

In short, the key here is that a WARC/WAT/WET file is not simply a large file that has been compressed. Rather, it is a series of smaller, wholly-independent compression blocks. These compressed blocks can then be accessed using a byte offset and byte length; when sent to a decompression function, only that block will be decompressed.

Compressing as separate blocks allows you to keep data compressed on disk, while also allowing for random access. This is demonstrated in the [Common Crawl Search Index](http://index.commoncrawl.org/CC-MAIN-2017-09/). If you [search for archive.org](http://index.commoncrawl.org/CC-MAIN-2017-09-index?url=archive.org&output=json), you'll see a result like this:

> {"urlkey": "org,archive)/", "timestamp": "20170219162823", "status": "301", "url": "http://archive.org/", "filename": "crawl-data/CC-MAIN-2017-09/segments/1487501170186.50/crawldiagnostics/CC-MAIN-20170219104610-00544-ip-10-171-10-108.ec2.internal.warc.gz", "length": "534", "mime": "text/html", "offset": "517355", "digest": "3I42H3S6NNFQ2MSVX7XZKYAYSCX5QBYJ"}

In this JSON object, the full HTTP response and WARC headers are referenced as:
* filename:
  * crawl-data/CC-MAIN-2017-09/segments/1487501170186.50/crawldiagnostics/CC-MAIN-20170219104610-00544-ip-10-171-10-108.ec2.internal.warc.gz
* offset:
  * 517355
* length:
  * 534

If you were to select the 534 bytes of that file, beginning at byte 517355, and decompress them, you would get that record for the home page of archive.org.

## Zstandard

Zstandard is a new(ish) compression algorithm by Yann Collet; the author of LZ4, a common compression algorithm also found in ZFS, and xxHash64, one of the fastest and most robust 64-bit non-cryptographic hash functions out today.

While Yann was working on Zstandard, he was hired by Facebook. Zstandard went on to be released by Facebook at version 1.0 and is now widely used by them in production.

Zstandard is designed for very fast decompression, and pretty good compression. By "pretty good," I mean that the PAQ series of algorithms generally have the [highest compression ratios](http://mattmahoney.net/dc/text.html), at the cost of far greater compression and decompression times.

In my testing, of which there is more data further below, I was able to achive a random WARC file compression size of 793,764,785 bytes vs Gzip's compressed size of 959,016,011.

This is about 82.7% of the Gzipped size. Using Zstandard's compression level of 8, it took about 3 minutes and 20 seconds on an Intel i3-4130 CPU @ 3.40GHz, while creating a new compression block for each "WARC-Type" of request, response, and metadata. Further compression should be achievable if those three WARC Types should actually be compressed in one block.

## Compression Dictionary

This was achieved through the use of a dictionary feature that comes with Zstandard.

It's possible to "train" a Zstandard dictionary given samples of the kinds of documents you will be compressing. If I understand correctly, that dictionary is then used to calculate term frequencies and optimized Huffman codes. That reference dictionary then also doesn't need to be included with each compression block, further reducing compression sizes.

Zstandard dictionary training prefers short text documents. Because WARC files - averaging 4.5-5 gigabytes - are not "short," it was necessary to extract the first 128k (per Zstandard requirements) of each document. You can see how this was able to be quickly done in ```sh/dict.seed.sh```

The final dictionary I built used the first 128k of 19,712 files. The dictionary took about 45 minutes to train and resulted in a final dictionary size of 8.2 megabytes. This file is at ```dict/zstd.warc.dict```

Note that you should be able to use the dictionary file included in this repo.

## Compression Block Slicing

Zstandard does not currently have a command-line option to decompress slices of files; for example, decompress 534 bytes at offset 517,355 of file WARC.zst.

However, the functions in the current (as of 2017.03.20) Zstandard C library do allow for this.

## Test Implementation

The code in this repository successfully parses a WARC file into blocks by "WARC-Type", compresses the block, and writes that block to a file. It then logs that URL, the offset, and byte length in a plain text file. An example of that file is in ```warc_zst_dat/```

```src/main.slice.c``` then demonstrates decompressing that block at specific offsets and byte lengths.

Both the compression and decompression make use of the included dictionary file at ```dict/zstd.warc.dict```

## Performance Tuning

The two factors that make Zstandard tunable for the WARC use case are:

1) Dictionary files. The larger the training input, the better.
2) The compression level.

The practical runtime considerations here are:

1) For dictionary files, it makes sense to test with as large of a data set as you can across an entire corpus. (e.g. all of Common Crawl's 2017.02 set vs. a separate dictionary for each file).
	* The training is quite fast. With 2.5 GB of WARC samples across 19,712 files, it took only about 45 minutes.
	* Setting a larger ```--maxdict``` size also helps.

2) Compression levels
	* The higher the compression level, the longer it takes to compress. Note that the training times and final sizes for various levels in the table below use a different dictionary than I've referenced above.
	* For comparison, the Gzipped file was 949,865,238 bytes:

Compression Level | Compression Time | Compressed Size
------------|-------------------|--------------
Level 1		|	17s, 217ms		|	969,729,191
Level 2		|	24s, 198ms		|	932,530,477
Level 3		|	40s, 724ms		|	892,326,602
Level 4		|	77s, 575ms		|	890,282,159
Level 5		|	65s, 457ms		|	849,449,579
Level 6		|	121s, 980ms		|	825,325,823
Level 7		|	177s, 764ms		|	812,392,350
Level 8		|	225s, 305ms		|	800,435,404
Level 9		|	277s, 780ms		|	795,902,253
Level 10	|	315s, 916ms		|	791,670,426
Level 11	|	477s, 254ms		|	789,060,640
Level 12	|	511s, 414ms		|	785,789,248
Level 13	|	620s, 656ms		|	783,608,901
Level 14	|	720s, 792ms		|	781,443,804
Level 15	|	825s, 927ms		|	775,446,236
Level 16	|	1131s, 729ms	|	772,099,885
Level 17	|	1116s, 361ms	|	768,755,082
Level 18	|	1631s, 792ms	|	757,769,728
Level 19	|	1922s, 710ms	|	743,957,209
Level 20	|	1977s, 620ms	|	740,306,436
Level 21	|	2576s, 578ms	|	735,544,787
Level 22	|	2960s, 894ms	|	735,042,474


Clearly, there is a computational threshold that makes sense for a project like the Common Crawl to stay under at the compression process. But these numbers should be useful for refernce when comparing to Gzip, which I expect to be somewhere around Compression Level 8 for Zstandard.

If that compression time is similar it would offer about 17% savings in overall size. This would bring the February 2017 crawl's WARC files down from 55.8 terabytes to around 46.5 terabytes. On a 100Mbit/s connection, this equates to saving the 2017.02 Common Crawl WARC files 8 days faster.

## Next Steps

The intent of this research is to propose Zstandard as a recommendation for more efficiently compressing WARC/WET/WAT files.

The code in this repository was not intended for use by anyone else, though I'm making it publicly available. If it makes sense to pursue this further, I'm happy to design more robust testing processes and code, including allowing for them to be more portable and run by others.

Refer to [this post on the Common Crawl Group](https://groups.google.com/forum/?hl=en#!topic/common-crawl/bO6B6xQJnEE) for discussing what this might look like moving forward.

---

## Pseudocode for Testing WARC Compression Using Zstandard

This was written early in the coding process. Some specifics have likely changed, though the important parts are still accurate.

>!!! DO NOT TRY AND RUN THIS CODE
>
> This repo is only public as a proof of concept and reference. It is not intended to be run by anyone else. If you get it working, great. If not, I won't be supporting making this usable elsewhere unless folks who make these kinds of decisions request that I do so.

	# Initialize Warc Struct
		- memset(warc_st, 0)

	# Read Command Line Options
		- Input Filename
			- If .gz, force gzip extraction
			- else, treat as decompressed plaintext
		- Input Directory
			- If not specified, use binary's current directory
		- Use dictionary
			- Get dictionary filename

	# Import File
		- Get filename from command line
		- If gzipped (determined only by .gz extension)
			- decompress
			- save to file
		- Memory Map warc file

	# Confirm our WarcInfo Header Position and Length
		- memmem(WARC_STR_BEG_NFO)

	# Traverse File, Set URI Entries
		) order in files is: request, response, meta
		- Set all requests: memmem(WARC_STR_BEG_REQ)
			- realloc() to add a new entry to the uri array
			) this is done as other warc parsers I've written can
			  get caught up on non-ascii chars (especially when
			  using str*() functions). this is likely unnecessary
			  here, but will play this demo a bit safer.
			) well set points for each request, then go back over
			  each request subsection to set response and metadata
			  points. and, finally, the uri for the entry.
		- For, and within, each large request block
			- Set all response blocks: memmem(WARC_STR_BEG_RSP)
			- Set all metadata blocks: memmem(WARC_STR_BEG_MTA)
		- Recalculate request block length using response pointer
		- Within the new, shorter/final request block
			- Set the uri: memmem(WARC_STR_HDR_URI)

	# Initialize and Start Timer

	# Allocate zstd Output Memory
		- Calculate memory size needed: ZSTD_compressBound()
			) because we're compressing the document in chunks,
			  and each chunk will have extra data (headers, etc),
			  we must also calculate the memory requirement based on
			  each of those segment sizes, and not the overall sum
			  of each segment length; e.g. ZSTD_compressBound(sum)
			- int zstd_out_byt = 0;
			- zstd_out_byt += ZSTD_compressBound(warcinfo)
			- For each uri
				- zstd_out_byt += ZSTD_compressBound(url.request.byt)
				- zstd_out_byt += ZSTD_compressBound(url.response.byt)
				- zstd_out_byt += ZSTD_compressBound(url.metadata.byt)
		- calloc(zstd_out_byt)

	# Compress
		- ZSTD_compress(warcinfo)
			- Store block offsets and byte lengths
				- warc.zstd.warcinfo in warc.zstd.filename.dat
		- For each warc.zstd.uri[]
			- For each of (reqest, response, metadata)
				- ZSTD_compress(...)
				- Store starting byte position and length
					- warc.zstd.uri[].[section] in warc.zstd.filename.dat

	# Stop Timer
		) note that the timer is used *only* for the
		  compression-related functions, and does not include
		  the disk write time. this is to get a fairer, more real
		  world metric as folks' memory and cpu speeds will likely
		  have less variation than disk speeds.
		) note, too, that this is still somewhat skewed as reading
		  the file is done using mmap(). therefore, the file is not
		  entirely allocated to memory, and it's possible to still
		  have performance affected by disk reads. this is attempted
		  to be mitigated using posix_madvise(POSIX_MADV_SEQUENTIAL)

	# Save Compressed File
		- to warc.zstd.filename.zst

	# Save Statistics to Plaintext File
		- to warc.zstd.filename.dat
		) Output Format: something like... (without spaces around tabs, etc)
			>	uri count \n
			>	warcinfo \t offset \t byte len
			)	for each uri:
				>	uri \n
					>	\t request  \t offset \t bytes \n
					>	\t response \t offset \t bytes \n
					>	\t metadata \t offset \t bytes \n
			) to choose/test a random uri to get offsets and lengths
			  1, find a random place in the file
              2, pos += strcspn(pos, "\n") + 1;
                 now you're at a url.
			  3, store url, iterate over next 3 lines for other data

	# Wrap up
		- free(warc.dat.uri.arr)
		- free(warc.zstd.uri)


---
    this does loop through the file twice. i've done this because this
    structure is useful for me for some other things i do with this data.
    if someone were to create a more optimized version of this, it would
    be very easy to go through the file only once and likely get a fair
    performance increase.

