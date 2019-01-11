// -*- coding: utf-8 -*-
#pragma once

#include <string>
#include <vector>
#include <climits>
#include <iostream>
#include <cassert>
#include <fstream>
#include <queue>
#include <istream>
#include <ostream>
#include "common.hpp"
#include "derivation_tree.hpp"

namespace STreeComp{
  struct stat_vec {
    double avg;
    unsigned int median;
    unsigned int max;
    unsigned int min;
  };  
  const static unsigned int NNODE_FID = UINT_MAX; // default fid for normal nodes

  class Stat{
  public:
    std::vector<unsigned int> len_factors;
    std::vector<unsigned int> len_lfactors;
    std::vector<unsigned int> len_rfactors;
    std::vector<unsigned int> len_deleted_factors;
    std::vector<unsigned int> alive_time;
    std::vector<unsigned int> pos_prev_used;
    Stat(unsigned int max_fnode = 0) : len_factors(0), len_lfactors(0), len_rfactors(0),
                                       len_deleted_factors(0), alive_time(0),
                                       pos_prev_used(max_fnode, 0) {};
    void delNode(unsigned int fid, unsigned int depth){
      this->len_deleted_factors.push_back(depth);
      const unsigned int pos_current = this->len_factors.size();
      assert (this->pos_prev_used[fid] != UINT_MAX);
      const unsigned int pos_prev = this->pos_prev_used[fid];
      assert(pos_prev < pos_current);
      this->alive_time.push_back(pos_current - pos_prev);
      this->pos_prev_used[fid] = UINT_MAX;
    }
    
    ~Stat(){
      // computes alive time for each of the rest factors
      for (unsigned int i = 0; i < this->pos_prev_used.size(); i++){
	unsigned int pos_prev = this->pos_prev_used[i];
	if (pos_prev != UINT_MAX) {
	  assert(len_factors.size() > pos_prev);
	  alive_time.push_back(len_factors.size() - pos_prev);
	}
      }

      //unsigned int num_stat = 5;
      std::vector<std::string> stat_names;
      stat_names.push_back("len_factors");
      stat_names.push_back("len_lfactors");
      stat_names.push_back("len_rfactors");
      stat_names.push_back("len_deleted_factors");
      stat_names.push_back("alive_time");
      std::vector<std::vector<unsigned int> *> vecs;
      vecs.push_back(&len_factors);
      vecs.push_back(&len_lfactors);
      vecs.push_back(&len_rfactors);
      vecs.push_back(&len_deleted_factors);
      vecs.push_back(&alive_time);

      std::cerr << "len_factors.size()=" << len_factors.size()
		<< " len_lfactors.size()=" << len_lfactors.size()
		<< " alive_time.size()=" << alive_time.size()
		<< std::endl;
    }
  };


  class Node{
  public:
    unsigned int depth; // the depth of the root node is 0.
    unsigned int id; // unique id for nodes.
    DTree::Tree path_label;
    Node * parent;
    std::vector<Node *> children;

    Node(unsigned int depth, unsigned int id, DTree::Tree path_label):
      depth(depth),
      id(id),
      path_label(path_label),
      parent(NULL){

    };


    ~Node(){
    };

    inline unsigned int edgeLen() const{return path_label.size();};
    inline unsigned int getFid() const{return path_label.getRoot();}
    inline bool isFactor() const{return this->path_label.getRootSize() == this->depth;}
    void info() const{ // for debug
      std::cerr << " depth=" << depth
		<< " id=" << id
		<< " fid=" << getFid()
		<< " children.size()=" << children.size();
      if (parent){
	std::cerr << " par_id=" << parent->id;
	std::cerr << " par_fid=" << parent->getFid();
      }
      std::cerr << std::endl;
    }
  };


  
  class Tree{
  protected:
    unsigned int num_nodes;
    unsigned int num_fnodes;
    Stat * stat;
    std::istream & is; //$is should be protedted   
    unsigned int codeSize;
    unsigned int in_str_size;
    unsigned int out_factor_size;
    
  public:

    Node* root;
    std::vector<Node *> nodes;
    Node* cur;
    unsigned int len_matched;
    Node * lastFNode;
    double start_time;

    Tree(std::istream & is) : is(is),
			      in_str_size(0), out_factor_size(0){
      Init();
    }
    void Init(){
      stat = NULL;
      this->num_nodes = this->num_fnodes = 0;
      // initially adds root node
      DTree::Tree dtree = DTree::Tree();
      dtree.setRange(0, 0, true);
      this->root = this->createNode(true, 0, dtree); // sets root id and fid to 0 and 0 respectively.
      this->cur = this->root;
      this->lastFNode = this->root;
      this->len_matched = 0;
    }

