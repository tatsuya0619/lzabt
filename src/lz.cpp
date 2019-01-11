// -*- coding: utf-8 -*-
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sys/stat.h>
#include <cassert>
#include <sstream>
#include <fstream>
#include "common.hpp"
#include "lz.hpp"
#include "slp2enc.hpp"
#define THRESHOLD (1e-03)

namespace LZFF{
  unsigned int ff_compress(STree::Tree & tree,
			   pair_vec&seq){

    std::string remain_string;
    bool unknown_chars[CHAR_SIZE];
    for(unsigned int i = 0; i < CHAR_SIZE; ++i)
      unknown_chars[i] = true;
    seq.push_back(std::make_pair(0,0)); // dummy
    while (tree.empty() || !remain_string.empty()){
      STree::Node * cur = tree.findLastFNode(remain_string);
      if(cur->depth == 1){
	unsigned char new_char = (unsigned char)cur->label[0];
	if(unknown_chars[new_char]){
	  unknown_chars[new_char] = false;
	  seq.push_back(std::make_pair(tree.root->fid, new_char));
	}
      }
      if(cur == tree.root) break;
      if (tree.firstFactor == NULL){
	tree.firstFactor = cur;
      }else{
	tree.insertFactorNode(tree.firstFactor, remain_string, 0, cur->depth);
	seq.push_back(std::make_pair(tree.firstFactor->fid, cur->fid));
	tree.firstFactor = NULL;
      }
      remain_string.erase(0, cur->depth);
    }
    if (tree.firstFactor != NULL) {
      seq.push_back(std::make_pair(tree.firstFactor->fid, tree.root->fid));
    }
    return seq.size();
  }

  unsigned int ff_compress(std::ifstream &ifs,
		   pair_vec&seq){
    STree::Tree tree(ifs, 1024);
    return ff_compress(tree, seq);
  }
  
  unsigned int ff_compress_ns(pair_vec &seq,
			      NOT_STREAM::STree::Tree & tree){
    assert(tree.root != NULL);

    unsigned int p = 0; // current position of the input string
    unsigned int n = (unsigned int) tree.getStrSize();
    seq.push_back(std::make_pair(0,0)); // for root node
    NOT_STREAM::STree::Node * firstFactor = NULL;
    while (p < n){
      NOT_STREAM::STree::Node * cur_factor = tree.findLastFNodeFrom(tree.root, p);
      assert(p + cur_factor->
             depth <= n);
      if (cur_factor == tree.root){
        seq.push_back(std::make_pair(tree.root->fid, (unsigned char) tree.getCharAt(p)));
        cur_factor = tree.insertCharNode(tree.getCharAt(p));
        assert(cur_factor->depth == 1);
        assert(cur_factor->isFactor());
      }
      if (firstFactor == NULL){
        firstFactor = cur_factor;
      }else {
        seq.push_back(std::make_pair(firstFactor->fid, cur_factor->fid));
        const NOT_STREAM::STree::Node * new_node = tree.insertFactorNode(firstFactor, p, cur_factor->depth);

        if (UTIL::DEBUG_LEVEL > 2){

          const unsigned int prev_p = p - firstFactor->depth;
          std::string ls, rs;
          ls = "";
          rs = "";
          tree.toString(firstFactor, ls);
          tree.toString(cur_factor, rs);
          std::cerr << "[" << new_node->fid << "]=" 
                    << "[" << firstFactor->fid << "]"
                    << "[" << cur_factor->fid << "]="
                    << "[" << ls << "]"
                    << "[" << rs << "]"
                    << std::endl;
          if (ls != tree.substr(prev_p, firstFactor->depth)){
            std::cerr << "prev_p=" << prev_p
                      << " depth=" << firstFactor->depth << std::endl;
            // std::cerr.flush();
            std::cerr << "origin[" << tree.substr(prev_p, firstFactor->depth) <<  "] "
                      << "firstfactor[" << ls << "]" << std::endl;
          }
          if (rs != tree.substr(p, cur_factor->depth)){
            std::cerr << "origin[" << tree.substr(p, cur_factor->depth) <<  "] "
                      << "secondfactor[" << rs << "]" << std::endl;
          }
	}
        firstFactor = NULL;
      }
      p += cur_factor->depth;
    }
    if (firstFactor != NULL){
      seq.push_back(std::make_pair(firstFactor->fid, tree.root->fid));
    }

    return (unsigned int) seq.size();
  }

  unsigned int ff_compress_ns(std::string & str, pair_vec &seq){
    NOT_STREAM::STree::Tree tree(str);
    return ff_compress_ns(seq, tree);
  }

