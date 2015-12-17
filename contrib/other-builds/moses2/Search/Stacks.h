/*
 * Stacks.h
 *
 *  Created on: 6 Nov 2015
 *      Author: hieu
 */

#pragma once

#include <vector>
#include <queue>
#include <unordered_map>
#include "Stack.h"

class Stkcmp1 {
public:
  bool operator()(const Bitmap* left, const Bitmap* right) const
  {
    return right->GetNumWordsCovered() < left->GetNumWordsCovered();
  }
};

class Stkcmp2 {
public:
  
  size_t operator()(const Bitmap* obj) const {
    return std::hash<int>()(obj->GetNumWordsCovered());
  }
  
  bool operator()(const Bitmap* a, const Bitmap* b) const {
    return a->GetNumWordsCovered() == b->GetNumWordsCovered();
  }
  
};


class Stacks {
	  friend std::ostream& operator<<(std::ostream &, const Stacks &);
public:
	Stacks();
	virtual ~Stacks();
  
  bool empty();
  
  Stack* getNextStack();
  
//
//	void Init(size_t numStacks);
//
//	size_t GetSize() const
//	{ return m_stacks.size(); }
//
    const Stack &Back() const
  { return *m_lastStack; }
//    { return *m_stacks.back(); }
//
//    Stack &operator[](size_t ind)
//    { return *m_stacks[ind]; }
//
//    void Delete(size_t ind) {
//    	delete m_stacks[ind];
//    	m_stacks[ind] = NULL;
//    }

	void Add(const Hypothesis *hypo, Recycler<Hypothesis*> &hypoRecycle);

protected:
//	std::vector<Stack*> m_stacks;
  
//  
//  static bool stkcmp1(const Bitmap* left, const Bitmap* right) {
//    return right->GetNumWordsCovered() < left->GetNumWordsCovered();
//  }
  
//  std::priority_queue<const Bitmap*, std::vector<const Bitmap*>, decltype(&stkcmp1) > m_queue(&stkcmp1);
  
  std::priority_queue<const Bitmap*, std::vector<const Bitmap*>, Stkcmp1 > m_queue;
  //std::unordered_map<const Bitmap*, Stack*, UnorderedComparer<Bitmap>, UnorderedComparer<Bitmap> > m_stacksMap;
  std::unordered_map<const Bitmap*, Stack*, Stkcmp2, Stkcmp2 > m_stacksMap;
  
  Stack* m_lastStack;
};

