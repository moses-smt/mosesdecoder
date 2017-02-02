/**
 * IndexedPhrasesPair - implementation of a single phrase pair source and target
 * phrases represented by numeric indices.
 *
 * (C) Ceslav Przywara, UFAL MFF UK, 2011
 *
 * $Id$
 *
 * TODO:
 * - current unordered_map implementation is terribly slow. More sophisticated
 * design of hash function should help.
 */

#ifndef INDEXEDPHRASESPAIR_H
#define	INDEXEDPHRASESPAIR_H

#include <vector>
#include <new>
#include <algorithm>
#include <string.h>
#ifdef USE_UNORDERED_MAP
#include <tr1/functional_hash.h>
#endif

// Forward declaration because of friend comparison operator declaration below.
template<class OrientationIndexType, class TokenIndexType> class IndexedPhrasesPair;

#ifdef USE_UNORDERED_MAP
template<class OrientationIndexType, class TokenIndexType>
class IndexedPhrasePairHasher;

// Comparison operator (is going to be declared as friend).
template<class OrientationIndexType, class TokenIndexType>
bool operator== (const IndexedPhrasesPair<OrientationIndexType, TokenIndexType>& lhs, const IndexedPhrasesPair<OrientationIndexType, TokenIndexType>& rhs);
#else
// Comparison operator (is going to be declared as friend).
template<class OrientationIndexType, class TokenIndexType>
bool operator< (const IndexedPhrasesPair<OrientationIndexType, TokenIndexType>& lhs, const IndexedPhrasesPair<OrientationIndexType, TokenIndexType>& rhs);
#endif

/**
 * Structure capable of holding a phrase pair consisting of:
 * a) source phrase
 * b) target phrase
 * c) word alignment
 * d) orientation info
 * @param OrientationIndexType
 * @param TokenIndexType - datatype for token indices.
 */
template<class OrientationIndexType = unsigned char, class TokenIndexType = unsigned int>
class IndexedPhrasesPair {
public:

    typedef TokenIndexType token_index_t;

    typedef OrientationIndexType orientation_info_index_t;

    typedef std::vector<TokenIndexType> phrase_t;

    // A single alignment point.
    typedef unsigned char alignment_point_t;

    // A single pair of alignments points.
    typedef std::pair<alignment_point_t, alignment_point_t> alignment_pair_t;

    // A single phrase alignment (eg. 0-0 0-1 1-2)
    typedef std::vector<alignment_pair_t> alignment_t;

#ifdef USE_UNORDERED_MAP
    // Unordered map requires hashing functor object.
    typedef IndexedPhrasePairHasher<OrientationIndexType, TokenIndexType> Hash;
#endif

private:

    /** @var Source and target phrase as array of respective token indices */
    token_index_t* _data;

    /** @var A single phrase alignment stored in array */
    alignment_point_t* _alignment;

    /** @var Index of orientation info string */
    orientation_info_index_t _orientationInfoIndex;

    alignment_point_t _srcPhraseLength;

    alignment_point_t _tgtPhraseLength;

    alignment_point_t _alignmentLength;

public:

    IndexedPhrasesPair(void): _data(NULL), _alignment(NULL), _orientationInfoIndex(0), _srcPhraseLength(0), _tgtPhraseLength(0), _alignmentLength(0) {}

    IndexedPhrasesPair(const phrase_t& srcPhrase, const phrase_t& tgtPhrase, orientation_info_index_t orientationInfo, const alignment_t& alignment);

    IndexedPhrasesPair(const IndexedPhrasesPair<OrientationIndexType, TokenIndexType>& copy);

    ~IndexedPhrasesPair(void);

    IndexedPhrasesPair<OrientationIndexType, TokenIndexType>& operator=(const IndexedPhrasesPair<OrientationIndexType, TokenIndexType>& other);

    phrase_t srcPhrase(void) const { return phrase_t(_data, _data + _srcPhraseLength); }

    phrase_t tgtPhrase(void) const { return phrase_t(_data + _srcPhraseLength, _data + _srcPhraseLength + _tgtPhraseLength); }

    orientation_info_index_t orientationInfo(void) const { return _orientationInfoIndex; }

    alignment_t alignment(void) const;

    const alignment_point_t * alignmentData(void) const { return _alignment; }

    alignment_point_t alignmentLength(void) const { return _alignmentLength; }

#ifdef USE_UNORDERED_MAP
    friend class IndexedPhrasePairHasher<OrientationIndexType, TokenIndexType>;
    friend bool operator== <> (const IndexedPhrasesPair<OrientationIndexType, TokenIndexType>& lhs, const IndexedPhrasesPair<OrientationIndexType, TokenIndexType>& rhs);
#else
    friend bool operator< <> (const IndexedPhrasesPair<OrientationIndexType, TokenIndexType>& lhs, const IndexedPhrasesPair<OrientationIndexType, TokenIndexType>& rhs);
#endif
};


