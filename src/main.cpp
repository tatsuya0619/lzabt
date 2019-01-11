#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <getopt.h>
#include <cstdlib>
#include <cassert>
#include <chrono>

#include "common.hpp"
#include "string_tree.hpp"
#include "lz.hpp"
#include "slp2enc.hpp"

void usage(char * argv []){
    std::cout << "Usage  : " << argv[0] << " [options]" << std::endl
	      << "Options: " << std::endl
	      << "  -f FileName           : input file" << std::endl
	      << "  -o FileName           : output file" << std::endl
	      << "  -c                    : check whether decompressed string equals the input" << std::endl
	      << "  -d NUM                : set the debug level" << std::endl
      	      << "  -a lzd_ns             : LZD not using stream" << std::endl
              << "  -a lzd                : LZD" << std::endl
      	      << "  -a lzabt_ns           : LZABT not using stream" << std::endl
              << "  -a lzabt              : LZABT" << std::endl
	      << "  -a lzabt_comp         : LZABT in compressed space" << std::endl
	      << "  -p alpha              : this param is for LZ-ABT" << std::endl;

}



namespace algo{
  std::string LZD        = "lzd";
  std::string LZD_NS     = "lzd_ns";
  std::string LZABT      = "lzabt";
  std::string LZABT_COMP = "lzabt_comp";
  std::string LZABT_NS   = "lzabt_ns";  
};


unsigned int get_file_size(std::string filename){
  std::ifstream file(filename, std::ifstream::ate);
  unsigned int filesize = file.tellg();
  file.close();
  //std::cout << "input filesize = " << filesize << std::endl;
  return filesize;
}

int main(int argc, char * argv[]){
  int ch;
  std::string s, r, inFile;
  std::string algoname = "";
  std::string out_fname;
  bool help = false;
  double alpha = 0;
  while ((ch = getopt(argc, argv, "p:f:s:a:d:pl:o:h")) != -1) {
    switch (ch) {
    case 'f':
      inFile = optarg;
      break;
    case 'a':
      algoname = optarg;
      break;
    case 'd':
      UTIL::DEBUG_LEVEL = atoi(optarg);
      break;
    case 'o':
      out_fname = optarg;
      break;
    case 'h':
      help = true;
      break;

    case 'p':
      alpha = atof(optarg);
      break;
    }
  }
  if (help ||!(
	algoname == algo::LZD || algoname == algo::LZD_NS ||
	algoname == algo::LZABT || algoname == algo::LZABT_COMP ||
	algoname == algo::LZABT_NS
        )){
    usage(argv);
    exit(1);
  }


  if(algoname == algo::LZD){
    unsigned int filesize = get_file_size(inFile);
    std::ifstream ifs (inFile);
    
    std::vector<std::pair<unsigned int, unsigned int> > sequence;
    LZFF::ff_compress(ifs, sequence);
    std::vector<std::pair<unsigned int, unsigned int> > vars;
    std::vector<unsigned int> roots = LZFF::seq2vars(sequence, vars);
    slp2enc(vars, filesize, out_fname, roots.data(), roots.size());
    
  }else if(algoname == algo::LZD_NS){
    UTIL::stringFromFile(inFile, s);
    std::vector<std::pair<unsigned int, unsigned int>>seq;
    LZFF::ff_compress_ns(s, seq);
    std::vector<std::pair<unsigned int, unsigned int> > vars;
    std::vector<unsigned int> roots = LZFF::seq2vars(seq, vars);
    slp2enc(vars, (unsigned int) s.size(), out_fname, roots.data(), roots.size());

  }else if (algoname == algo::LZABT_NS){
    UTIL::stringFromFile(inFile, s);
    std::vector<std::pair<unsigned int, unsigned int>>seq;
    std::vector<unsigned int> stacked_fids = LZFF::ff_compress_abt_ns(s, seq, alpha);
    std::vector<std::pair<unsigned int, unsigned int> > vars;
    std::vector<unsigned int> roots = LZFF::seq2vars(seq, vars, stacked_fids);
    slp2enc(vars, (unsigned int) s.size(), out_fname, roots.data(), roots.size());
    
  }else if(algoname == algo::LZABT ||
	   algoname == algo::LZABT_COMP){
    unsigned int filesize = get_file_size(inFile);

    std::ifstream ifs (inFile);    
    std::vector<std::pair<unsigned int, unsigned int> > sequence;
    std::vector<unsigned int> stacked_fids = (algoname == algo::LZABT)?
      LZFF::ff_compress_abt(ifs, sequence, alpha):
      LZFF::ff_compress_abt_compressed(ifs, sequence, alpha);
    std::vector<std::pair<unsigned int, unsigned int> > vars;
    std::vector<unsigned int> roots = LZFF::seq2vars(sequence, vars, stacked_fids);
    slp2enc(vars, filesize, out_fname, roots.data(), roots.size());
    
  }

  return 0;
}