    inline bool atNode() const{
      return len_matched == cur->path_label.size();
    }
    inline bool atNode(const Node * node, unsigned int len_matched) const{
      assert(node != NULL);
      return len_matched == node->path_label.size();
    }

    inline bool empty(){ return this->is?true:false;}
     ~Tree(){
      for(unsigned int i = 0; i < this->nodes.size(); i++){
	if (this->nodes[i]) delete this->nodes[i];
      }

      if (this->stat) delete this->stat;
    }


    inline Node * createNode(bool isFactor,  unsigned int depth, DTree::Tree path_label){
      Node * node;
      if (isFactor){
        node = new Node (depth, this->getNewId(), path_label);
        registerFNode(node);
      }else{
        node = new Node (depth, this->getNewId(), path_label);

      }
      
      registerNode(node);
      return node;
    }

    inline virtual void registerFNode(Node * node){
      this->num_fnodes++;
    };
    inline virtual void registerNode(Node * node){
      this->nodes.push_back(node);
      this->num_nodes++;
    };
    /** 
     * changes the node to a factor node.
     * 
     * @param node 
     */
    inline void change2FNode(Node * node, DTree::Tree new_label){
      //node->getFid() = this->getNewFId();
      registerFNode(node);
      new_label.setRange(new_label.getRootSize() - node->path_label.size(), node->path_label.size());
      node->path_label = new_label;
    }

    inline virtual unsigned int getNewId(){
      return getNumNodes();
    };

    
    inline unsigned int getNumNodes(){return num_nodes;};
    inline unsigned int getNumFNodes(){return num_fnodes;};
    inline Node * getNode(unsigned int node_id) const {
      assert(this->nodes[node_id] != NULL);
      return this->nodes[node_id];
    };

    /** 
     * gets the child node whose in-edge label begins with  $c
     * 
     * @param node 
     * @param c 
     * 
     * @return 
     */
    Node * getChildBeginWith(Node * node, char c){
      assert(node != NULL);
      typename std::vector<Node *>::iterator itr;
      for(itr = node->children.begin(); itr != node->children.end(); itr++){
	if ((*itr)->path_label.getFirstChar() == c){
	  return *itr;
	}
      }
      return NULL;
    }

    /** 
     * gets the child whose in-edge label matches with $str[$strbegin..]
     * 
     * @param node 
     * @param strbegin 
     * 
     * @return 
     */
    Node * getChild(Node * node, DTree::Tree path_label){
      assert(node != NULL);
      Node * candidate_child = this->getChildBeginWith(node, path_label.getNextChar(1));
      path_label.getPrevChar(1);
      if (candidate_child != NULL && canReachTo(candidate_child, path_label, node->depth)) return candidate_child;
      else return NULL;
    }

    /** 
     * checks whether the concatenated string of in-edge labeles
     * from $from to $to matches with $str[$strbegin..]
     *
     * @param node 
     * @param strbegin 
     * 
     * @return 
     */
    inline bool canReachTo(Node * node, DTree::Tree label, unsigned int begin_pos){
      assert(node != NULL);
      unsigned int tmp = node->path_label.size() == LCPToNode(node, label, begin_pos);
      return tmp;
    };


    /** 
     * gets the first character of in-edge label
     * 
     * @param node 
     * 
     * @return 
     */
    char getInFirstChar(Node * node);
    inline Node * getParent(const Node * node) const {return node->parent;};
    inline void addChild(Node * parent, Node * child){
      parent->children.push_back(child);
      child->parent = parent;
    }

    /** 
     * creates and adds a new node whose parent is $parent and
     * in-edge label is $str[$in_edge_begin..$in_edge_begin + $in_edge_len - 1]
     * 
     * @param parent 
     * @param isFactor 
     * @param in_edge_begin 
     * @param in_edge_len 
     * 
     * @return 
     */

    inline Node * addChild(Node * parent, bool isFactor, DTree::Tree path_label){
      assert(parent != NULL);
      unsigned int depth = parent->depth + path_label.size();
      Node * child = this->createNode(isFactor, depth, path_label);
      this->addChild(parent, child);
      return child;
    }

    /**
     * inserts a new factor node that can be reached from $node with
     * $str[$strbegin..$strbegin + $len-1]
     * 
     * @param node 
     * @param strbegin 
     * @param len
     */