  std::vector<unsigned int> ff_compress_abt_ns(pair_vec &seq,
					       NOT_STREAM::STree::Tree & tree,
				 double coef_limit){
      
    assert(tree.root != NULL);
    unsigned int p = 0; // current position of the input string
    unsigned int n = (unsigned int) tree.getStrSize();
    NOT_STREAM::STree::Node* stacked_factors[100];
    unsigned int sp = 0;
    unsigned int limit = 1;
    seq.push_back(std::make_pair(0,0)); // for root node

    while (p < n){
      NOT_STREAM::STree::Node * cur_factor = tree.findLastFNodeFrom(tree.root, p, limit);
      assert(p + cur_factor->depth <= n);
      if (cur_factor == tree.root){
	seq.push_back(std::make_pair(tree.root->fid, (unsigned char) tree.getCharAt(p)));
	cur_factor = tree.insertCharNode(tree.getCharAt(p));
      }
      
      stacked_factors[sp++] = cur_factor;
      p += cur_factor->depth;
      while(sp > 1){
	//get the 2 last factors
	NOT_STREAM::STree::Node* lasts[2] = {stacked_factors[sp-2], stacked_factors[sp-1]}; 
	 
	if(checkRatio(lasts[0]->depth, lasts[1]->depth, coef_limit)){
	  seq.push_back(std::make_pair(lasts[0]->fid, lasts[1]->fid));
	  NOT_STREAM::STree::Node * new_factor = tree.insertFactorNode(lasts[0], p - lasts[1]->depth,
								       lasts[1]->depth);
	  --sp;
	  stacked_factors[sp-1] = new_factor;
	}else break;
      }
      limit = stacked_factors[sp-1]->depth * coef_limit+THRESHOLD; //ファクターの長さの上限
    }

    
    std::vector<unsigned int> root_curs(0);
    for(unsigned int i = 0; i < sp; ++i){
      root_curs.push_back(stacked_factors[i]->fid);
    }
    
    return root_curs;
  }

  std::vector<unsigned int> ff_compress_abt_ns(std::string & str,
					       pair_vec &seq,
					       double alpha){
    NOT_STREAM::STree::Tree tree(str);
    double coef_limit;
    if(alpha < THRESHOLD || alpha > (double)1/2){
      std::cout << "alpha value is too small or too large, EXIT" << std::endl;
      exit(1);
    }
    coef_limit = (1-alpha)/alpha;
    return ff_compress_abt_ns(seq, tree, coef_limit);
  }
  
  std::vector<unsigned int> ff_compress_abt(STree::Tree & tree, std::vector<std::pair<unsigned int,
					    unsigned int>>&seq, double coef_limit){

    std::string remain_string;
    STree::Node* stacked_factors[100];
    std::string tmp = "";
    unsigned int sp = 0;
    unsigned int limit = 1;
    bool unknown_chars[CHAR_SIZE];

    
    for(unsigned int i = 0; i < CHAR_SIZE; ++i)
      unknown_chars[i] = true;
    seq.push_back(std::make_pair(0,0)); // dummy
    while (tree.empty() || !remain_string.empty()){
      STree::Node * cur = tree.findLastFNode(remain_string, limit);
      if(cur->depth == 1){
	unsigned char new_char = (unsigned char)cur->label[0];
	if(unknown_chars[new_char]){
	  unknown_chars[new_char] = false;
	  seq.push_back(std::make_pair(tree.root->fid, new_char));
	}
      }
      if(cur == tree.root) break;
      stacked_factors[sp++] = cur;
      remain_string.erase(0, cur->depth);

      while(sp > 1){
	STree::Node* lasts[2] = {stacked_factors[sp-2], stacked_factors[sp-1]};
	std::string path_label;
	if(checkRatio(lasts[0]->depth, lasts[1]->depth, coef_limit)){
	  tree.toString(lasts[1], path_label); //toString takes long time...
	  seq.push_back(std::make_pair(lasts[0]->fid, lasts[1]->fid));
	  STree::Node *new_node = tree.insertFactorNode(lasts[0], path_label, 0, path_label.length());
	  //tree.write(lasts[0]->fid, lasts[1]->fid);
	  --sp;
	  stacked_factors[sp-1] = new_node;
	}else break;
      }
      limit = stacked_factors[sp-1]->depth * coef_limit+THRESHOLD; //ファクターの長さの上限
    }
    std::vector<unsigned int> root_fids(0);
    for(unsigned int i = 0; i < sp; ++i)
      root_fids.push_back(stacked_factors[i]->fid);
    
    return root_fids;
  }

  std::vector<unsigned int> ff_compress_abt(std::ifstream &ifs,
					    std::vector<std::pair<unsigned int,
					    unsigned int>>&seq,
					    double alpha){
    STree::Tree tree(ifs, 1024);
    double coef_limit = (1-alpha)/alpha;
    return ff_compress_abt(tree, seq, coef_limit);
  }
  
  std::vector<unsigned int> ff_compress_abt_compressed(std::ifstream &ifs, pair_vec&seq, double alpha){
    //we prepare 2 stacks
    if(alpha < THRESHOLD || alpha > (double)1/2){
      std::cout << "alpha value is too small or too large EXIT" << std::endl;
      exit(1);
    }
    DTree::Tree::createRoutesBuffer(2);
    STreeComp::Tree tree(ifs);
    double coef_limit = (1-alpha)/alpha;
    return ff_compress_abt_compressed(tree, seq, coef_limit);
  }

