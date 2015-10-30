#ifndef MORPHTRIE_H_
#define MORPHTRIE_H_

#include <vector>
#include "Node.h"

template<class KeyClass, class ValueClass>
class MorphTrie
{
    public:
        MorphTrie() {}
        Node<KeyClass, ValueClass>* insert(const std::vector<KeyClass>& word, const ValueClass& value);
        Node<KeyClass, ValueClass>* getNode(const std::vector<KeyClass>& word);
    private:
        Node<KeyClass, ValueClass> root;
};

template<class KeyClass, class ValueClass>
Node<KeyClass, ValueClass>* MorphTrie<KeyClass, ValueClass>::insert(const std::vector<KeyClass>& word,
																	const ValueClass& value)
{
    Node<KeyClass, ValueClass>* cNode = &root;
    for (size_t i = 0; i < word.size(); ++i)
    {
        KeyClass cKey = word[i];
        cNode = cNode->addSubnode(cKey);
    }
    cNode->setValue(value);
    return cNode;
}

template<class KeyClass, class ValueClass>
Node<KeyClass, ValueClass>* MorphTrie<KeyClass, ValueClass>::getNode(const std::vector<KeyClass>& word)
{
    Node<KeyClass, ValueClass>* cNode = &root;
    for (size_t i = 0; i < word.size(); ++i)
    {
        KeyClass cKey = word[i];
        cNode = cNode->findSub(cKey);
        if (cNode == NULL)
        {
            return NULL;
        }
    }
    return cNode;
}

#endif /* end of include guard: MORPHTRIE_H_ */
