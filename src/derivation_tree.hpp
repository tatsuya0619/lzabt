#pragma once
#include <utility>
#include <vector>
#include <climits>
#include <iostream>
#include <cassert>
#include <fstream>
#include <queue>
#include <istream>
#include <ostream>

namespace DTree{
  using id = unsigned int;
  class Nodes{
  protected:
    std::vector<unsigned int>nodes;
  public:

    ~Nodes(){};
    //return new assigned node id;
    unsigned int appendNode(unsigned int length, unsigned int left, unsigned int right){
      nodes.push_back(length);
      nodes.push_back(left);
      nodes.push_back(right);
      return nodes.size()/3-1;
    }
    //return new assigned node id;
    unsigned int appendNode(unsigned int ch){
      nodes.push_back(1);
      nodes.push_back(0);
      nodes.push_back(ch);
      return nodes.size()/3-1;
    }
    inline bool isChar(id i)const{return nodes[i*3+1] == 0;}
    inline unsigned int getChar(id i)const{return nodes[i*3+2];}
    inline unsigned int getLength(id i)const{return nodes[i*3];}
    inline id getLeft(id i)const{return nodes[i*3+1];}
    inline id getRight(id i)const{return nodes[i*3+2];}
     
    void printAllString(id i){
      if(this->isChar(i)){
	std::cout << (char)(this->getChar(i));
	return;
      }
      printAllString(getLeft(i));
      printAllString(getRight(i));
    }
  };
  
  using pair = std::pair<id, bool>;
  using traced_route = std::vector<pair>;
  
  class Tree{
  protected:
    static Nodes nodes;
    unsigned int length;
    id root;
    unsigned char first_char;


    inline void setFirstChar(unsigned int pos){this->first_char = getCharAt(pos);}
    inline char getLeftMostChar(traced_route *route)const{
      while(true){
	pair &last = route->back();
	if(nodes.isChar(last.first)){
	  return nodes.getChar(last.first);
	}
	route->push_back(std::make_pair(nodes.getLeft(last.first), true));
      }
    }
    inline char getRightMostChar(traced_route* route)const{
      while(true){
	pair &last = route->back();
	if(nodes.isChar(last.first)){
	  return nodes.getChar(last.first);
	}
	route->push_back(std::make_pair(nodes.getRight(last.first), false));
      }
    }
    
    void readyToReadWith(unsigned int, id, unsigned int)const;
    inline id getCharNodeAt(unsigned int pos)const{
	return getCharNodeAt(pos, getRoot());
    }
    inline id getCharNodeAt(unsigned int pos, id node_id)const{
      if(nodes.isChar(node_id))
	return node_id;
       if(pos < nodes.getLength(nodes.getLeft(node_id))){
	
	return getCharNodeAt(pos, nodes.getLeft(node_id));
      }else
	return getCharNodeAt(pos-nodes.getLength(nodes.getLeft(node_id)),
			     nodes.getRight(node_id));
    }
    char getNextChar(unsigned int, id)const;
    char getPrevChar(unsigned int, id)const;
  public:
    //tuple (traced node, traced direction)

    static std::vector<traced_route> traced_routes;
    static std::vector<unsigned int> poses; // represent the number of processed characters 
    
    Tree(char);
    //dummy, shouldn't be used
    Tree();
    Tree(Tree &, Tree&);
    static void createRoutesBuffer(unsigned int);

    ~Tree(){};
    inline id getRoot()const{return this->root;}    
    inline unsigned int getRootSize()const{return nodes.getLength(getRoot());}

    inline char getFirstChar()const{return first_char;}
    inline char getNextChar(unsigned int route_id)const{
      return getNextChar(route_id, 0);
    }

    inline char getPrevChar(unsigned int route_id)const{
      return getPrevChar(route_id, 0);
    }

    inline void readyToRead(unsigned int route_id, unsigned int pos)const{
      readyToReadWith(route_id, getRoot(), pos);
    }
    inline void setRange(unsigned int beg, unsigned int len, bool init=false){
      setBegin(beg);
      setLength(len, init);
    }

    inline unsigned int  size()const{return this->length;}

    inline void setBegin(unsigned int new_begin){
      setFirstChar(new_begin);
      return;
    }
    
    inline void setLength(unsigned int new_length, bool init=false){
      if(this->length == new_length && !init) return;
      this->length = new_length;
      return;
    }

    inline char getCharAt(unsigned int pos)const{return nodes.getChar(getCharNodeAt(pos));};
    void printString();
  };
}