    Node *insertFactorNode(Node * node, DTree::Tree path_label){
      if (this->stat) {
	this->stat->len_lfactors.push_back(node->depth);
	this->stat->len_rfactors.push_back(path_label.size());
	this->stat->len_factors.push_back(node->depth + path_label.size());
      }

      assert(path_label.size() > 0);
      // Note, $par may be a given node.
      Node * last_node        = findLastNNodeFrom(node, path_label);
      assert(last_node != NULL);
      assert(last_node->depth >= node->depth);
      unsigned int matched_len = last_node->depth - node->depth; // the path length between a givne $node and $par.
      // if child is NULL, $par must be a leaf.
      Node * child = NULL;
      
      if (matched_len < path_label.size()){
	child = this->getChildBeginWith(last_node, path_label.getCharAt(last_node->depth));
      }

      if(matched_len == path_label.size()){
	// We have no remain string.
	// $par must be a branch node, and which is not a factor node.
	// we just change the node to a factor node;
	assert(!last_node->isFactor());
	this->change2FNode(last_node, path_label);

	return last_node;
      }else if (child == NULL){
	// There is no child which we can move with the remain string.
	// We only have to insert a new node from $par
	assert((last_node->depth - node->depth) < path_label.size());
	assert(!(last_node == this->root && path_label.size() == 1));
	
	path_label.setRange(last_node->depth, path_label.size() - matched_len);
	return this->addChild(last_node, true, path_label);
      }else{
	// We can move to the middle of an edge with the remain string.
	// We have to create a new branch node between $par and $child.
	// If the remain string is completely included in $str[$strbegin:$strend],
	// the new node is a factor node,
	// and other wise we have to create new branch node and insert a leaf.

	path_label.setRange(last_node->depth, path_label.size() - matched_len);
	path_label.readyToRead(1, last_node->depth);
	unsigned int lcp = this->LCPToNode(child, path_label, last_node->depth);
	assert(child->edgeLen() > lcp);
	bool isFactor = false;
	if (lcp == path_label.size()) isFactor = true;
	DTree::Tree branch_label = path_label;
	branch_label.setLength(lcp);
	Node * branchNode = this->splitEdge(last_node, child, branch_label);
	if (!isFactor){
	  path_label.setRange(last_node->depth + lcp, path_label.size() - lcp);
	  return this->addChild(branchNode, true, path_label);
	}else{
	  //insert Fnode at the middle of edge.
	  //branchNode was created at previous step.
	  this->change2FNode(branchNode, path_label);
	  return branchNode;
	}
      }
    }


    /** 
     * inserts a new factor node that corresonds a single character
     * 
     * @param c 
     * 
     * @return
     */
    inline virtual Node * insertCharNode(char c){
      DTree::Tree new_path_label = DTree::Tree(c);
      new_path_label.setRange(0, 1, true);
      if (this->stat) {
        this->stat->len_factors.push_back(1);
      }

      return this->addChild(this->root, true, new_path_label);
    }

    /**
     * unlinks from $par to $child
     * replaces $child with $replace_node if $replace_node != NULL
     * 
     * @param par 
     * @param child 
     * @param replace_node 
     * 
     * @return 
     */
    bool unlinkChild(Node * par, Node * child, Node * replace_node = NULL){
      typename std::vector<Node *>::iterator itr;
      for(itr = par->children.begin(); itr != par->children.end(); itr++){
	if (*itr == child){
	  if (replace_node){
	    (*itr) = replace_node;
	    replace_node->parent = par;
	  }else{
	    par->children.erase(itr);
	  }
	  child->parent = NULL;
	  return true;
	}
      }
      return false;
    }

    /** 
     * splits the edge between $par to $child at $len
     * inserts and returns a new branch node
     * 
     * @param par
     * @param child
     * @param len
     * 
     * @return
     */
    Node * splitEdge(Node * par, Node * child, DTree::Tree label){
      assert(child->parent == par);
      // deletes link from $par to $child
      this->unlinkChild(par, child);

      // inserts a branch node
      Node * middleNode = this->addChild(par, false, label);
      // updates the child's edge
      child->path_label.setRange(middleNode->depth,
				 child->path_label.size()-label.size()); 
      this->addChild(middleNode, child);

      return middleNode;
    }

    /**
     * finds the deepest node which can be reached from $node with
     * $str[$strbegin..$strbegin+$len-1]
     * if $findFNode is true, finds a factor node only
     *
     * @param node 
     * @param strbegin 
     * @param findFNode
     * @param len
     * 
     * @return 
     */
    unsigned int findLastNodeIdFrom(Node * cur, bool findFNode, DTree::Tree path_label){
      assert(cur != NULL);
      const Node * lastFNode = NULL;
      if (findFNode && cur->isFactor()) lastFNode = cur;
      unsigned int i = 0;
      DTree::Tree sub_path_label = path_label;
      sub_path_label.readyToRead(1, cur->depth);
      while(cur && i < path_label.size()){ // 注意
	sub_path_label.setLength(path_label.size() -i);
	Node * child = getChild(cur, sub_path_label);
	// not founds the child which matches $str[i..]
	if (child == NULL || (i + child->edgeLen()) > path_label.size()) break;
	cur = child;
	i += child->edgeLen();
	if (cur->isFactor()) lastFNode = cur;
      }
      assert(cur != NULL);
      //delete sub_path_label;
      if (findFNode) return lastFNode->id;
      else return cur->id;
    }

    
    inline Node * findLastNodeFrom(
				   bool findFNode, Node * node, DTree::Tree path_label){
      return this->getNode(this->findLastNodeIdFrom(node, findFNode, path_label));
    }
    inline Node * findLastNNodeFrom(Node * node, DTree::Tree path_label){
      return findLastNodeFrom(false, node, path_label);
    }

