#pragma once

#ifdef NO_CUDA

#include <vector>
#include <functional>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/permutation_iterator.hpp>

namespace iterlib = boost;
namespace func = std;

#else

#include <thrust/iterator/counting_iterator.h>
#include <thrust/iterator/transform_iterator.h>
#include <thrust/iterator/permutation_iterator.h>
#include <thrust/functional.h>

namespace iterlib = thrust;
namespace func = thrust;

#endif

template <typename Iterator>
class row_repeater {
  public:

    typedef typename iterlib::iterator_difference<Iterator>::type difference_type;

    struct repeater_functor : public func::unary_function<difference_type,difference_type> {
        difference_type cols;

        repeater_functor(difference_type cols)
            : cols(cols) {}

#ifndef NO_CUDA
        __host__ __device__
#endif
        difference_type operator()(const difference_type& i) const { 
            return i % cols;
        }
    };

    typedef typename iterlib::counting_iterator<difference_type> CountingIterator;
    typedef typename iterlib::transform_iterator<repeater_functor, CountingIterator> TransformIterator;
    typedef typename iterlib::permutation_iterator<Iterator,TransformIterator> PermutationIterator;

    typedef PermutationIterator iterator;

    row_repeater(Iterator first, Iterator last, difference_type cols)
        : first(first), last(last), cols(cols) {}
   
    iterator begin(void) const {
        return PermutationIterator(first, TransformIterator(CountingIterator(0), repeater_functor(cols)));
    }

    iterator end(void) const {
        return begin() + (last - first) * cols;
    }
    
  protected:
    Iterator first;
    Iterator last;
    difference_type cols;
};

template <typename Iterator>
class col_repeater {
  public:

    typedef typename iterlib::iterator_difference<Iterator>::type difference_type;

    struct repeater_functor : public func::unary_function<difference_type,difference_type> {
        difference_type cols;

        repeater_functor(difference_type cols)
            : cols(cols) {}

#ifndef NO_CUDA
        __host__ __device__
#endif
        difference_type operator()(const difference_type& i) const { 
            return i / cols;
        }
    };

    typedef typename iterlib::counting_iterator<difference_type> CountingIterator;
    typedef typename iterlib::transform_iterator<repeater_functor, CountingIterator> TransformIterator;
    typedef typename iterlib::permutation_iterator<Iterator,TransformIterator> PermutationIterator;

    typedef PermutationIterator iterator;

    col_repeater(Iterator first, Iterator last, difference_type cols)
        : first(first), last(last), cols(cols) {}
   
    iterator begin(void) const {
        return PermutationIterator(first, TransformIterator(CountingIterator(0), repeater_functor(cols)));
    }

    iterator end(void) const {
        return begin() + (last - first) * cols;
    }
    
  protected:
    Iterator first;
    Iterator last;
    difference_type cols;
};

template <typename Iterator>
class strided_range
{
    public:

    typedef typename iterlib::iterator_difference<Iterator>::type difference_type;

    struct stride_functor : public func::unary_function<difference_type,difference_type>
    {
        difference_type stride;

        stride_functor(difference_type stride)
            : stride(stride) {}

#ifndef NO_CUDA
        __host__ __device__
#endif
        difference_type operator()(const difference_type& i) const
        { 
            return stride * i;
        }
    };

    typedef typename iterlib::counting_iterator<difference_type>                   CountingIterator;
    typedef typename iterlib::transform_iterator<stride_functor, CountingIterator> TransformIterator;
    typedef typename iterlib::permutation_iterator<Iterator,TransformIterator>     PermutationIterator;

    // type of the strided_range iterator
    typedef PermutationIterator iterator;

    // construct strided_range for the range [first,last)
    strided_range(Iterator first, Iterator last, difference_type stride)
        : first(first), last(last), stride(stride) {}
   
    iterator begin(void) const
    {
        return PermutationIterator(first, TransformIterator(CountingIterator(0), stride_functor(stride)));
    }

    iterator end(void) const
    {
        return begin() + ((last - first) + (stride - 1)) / stride;
    }
    
    protected:
    Iterator first;
    Iterator last;
    difference_type stride;
};
