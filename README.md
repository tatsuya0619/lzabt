# LZD
This is an implementation of the LZ Double-factor factorization (LZD factorization).
LZD is a simple extension of the well-known compression algorithm LZ78.
While LZ78 factorize an input string to the sequence of pairs of a longest previous factor and the succeeding character, 
LZD factorize an input string to the sequence of pairs of a longest previous factor and the succeeding longest previous factor.
LZD shows better compression ratio than LZ78 in practical.

# LZ-ABT
LZ-ABT is a new LZ78-style grammar compression algorithm which is a simple online algorithm to create.
This can avoid the lower-bound time of the naive algorithms for LZD, other LZ78-style compression algorithms, which was observed in [Badkobeh et. al., SPIRE 2017, 51-67]


# Compile
We use [SCons](http://www.scons.org/) to build source codes.
To compile, you just type following command in the top of the project directory.
Binary files are put in `out` directory.

```sh
$ scons
```

Note that you may have to modify some settings such as compiler in SConstruct for your environment.


# Compress
## Usage

```sh
$ out/compress
Usage  : out/compress [options]
Options: 
  -f FileName           : input file
  -o FileName           : output file
  -c                    : check whether decompressed string equals the input
  -d NUM                : set the debug level
  -l maxSize            : set max code size
  -a lzd_ns             : LZD not using stream
  -a lzd                : LZD
  -a lzabt_ns           : LZABT_NS
  -a lzabt              : LZABT
  -a lzabt_comp         : LZABT in compressed space
```


## Examples
The following command compresses `SConstruct`, and output to `hoge.lz` by the algorithm LZ-ABT.

```sh
$ out/compress -f SConstruct -o hoge.lz -a lzabt -p 0.2
```

# Decompress
## Usage
```sh
% out/lzdDecompress
Usage  : out/decompress [options]
Options                 : 
  -f FileName           : input file
  -o FileName           : output file
  -d NUM                : debug mode

```
To decode binary file encoded with 'out/comress' command, use 'out/decompress'

## Examples
The following command decompresses `hoge.lz`  and output to `decoded.txt`.
All encoded files compressed by 'out/compress' have the same encode format
```sh
$ out/deecompress -f hoge.lz -o decoded.txt
```

# References
The detail of the algorithm was described in the following papers.

>Keisuke Goto, Hideo Bannai, Shunsuke Inenaga and Masayuki Takeda. LZD Factorization: Simple and Practical Online Grammar Compression with Variable-to-Fixed Encoding, In Proceedings of the 26th Annual Symposium on Combinatorial Pattern Matching

>Tatsuya Ohno, Keisuke Goto, Yoshimasa Takabatake, Tomohiro I and Hiroshi Sakamoto. LZ-ABT: A Practical Algorithm for \alpha-Balanced Grammar Compression, In Proceedings of the 29th International Workshop on Combinatorial Algorithms

# License
Copyright (c) 2018 Tatsuya Ohno. This software released under the MIT License.