    /** 
     * 
     * sets the state at the root node
     * 
     *@return 
     */
    void inline initState() {
      this->cur = this->root;
      this->lastFNode = this->root;
      this->len_matched = 0;
    }

    inline Node * moveWith(Node * node, unsigned int len_matched, const char c){
      if (this->atNode(node, len_matched)) {
	return this->getChildBeginWith(node, c);
      }
      char next_char= node->path_label.getNextChar(0);
      if(next_char == c){
        return node;
      }else {
        return NULL;
      }
    }
    
    inline bool moveIfPossible(const char c){
      Node * res = moveWith(this->cur, this->len_matched, c);

      if (res != NULL) {
        if (this->cur == res) {
          this->len_matched++;
        }else {
	  //when next child is found
	  res->path_label.readyToRead(0, cur->depth);
	  res->path_label.getNextChar(0);
          this->cur = res;
          this->len_matched = 1;
        }
        return true;
      }
      this->cur = this->root;
      this->len_matched = 0;
      return false;
    }

    /** 
     * return NULL if we can move with $new_char,
     * otherwise return the last factor node.
     * 
     * @param new_char 
     * 
     * @return 
     */
    Node * findLastFNodeWithChar(const unsigned char new_char){
      if (!moveIfPossible(new_char)){
        // we couldn't move with $new_char
        if (this->lastFNode == this->root){
          return this->insertCharNode(new_char);
        }else{
          Node * tmp = this->lastFNode;
          this->lastFNode = this->root;
          return tmp;
        }
      }else if (this->atNode(this->cur, this->len_matched) &&
                this->cur->isFactor()){
        // we could move with $new_char, and be at a factor node.
        this->lastFNode = this->cur;
      }
      // we could move
      return NULL;
    }

    /** 
     * finds the last factor node with $remain_string and $is.
     * Note that $remain_string contains the decompressed string of the obtained node.
     * 
     * @param is 
     * @param remain_string 
     * 
     * @return 
     */
    
    Node * findLastFNode(std::string & remain_string, unsigned int limit = UINT_MAX){
      assert(is || !remain_string.empty());
      char new_char;
      unsigned int length = 0;

      // reads $remain_string first.
      for (unsigned int i = 0; i < remain_string.size() && length < limit; ++i){
        Node * res = findLastFNodeWithChar(remain_string[i]);
	++length;
        if (res != NULL){
          // found the last factor node
          this->initState();
          return res;
        }
      }
      
      // reads $is
      for(;  length < limit && is && is.get(new_char);) {
	remain_string.push_back(new_char);
	Node * res = findLastFNodeWithChar(new_char);
	++length;
        if (res != NULL){
          this->initState();
          return res;
        }
      }
      Node * res = this->lastFNode;
      this->initState();
      return res;
    }

    /**
     * computes the longest common prefix between $str[$strbegin..$strbegin + $len - 1] and the label strings on the paths from $node
     * NOTE if @len = UINT_MAX, we recognize @len is the length of  str.
     *
     * @param node
     * @param strbegin
     * @param len
     *
     * @return
     */


    unsigned int LCPToNode(Node * node, DTree::Tree label, unsigned int begin_pos){
      assert(node != NULL);
      unsigned int lcp = 0;
      node->path_label.readyToRead(0, begin_pos);
      for(; (lcp < node->edgeLen()) && (lcp < label.size()); lcp++){
	unsigned int tmp1= node->path_label.getNextChar(0);
	unsigned int tmp2 = label.getNextChar(1);
	if (tmp1 != tmp2){
	  label.getPrevChar(1);
	  return lcp;
	}
      }
      return lcp;
    }

    void toString(const Node * node, std::string & s) const;
    void outputString(const Node * node, std::ostream & os) const;

    inline void info();
    inline void verify(const Node * node) const{
      assert(this->getNode(node->id) == node);
    }

    void showAllNode() const;
    void showAllNode(const Node * node, std::string & s) const;
    bool isValid() const;
    bool isValid(const Node * node) const;
    void checkAllNodes(){
      for(unsigned int i = 1; i < this->nodes.size(); i++){
	if(this->nodes[i]->path_label.size() == 0){
	  std::cout << "path_label.size() is zero!!" << std::endl;
	  std:: cout <<"fid = " << this->nodes[i]->getFid() << std::endl;
	  exit(-1);
	}
      }
    }
  };
}

