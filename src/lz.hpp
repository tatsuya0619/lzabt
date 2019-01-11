// -*- coding: utf-8 -*-
#pragma once

#include <string>
#include <vector>
#include <climits>
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include "string_tree_ns.hpp"
#include "string_tree.hpp"
#include "string_tree_compressed.hpp"
#include "derivation_tree.hpp"
#include "slp2enc.hpp"
#define THRESHOLD (1e-03)

namespace LZFF {
  using pair_vec = std::vector<std::pair<unsigned int, unsigned int>>;
  class FileInfo {
    public:
    unsigned long long fileSize;
    unsigned long long seqSize;
    unsigned int codeSize;
    unsigned int last_right_factor;
    FileInfo(unsigned int fileSize, unsigned int seqSize,
             unsigned int codeSize, unsigned int last_right_factor) :
      fileSize(fileSize), seqSize(seqSize),
      codeSize(codeSize), last_right_factor(last_right_factor){};
  };
  inline void writeHeader(std::ostream & os,
    FileInfo & info){
    os.write(reinterpret_cast<const char *> (&info.fileSize), sizeof(info.fileSize));
    os.write(reinterpret_cast<const char *> (&info.seqSize), sizeof(info.seqSize));
    os.write(reinterpret_cast<const char *> (&info.codeSize), sizeof(info.codeSize));
    os.write(reinterpret_cast<const char *> (&info.last_right_factor), sizeof(info.last_right_factor));
  }
  inline void readHeader(std::istream & is, FileInfo & info){
    is.read(reinterpret_cast<char *> (&info.fileSize), sizeof(info.fileSize));
    is.read(reinterpret_cast<char *> (&info.seqSize), sizeof(info.seqSize));
    is.read(reinterpret_cast<char *> (&info.codeSize), sizeof(info.codeSize));
    is.read(reinterpret_cast<char *> (&info.last_right_factor), sizeof(info.last_right_factor));
  }
  inline std::string show(FileInfo & info) {
    std::ostringstream str;
    str << "fileSize=" << info.fileSize << ", seqSize=" << info.seqSize
        << ", codeSize" << info.codeSize << ", last_right_factor" << info.last_right_factor;
    return str.str();
  }
 
  unsigned int ff_compress(STree::Tree & tree,
					pair_vec&);
  unsigned int ff_compress(std::ifstream &ifs,
					pair_vec&);
  /**
   *   computes LZD
   *
   *  @param lz
   *  @param tree
   *
   *  @return the length of factor sequence
   */
  unsigned int ff_compress_ns(pair_vec&,
			      NOT_STREAM::STree::Tree & tree);

  /**
   *  computes LZD
   *
   *  @param str
   *  @param lz
   *
   *  @return the length of factor sequence
   */
  unsigned int ff_compress_ns(std::string & str,
			      pair_vec&);

  inline bool checkRatio(unsigned int x, unsigned int y, double coef_limit){
    return (double)x <= y*coef_limit;
  }

  //LZ-ABT, streaming, not compressed space.
  std::vector<unsigned int> ff_compress_abt(STree::Tree &,
					    pair_vec&,
					    double);
  std::vector<unsigned int> ff_compress_abt(std::ifstream &,
					    pair_vec&,
					    double);

  //LZ-ABT, streaming, compressed space.
  std::vector<unsigned int> ff_compress_abt_compressed(STreeComp::Tree &tree,
						       pair_vec &seq,
						       double coef_limit);

  //LZ-ABT, streaming, compressed space.
  std::vector<unsigned int> ff_compress_abt_compressed(std::ifstream &,
						       pair_vec&,
						       double);
  
  //LZ-ABT, not streaming, not compressed space.
  std::vector<unsigned int> ff_compress_abt_ns(pair_vec&,
					       NOT_STREAM::STree::Tree & tree,
					       double coef_limit);
  
  std::vector<unsigned int> ff_compress_abt_ns(std::string & str,
					       pair_vec &,
					       double coef_limit);

  /**
   *  computes an SLP from a factor sequence of LZD by concatenating adjacent factors
   *
   *  @param seq  factor sequence
   *  @param vars output SLP
   */
  std::vector<unsigned int>  seq2vars(const pair_vec & seq, pair_vec & vars,
				      std::vector<unsigned int> stacked_fids = std::vector<unsigned int>());

  /**
   *  computes an SLP from a factor sequence of LZMW by concatenating adjacent factors
   *
   *  @param seq  factor sequence of LZMW
   *  @param vars variables of LZMW
   *  @param vars output SLP
   */
};
