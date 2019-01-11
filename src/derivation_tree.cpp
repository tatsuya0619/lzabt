#include "derivation_tree.hpp"
using namespace DTree;

std::vector<traced_route> Tree::traced_routes;
std::vector<unsigned int> Tree::poses; // true if the first character has already been read
Nodes Tree::nodes;

Tree::Tree(char c){
  this->root = nodes.appendNode(c);
}

Tree::Tree(){
  this->root = nodes.appendNode(0, 0, 0);
}

Tree::Tree(Tree &left, Tree &right){
  this->root = nodes.appendNode(nodes.getLength(left.getRoot())+nodes.getLength(right.getRoot()),
			left.getRoot(), right.getRoot());
}

char Tree::getNextChar(unsigned int route_id, id next_root)const{
  ++poses[route_id];
  if(poses[route_id] == 1){
    return getFirstChar();
  }

  traced_route *route = &traced_routes[route_id];
  while(route->size() > 1){
    route->pop_back();
    pair &last = route->back();
    bool isLeft = last.second;
    if(isLeft){
      last.second = false;
      route->push_back(std::make_pair(nodes.getRight(last.first), true));
      return getLeftMostChar(route);
    }
  }
  assert(next_root != 0);
  route->clear();
  route->push_back(std::make_pair(next_root, true));
  return getLeftMostChar(route);
}

char Tree::getPrevChar(unsigned int route_id, id prev_root)const{
  --poses[route_id];

  traced_route *route = &traced_routes[route_id];
  if(poses[route_id] == 0){
    return getFirstChar();
  }
  while(route->size() > 1){
    route->pop_back();
    pair &last = route->back();
    bool isLeft = last.second;
    if(!isLeft){
      last.second = true;
      route->push_back(std::make_pair(nodes.getLeft(last.first), false));
      return getRightMostChar(route);
    }
  }
  assert(route->size() == 1);
  route->clear();
  route->push_back(std::make_pair(prev_root, false));
  return getRightMostChar(route);
}

void Tree::createRoutesBuffer(unsigned int num){
  traced_routes.resize(num);
  poses.resize(num);
}

void Tree::readyToReadWith(unsigned int route_id, id root_node, unsigned int first_pos)const{
  assert(root_node != 0);
  traced_route *route = &traced_routes[route_id];
  route->clear();
  poses[route_id] = 0;

  if(this->size() == 1) return; //if this->size() == 1, we don't have to prepare route.

  id cur_node = root_node;
  unsigned int pos = first_pos;
  while(true){
    route->push_back(std::make_pair(cur_node, false)); // this false is temporary
    if(nodes.isChar(cur_node)){
      return;
    }
    if(pos < nodes.getLength(nodes.getLeft(cur_node))){
      route->back().second = true;
      cur_node = nodes.getLeft(cur_node);
    }else{
      pos -=nodes.getLength(nodes.getLeft(cur_node));
      route->back().second = false;
      cur_node = nodes.getRight(cur_node);
    }
  }
}

void Tree::printString(){
  traced_route tmp(traced_routes[0]);
  bool tmp_pos = poses[0];
  std::cout << "root represents: \"";
  nodes.printAllString(this->getRoot());
  std::cout <<"\"" <<std::endl;

  traced_routes[0] = tmp;
  poses[0] = tmp_pos;
  std::cout << ", length =" << this->length
	    << ", first char =" << this->getFirstChar()
	    << ", root_size = " << this->getRootSize()
	    << ", pos = " << poses[0]
	    <<std::endl;
}