    std::vector<unsigned int> ff_compress_abt_compressed(STreeComp::Tree &tree,
						       pair_vec &seq,
						       double coef_limit){

    using Node = STreeComp::Node;
    std::vector<Node*> stacked_factors;
    std::string remain_string;
    unsigned int bp = 0;
    unsigned int limit = 1;
    bool unknown_chars[CHAR_SIZE];
    unsigned int num_of_processed = 0;
    bool is_limited = false;
    unsigned int max_length = 10000000;
    for(unsigned int i = 0; i < CHAR_SIZE; ++i)
      unknown_chars[i] = true;

    seq.push_back(std::make_pair(0,0)); // dummy
    while (tree.empty() || !remain_string.empty()){
      Node * cur = tree.findLastFNode(remain_string, limit);
      if(cur->depth == 1){
	unsigned char new_char = (unsigned char)cur->path_label.getFirstChar();
	if(unknown_chars[new_char]){
	  unknown_chars[new_char] = false;
	  seq.push_back(std::make_pair(tree.root->getFid(), new_char));
	}
      }
      if(cur == tree.root) break;
      stacked_factors.push_back(cur);
      remain_string.erase(0, cur->depth);
      num_of_processed += cur->depth;
      while(stacked_factors.size() > 1 + bp){
	Node* lasts[2] = {stacked_factors[stacked_factors.size()-2],
			  stacked_factors[stacked_factors.size()-1]
	};
	
 	if(checkRatio(lasts[0]->depth, lasts[1]->depth, coef_limit)){
	  seq.push_back(std::make_pair(lasts[0]->getFid(), lasts[1]->getFid()));
	  DTree::Tree path_label = DTree::Tree(lasts[0]->path_label, lasts[1]->path_label);
	  path_label.setRange(lasts[0]->depth,
			      lasts[1]->depth, true);
	  
	  Node *new_node = tree.insertFactorNode(lasts[0], path_label);
	  assert(new_node!= NULL);
	  stacked_factors.pop_back();
	  stacked_factors[stacked_factors.size()-1] = new_node;
	}else break;
      }
      if(!is_limited && num_of_processed > max_length){
	is_limited = true;
	max_length = max_length*20/tree.getNumFNodes();
	for(; stacked_factors[bp]->depth > max_length && bp < stacked_factors.size(); ++bp);
      }

      if(is_limited && stacked_factors[bp]->depth > max_length){
	++bp;
      }

      limit = stacked_factors[stacked_factors.size()-1]->depth * coef_limit+THRESHOLD; //the limit of next factor length
    }
    
    std::vector<unsigned int> root_fids(0);
    for(unsigned int i = 0; i < stacked_factors.size(); ++i)
      root_fids.push_back(stacked_factors[i]->getFid());
    return root_fids;
  }
  


  std::vector<unsigned int>  seq2vars(const pair_vec & in_seq,
				      pair_vec & vars,
				      std::vector<unsigned int> stacked_fids
				      ){
    
    std::vector<unsigned int> s2v(in_seq.size());
    vars.clear();
    unsigned int cur = CHAR_SIZE;
    vars.reserve(in_seq.size() * 2);
    bool flag = false; //if the last factor doesn't have the right
    // stores dummy values for SLPs
    for(size_t i = 0; i < CHAR_SIZE; i++){
      vars.push_back(std::make_pair(-1, -1));
    }


    // renumbers variable ids for SLPs
    for(size_t i = 1; i < in_seq.size(); i++){
      assert(cur < in_seq.size() + CHAR_SIZE);
      if (in_seq[i].first == 0){
        // the factor represents a character
        s2v[i] = in_seq[i].second;
        assert(0 <= s2v[i] && s2v[i] <= 255);
      }else if ((i+1) == in_seq.size() && in_seq.back().second == 0){
        // ignores the right of the last factor
	vars.push_back(std::make_pair(cur-1, s2v[in_seq[i].first]));
	flag = true;
      }else {
        // a pair of factors
        s2v[i] = cur;
        vars.push_back(std::make_pair(s2v[in_seq[i].first], s2v[in_seq[i].second]));
        cur++;
      }
    }
    
    std::vector<unsigned int> root_curs(0);
    if(stacked_fids.empty()){
      for(unsigned int i = CHAR_SIZE; i < vars.size(); ++i)
	root_curs.push_back(i);

    }else{
      for(unsigned int i = 0; i < stacked_fids.size(); ++i)
	root_curs.push_back(s2v[stacked_fids[i]]);
    }


    if(root_curs.back() < CHAR_SIZE || flag){ //the last factor is a character
      if(!flag){

	vars.push_back(std::make_pair(root_curs[root_curs.size()-2], root_curs[root_curs.size()-1]));
      }
      ++cur;
      root_curs.push_back(vars.size()-1);
      root_curs.erase(root_curs.end() -2);
      root_curs.erase(root_curs.end() -2);
    }

      return root_curs;
  }

}