////////////////////////////////////////////////////////////////////////////////
//// IndexedPhrasesPair IMPLEMENTATION /////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

template<class OrientationIndexType, class TokenIndexType>
IndexedPhrasesPair<OrientationIndexType, TokenIndexType>::IndexedPhrasesPair(const phrase_t& srcPhrase, const phrase_t& tgtPhrase, orientation_info_index_t orientationInfo, const alignment_t& alignment):
    _data(NULL), _alignment(NULL), _orientationInfoIndex(orientationInfo), _srcPhraseLength(static_cast<alignment_point_t>(srcPhrase.size())), _tgtPhraseLength(static_cast<alignment_point_t>(tgtPhrase.size())), _alignmentLength(alignment.size()) {

    // Save alignment.
    _alignment = new alignment_point_t[2 * _alignmentLength]; // Note: *2 for each pair.
    for ( size_t i = 0; i < alignment.size(); ++i ) {
        _alignment[i*2] = alignment[i].first;
        _alignment[i*2 + 1] = alignment[i].second;
    }

    // Save data.
    _data = new token_index_t[_srcPhraseLength + _tgtPhraseLength];
    std::copy(srcPhrase.begin(), srcPhrase.end(), _data);
    std::copy(tgtPhrase.begin(), tgtPhrase.end(), _data + _srcPhraseLength);

}

template<class OrientationIndexType, class TokenIndexType>
IndexedPhrasesPair<OrientationIndexType, TokenIndexType>::IndexedPhrasesPair(const IndexedPhrasesPair<OrientationIndexType, TokenIndexType>& copy):
    _data(NULL), _alignment(NULL), _orientationInfoIndex(copy._orientationInfoIndex), _srcPhraseLength(copy._srcPhraseLength), _tgtPhraseLength(copy._tgtPhraseLength), _alignmentLength(copy._alignmentLength) {

    // Copy alignment.
    // alignment_point_t alignmentLength = std::max(_srcPhraseLength, _tgtPhraseLength);
    _alignment = new alignment_point_t[2 * _alignmentLength]; // Note: *2 for each pair.
    memcpy(_alignment, copy._alignment, _alignmentLength * 2 * sizeof(alignment_point_t));

    // Copy data.
    _data = new token_index_t[_srcPhraseLength + _tgtPhraseLength];
    std::copy(copy._data, copy._data + _srcPhraseLength + _tgtPhraseLength, _data);
}

template<class OrientationIndexType, class TokenIndexType>
IndexedPhrasesPair<OrientationIndexType, TokenIndexType>::~IndexedPhrasesPair(void) {
    delete[] _alignment;
    delete[] _data;
}

template<class OrientationIndexType, class TokenIndexType>
IndexedPhrasesPair<OrientationIndexType, TokenIndexType>& IndexedPhrasesPair<OrientationIndexType, TokenIndexType>::operator=(const IndexedPhrasesPair<OrientationIndexType, TokenIndexType>& other) {

    if ( this != &other ) {

        // Copy alignment.
        _alignmentLength = other._alignmentLength;
        // alignment_point_t alignmentLength = std::max(_srcPhraseLength, _tgtPhraseLength);
        alignment_point_t * alignment = new alignment_point_t[2 * _alignmentLength]; // Note: *2 for each pair.
        memcpy(alignment, other._alignment, _alignmentLength * 2 * sizeof(alignment_point_t));
        if ( _alignment != NULL ) {
            delete[] _alignment; // !
        }
        _alignment = alignment; // !

        // Copy data.
        _srcPhraseLength = other._srcPhraseLength;
        _tgtPhraseLength = other._tgtPhraseLength;
        token_index_t * data = new token_index_t[_srcPhraseLength + _tgtPhraseLength];
        std::copy(other._data, other._data + _srcPhraseLength + _tgtPhraseLength, data);
        if ( _data != NULL ) {
            delete[] _data; // !
        }
        _data = data; // !

        //
        _orientationInfoIndex = other._orientationInfoIndex;

    }

    return *this;
}

template<class OrientationIndexType, class TokenIndexType>
typename IndexedPhrasesPair<OrientationIndexType, TokenIndexType>::alignment_t IndexedPhrasesPair<OrientationIndexType, TokenIndexType>::alignment(void) const {
    alignment_t a;

    //alignment_point_t alignmentLength = std::max(_srcPhraseLength, _tgtPhraseLength);
    for ( size_t i = 0; i < _alignmentLength; ++i ) {
        a.push_back(alignment_pair_t(_alignment[2*i], _alignment[2*i + 1]));
    }

    return a;
}

