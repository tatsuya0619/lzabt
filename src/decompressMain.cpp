#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <getopt.h>
#include <cstdlib>
#include <cassert>

#include "common.hpp"
#include "lz.hpp"
#include "string_tree_compressed.hpp"
#include "slp2enc.hpp"

void usage(char * argv []){
    std::cout << "Usage  : " << argv[0] << " [options]" << std::endl
	      << "Options                 : " << std::endl
	      << "  -f FileName           : input file" << std::endl
	      << "  -o FileName           : output file" << std::endl
	      << "  -d NUM                : debug mode" << std::endl;
}

int main(int argc, char * argv[]){
  int ch;
  std::string in_fname, out_fname;
  bool help = false;
  while ((ch = getopt(argc, argv, "f:d:o:h")) != -1) {
    switch (ch) {
    case 'f':
      in_fname = optarg;
      break;
    case 'd':
      UTIL::DEBUG_LEVEL = atoi(optarg);
      break;
    case 'o':
      out_fname = optarg;
      break;
    case 'h':
      help = true;
    }
  }
  if (help || in_fname.empty() || out_fname.empty()){
    usage(argv);
    exit(1);
  }

  std::ifstream ifs(in_fname.c_str(), std::ios::in | std::ios::binary);
  std::ofstream iofs(out_fname.c_str());
  encSLP_decompress(in_fname, out_fname);
  return 0;
}
