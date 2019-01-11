// -*- coding: utf-8 -*-
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sys/stat.h>
#include <cassert>
#include <algorithm>
#include <numeric>
#include <ostream>
#include "string_tree.hpp"
using namespace STree;
struct stat_vec {
  double avg;
  unsigned int median;
  unsigned int max;
  unsigned int min;
};

void Stat::delNode(unsigned int fid, unsigned int depth){
  this->len_deleted_factors.push_back(depth);
  const unsigned int pos_current = this->len_factors.size();
  assert (this->pos_prev_used[fid] != UINT_MAX);
  const unsigned int pos_prev = this->pos_prev_used[fid];
  assert(pos_prev < pos_current);
  this->alive_time.push_back(pos_current - pos_prev);
  this->pos_prev_used[fid] = UINT_MAX;
}

void Node::info() const{
  std::cerr << "label=[" << label << "]"
            << " depth=" << depth
            << " id=" << id
            << " fid=" << fid
            << " children.size()=" << children.size();
  if (parent){
    std::cerr << " par_id=" << parent->id;
    std::cerr << " par_fid=" << parent->fid;
  }
  std::cerr << std::endl;
}

Stat::~Stat(){
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

void Tree::Init(){
  stat = NULL;
  this->num_nodes = this->num_fnodes = 0;
  // initially adds root node
  this->root = this->createNode(true, 0, ""); // sets root id and fid to 0 and 0 respectively.
  this->cur = this->root;
  this->lastFNode = this->root;
  this->firstFactor = NULL;
  this->len_matched = 0;
  this->read_len_tkb = 0;
}

Tree::Tree(std::istream & is,
           unsigned int codeSize) : is(is), codeSize(codeSize),
                                    in_str_size(0), out_factor_size(0){
  Init();
}

Tree::~Tree(){
  for(unsigned int i = 0; i < this->nodes.size(); i++){
    if (this->nodes[i]) delete this->nodes[i];
  }
  if (this->stat) delete this->stat;
}

Node * Tree::getChildBeginWith(const Node * node, char c) const{
  assert(node != NULL);
  std::vector<Node *>::const_iterator itr;
  for(itr = node->children.begin(); itr != node->children.end(); itr++){
    if (getInFirstChar(*itr) == c){
      return *itr;
    }
  }
  return NULL;
}

Node * Tree::getChild(const Node * node, std::string & label,
                      unsigned int label_begin, unsigned int label_len) const{
  assert(node != NULL);
  assert(label_begin + label_len <= label.size());
  Node * candidate_child = this->getChildBeginWith(node, label[label_begin]);
  if (candidate_child != NULL && canReachTo(candidate_child, label, label_begin, label_len)) return candidate_child;
  else return NULL;
}

char Tree::getInFirstChar(const Node * node) const{
  assert(node != NULL);
  return node->label[0];
}

Node * Tree::insertFactorNode(Node * node, std::string & label,
  unsigned int label_begin, unsigned int label_len){
  if (this->stat) {
    this->stat->len_lfactors.push_back(node->depth);
    this->stat->len_rfactors.push_back(label_len);
    this->stat->len_factors.push_back(node->depth + label_len);
  }

  assert(label_len > 0);
  // Note, $par may be a given node.
  Node * last_node        = findLastNNodeFrom(node, label, label_begin, label_len);
  assert(last_node != NULL);
  assert(last_node->depth >= node->depth);
  unsigned int matched_len = last_node->depth - node->depth; // the path length between a givne $node and $par.
  // if child is NULL, $par must be a leaf.
  Node * child = NULL;
  if (matched_len < label_len){
    child = this->getChildBeginWith(last_node, label[label_begin + matched_len]);
  }

  if(matched_len == label_len){
    // We have no remain string.
    // $par must be a branch node, and which is not a factor node.
    // we just change the node to a factor node;
    assert(!last_node->isFactor());
    this->change2FNode(last_node);
    return last_node;
  }else if (child == NULL){
    // There is no child which we can move with the remain string.
    // We only have to insert a new node from $par
    assert((last_node->depth - node->depth) < label_len);
    assert(!(last_node == this->root && label_len == 1));
    return this->addChild(last_node, true, label, label_begin + matched_len, label_len - matched_len);  
  }else{
    // We can move to the middle of an edge with the remain string.
    // We have to create a new branch node between $par and $child.
    // If the remain string is completely included in $str[$strbegin:$strend],
    // the new node is a factor node,
    // and other wise we have to create new branch node and insert a leaf.
    unsigned int lcp = this->LCPToNode(child, label, label_begin + matched_len, label_len - matched_len);
    bool isFactor = false;
    if (matched_len + lcp == label_len) isFactor = true;

    Node * branchNode = this->splitEdge(last_node, child, lcp);

    if (!isFactor){
      return this->addChild(branchNode, true, label, label_begin + matched_len + lcp,
                            label_len - (matched_len + lcp));
    }else{
      this->change2FNode(branchNode);
      return branchNode;
    }
  }
}

bool Tree::unlinkChild(Node * par, Node * child, Node * replace_node){
  std::vector<Node *>::iterator itr;
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


Node * Tree::splitEdge(Node * par, Node * child, unsigned int len){
  assert(child->parent == par);
  assert(child->edgeLen() > len);
  // deletes link from $par to $child
  this->unlinkChild(par, child);

  // inserts a branch node
  Node * middleNode = this->addChild(par, false, child->label, 0, len);
  this->addChild(middleNode, child);

  // updates the child's edge
  child->label = child->label.substr(len);
  return middleNode;
}

unsigned int Tree::findLastNodeIdFrom(
  const Node * cur, bool findFNode, std::string & label,
  unsigned int label_begin, unsigned int label_len) const{
  assert(cur != NULL);
  const Node * lastFNode = NULL;
  if (findFNode && cur->isFactor()) lastFNode = cur;
  unsigned int i = 0;
  while(cur && i < label_len){ // 注意
    const Node * child = getChild(cur, label, label_begin + i, label_len - i);
    // not founds the child which matches $str[i..]
    if (child == NULL || (i + child->edgeLen()) > label_len) break;
    cur = child;
    i += child->edgeLen();
    if (cur->isFactor()) lastFNode = cur;
  }
  assert(cur != NULL);
  if (findFNode) return lastFNode->id;
  else return cur->id;
}

unsigned int Tree::LCPToNode(const Node * node, std::string & label, unsigned int label_begin, unsigned int label_len) const{
  if (label_begin + label_len > label.size()) return 0;
  assert(node != NULL);
  unsigned int lcp = 0;

  if (node->depth == 1){
    if (getInFirstChar(node) == label[label_begin]) return 1;
    else return 0;
  }
  for(; (lcp < node->edgeLen()) && (lcp < label_len); lcp++){
    if (node->label[lcp] != label[label_begin+lcp]) return lcp;
  }
  return lcp;
}

void Tree::toString(const Node * node, std::string & s) const{
  if (node == NULL) return;
  if (node == this->root) return;
  this->toString(node->parent, s);
  s.append(node->label);
}

inline void Tree::info(){
  std::cerr << "** TREE INFO:"
            << " num_nodes=" << this->num_nodes
            << " num_fnodes=" << this->num_fnodes
            << std::endl;
}
void Tree::showAllNode() const{
  std::string tmp = "";
  showAllNode(this->root, tmp);
}
void Tree::showAllNode(const Node * node, std::string & s) const{
  assert(node != this->getParent(node));
  this->verify(node);
  std::string new_os = s;
  if (node != this->root){
    new_os += node->label;
  }
  std::cerr << "[" + new_os+ "] ";
  node->info();
  for (unsigned int i = 0; i < node->children.size(); i++){
    this->showAllNode(node->children[i], new_os);
  }
}

bool Tree::isValid() const{
  return isValid(this->root);
}

bool Tree::isValid(const Node * node) const{
  assert(node->parent != node);
  bool res = true;
  for (unsigned int i = 0; i < node->children.size(); i++){
    res &= this->isValid(node->children[i]);
  }
  return res;
}