#ifdef USE_UNORDERED_MAP
template<class OrientationIndexType, class TokenIndexType>
class IndexedPhrasePairHasher: public std::unary_function<IndexedPhrasesPair<OrientationIndexType, TokenIndexType>, size_t> {

    typedef typename IndexedPhrasesPair<OrientationIndexType, TokenIndexType>::alignment_point_t alignment_point_t;

    std::tr1::hash<TokenIndexType> _hash;

public:
    size_t operator()(const IndexedPhrasesPair<OrientationIndexType, TokenIndexType>& phrasePair) const {
        size_t hash = 0;
        for ( alignment_point_t i = 0; i < phrasePair._srcPhraseLength + phrasePair._tgtPhraseLength; ++i ) {
            hash ^= _hash(phrasePair._data[i]);
        }
        return hash;
    }
};

template<class OrientationIndexType, class TokenIndexType>
bool operator== (const IndexedPhrasesPair<OrientationIndexType, TokenIndexType>& lhs, const IndexedPhrasesPair<OrientationIndexType, TokenIndexType>& rhs) {

    typedef typename IndexedPhrasesPair<OrientationIndexType, TokenIndexType>::alignment_point_t alignment_point_t;
    typedef typename IndexedPhrasesPair<OrientationIndexType, TokenIndexType>::token_index_t string_index_t;

    // Alignments comparable?
    if ( lhs._alignmentLength != rhs._alignmentLength ) {
        return false;
    }

    // Same alignment length -> compare alignments.
    int cmp = memcmp(lhs._alignment, rhs._alignment, lhs._alignmentLength * 2 * sizeof(alignment_point_t));

    if ( cmp != 0 ) {
        // Alignments differ.
        return false;
    }

    // Alignments are equal, compare phrases (data).

    if ( lhs._srcPhraseLength != rhs._srcPhraseLength ) {
        // Source phrase lengths differs.
        return false;
    }

    if ( lhs._tgtPhraseLength != rhs._tgtPhraseLength ) {
        // Target phrase lengths differs.
        return false;
    }

    // Phrases have matching lengths, compare the data in the end:
    cmp = memcmp(lhs._data, rhs._data, (lhs._srcPhraseLength + lhs._tgtPhraseLength) * sizeof(string_index_t));

    if ( cmp != 0 ) {
        // Data differ.
        return false;
    }

    // Compare orientation info in the end.
    return lhs._orientationInfoIndex == rhs._orientationInfoIndex;
}
#else
template<class OrientationIndexType, class StringIndexType>
bool operator< (const IndexedPhrasesPair<OrientationIndexType, StringIndexType>& lhs, const IndexedPhrasesPair<OrientationIndexType, StringIndexType>& rhs) {

    typedef typename IndexedPhrasesPair<OrientationIndexType, StringIndexType>::alignment_point_t alignment_point_t;
    typedef typename IndexedPhrasesPair<OrientationIndexType, StringIndexType>::token_index_t string_index_t;

    // Alignments comparable?
    if ( lhs._alignmentLength != rhs._alignmentLength ) {
        // Shorter alignment length => lesser item.
        return lhs._alignmentLength < rhs._alignmentLength;
    }

    // Same alignment length -> compare alignments.
    int cmp = memcmp(lhs._alignment, rhs._alignment, lhs._alignmentLength * 2 * sizeof(alignment_point_t));

    if ( cmp != 0 ) {
        // Alignments differ.
        return cmp < 0;
    }

    // Alignments are equal, compare phrases (data).

    if ( lhs._srcPhraseLength != rhs._srcPhraseLength ) {
        // Source phrase lengths differs.
        return lhs._srcPhraseLength < rhs._srcPhraseLength;
    }

    if ( lhs._tgtPhraseLength != rhs._tgtPhraseLength ) {
        // Target phrase lengths differs.
        return lhs._tgtPhraseLength < rhs._tgtPhraseLength;
    }

    // Phrases have matching lengths, compare the data in the end:
    cmp = memcmp(lhs._data, rhs._data, (lhs._srcPhraseLength + lhs._tgtPhraseLength) * sizeof(string_index_t));
    if ( cmp != 0 ) {
        // Data differ.
        return cmp < 0;
    }

    // Compare orientation info in the end.
    return lhs._orientationInfoIndex < rhs._orientationInfoIndex;
}
#endif

#endif	/* INDEXEDPHRASESPAIR_H */
