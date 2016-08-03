#pragma once


#if __cplusplus > 199711L


#include <type_traits>
#include <utility>
#include <thrust/tuple.h>
#include <thrust/functional.h>


namespace thrust
{
namespace experimental
{


template<class T>
struct is_placeholder : thrust::detail::false_type {};


template<unsigned int i>
struct is_placeholder<
  thrust::detail::functional::actor<
    thrust::detail::functional::argument<i>
  >
> : thrust::detail::true_type {};


namespace detail
{
namespace bind_detail
{


template<class T>
using decay_t = typename std::decay<T>::type;


template<class _Tp, _Tp... _Ip>
struct integer_sequence
{
  typedef _Tp value_type;
  static_assert(std::is_integral<_Tp>::value,
                "std::integer_sequence can only be instantiated with an integral type" );
  static constexpr size_t size() noexcept { return sizeof...(_Ip); }
};


template<size_t... _Ip>
using index_sequence = integer_sequence<size_t, _Ip...>;


template <class _Tp, _Tp _Sp, _Tp _Ep, class _IntSequence>
struct make_integer_sequence_impl_unchecked;


template <class _Tp, _Tp _Sp, _Tp _Ep, _Tp ..._Indices>
struct make_integer_sequence_impl_unchecked<_Tp, _Sp, _Ep,
                                            integer_sequence<_Tp, _Indices...>>
{
  typedef typename make_integer_sequence_impl_unchecked
                   <
                      _Tp, _Sp+1, _Ep,
                      integer_sequence<_Tp, _Indices..., _Sp>
                   >::type type;
};


template <class _Tp, _Tp _Ep, _Tp ..._Indices>
struct make_integer_sequence_impl_unchecked<_Tp, _Ep, _Ep,
                                            integer_sequence<_Tp, _Indices...>>
{
  typedef integer_sequence<_Tp, _Indices...> type;
};


template <class _Tp, _Tp _Ep>
struct make_integer_sequence_impl
{
  static_assert(std::is_integral<_Tp>::value,
                "std::make_integer_sequence can only be instantiated with an integral type" );
  static_assert(0 <= _Ep, "std::make_integer_sequence input shall not be negative");
  typedef typename make_integer_sequence_impl_unchecked
                   <
                      _Tp, 0, _Ep, integer_sequence<_Tp>
                   >::type type;
};


template<class _Tp, _Tp _Np>
using make_integer_sequence = typename make_integer_sequence_impl<_Tp, _Np>::type;


template<size_t _Np>
using make_index_sequence = make_integer_sequence<size_t, _Np>;


template<class... _Tp>
using index_sequence_for = make_index_sequence<sizeof...(_Tp)>;


__thrust_hd_warning_disable__
template<typename F, typename Tuple, size_t... I>
__host__ __device__
auto apply_impl(F&& f, Tuple&& t, index_sequence<I...>)
  -> decltype(
       std::forward<F>(f)(
         thrust::get<I>(std::forward<Tuple>(t))...
       )
     )
{
  return std::forward<F>(f)(
    thrust::get<I>(std::forward<Tuple>(t))...
  );
}


template<typename F, typename Tuple>
__host__ __device__
auto apply(F&& f, Tuple&& t)
  -> decltype(
       apply_impl(
         std::forward<F>(f),
         std::forward<Tuple>(t),
         make_index_sequence<thrust::tuple_size<decay_t<Tuple>>::value>()
       )
     )
{
  using Indices = make_index_sequence<thrust::tuple_size<decay_t<Tuple>>::value>;
  return apply_impl(
    std::forward<F>(f),
    std::forward<Tuple>(t),
    Indices()
  );
}


template<class ArgTuple, class BoundArg>
__host__ __device__
auto substitute_arg(ArgTuple&&, BoundArg&& bound_arg,
                    typename thrust::detail::disable_if<
                      is_placeholder<decay_t<BoundArg>>::value
                    >::type* = 0)
  -> decltype(
       std::forward<BoundArg>(bound_arg)
     )
{
  return std::forward<BoundArg>(bound_arg);
}


template<unsigned int i>
struct placeholder
  : thrust::detail::functional::actor<
      thrust::detail::functional::argument<i>
    >
{};


template<class T>
struct argument_index
  : thrust::detail::integral_constant<
      unsigned int, 0
    >
{};


template<unsigned int i>
struct argument_index<
  thrust::detail::functional::actor<
    thrust::detail::functional::argument<i>
  >
>
  : thrust::detail::integral_constant<
      unsigned int, i
    >
{};


template<class ArgTuple, class BoundArg>
__host__ __device__
auto substitute_arg(ArgTuple&& arg_tuple, const BoundArg&,
                   typename thrust::detail::enable_if<
                     is_placeholder<decay_t<BoundArg>>::value
                   >::type* = 0)
  -> decltype(
       thrust::get<
         argument_index<BoundArg>::value
       >(std::forward<ArgTuple>(arg_tuple))
     )
{
  const unsigned int idx = argument_index<BoundArg>::value;
  return thrust::get<idx>(std::forward<ArgTuple>(arg_tuple));
}


// XXX WAR nvbug 1527140
//     unpack template parameter packs into thrust::tuple manually
template<class... T>
struct tuple_war_1527140;

template<>
struct tuple_war_1527140<>
{
  using type = thrust::tuple<>;
};

template<class T1>
struct tuple_war_1527140<T1>
{
  using type = thrust::tuple<T1>;
};

template<class T1, class T2>
struct tuple_war_1527140<T1,T2>
{
  using type = thrust::tuple<T1,T2>;
};

template<class T1, class T2, class T3>
struct tuple_war_1527140<T1,T2,T3>
{
  using type = thrust::tuple<T1,T2,T3>;
};

template<class T1, class T2, class T3, class T4>
struct tuple_war_1527140<T1,T2,T3,T4>
{
  using type = thrust::tuple<T1,T2,T3,T4>;
};

template<class T1, class T2, class T3, class T4, class T5>
struct tuple_war_1527140<T1,T2,T3,T4,T5>
{
  using type = thrust::tuple<T1,T2,T3,T4,T5>;
};

template<class T1, class T2, class T3, class T4, class T5, class T6>
struct tuple_war_1527140<T1,T2,T3,T4,T5,T6>
{
  using type = thrust::tuple<T1,T2,T3,T4,T5,T6>;
};

template<class T1, class T2, class T3, class T4, class T5, class T6, class T7>
struct tuple_war_1527140<T1,T2,T3,T4,T5,T6,T7>
{
  using type = thrust::tuple<T1,T2,T3,T4,T5,T6,T7>;
};

template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
struct tuple_war_1527140<T1,T2,T3,T4,T5,T6,T7,T8>
{
  using type = thrust::tuple<T1,T2,T3,T4,T5,T6,T7,T8>;
};

template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
struct tuple_war_1527140<T1,T2,T3,T4,T5,T6,T7,T8,T9>
{
  using type = thrust::tuple<T1,T2,T3,T4,T5,T6,T7,T8,T9>;
};

template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9, class T10>
struct tuple_war_1527140<T1,T2,T3,T4,T5,T6,T7,T8,T9,T10>
{
  using type = thrust::tuple<T1,T2,T3,T4,T5,T6,T7,T8,T9,T10>;
};

template<class... T>
using tuple = typename tuple_war_1527140<T...>::type;


// XXX replace this with the variadic forward_as_tuple() when thrust::tuple's constructor can receive && references
inline __host__ __device__
tuple<> forward_as_tuple()
{
  return tuple<>();
}


template<class T>
__host__ __device__
tuple<T&> forward_as_tuple(T& arg)
{
  return tuple<T&>(arg);
}


template<class T>
__host__ __device__
tuple<const T&> forward_as_tuple(const T& arg)
{
  return tuple<const T&>(arg);
}


template<class T1, class T2>
__host__ __device__
tuple<T1&,T2&> forward_as_tuple(T1& arg1, T2& arg2)
{
  return tuple<T1&,T2&>(arg1, arg2);
}


template<class T1, class T2>
__host__ __device__
tuple<T1&,const T2&> forward_as_tuple(T1& arg1, const T2& arg2)
{
  return tuple<T1&,const T2&>(arg1, arg2);
}


template<class T1, class T2>
__host__ __device__
tuple<const T1&,T2&> forward_as_tuple(const T1& arg1, T2& arg2)
{
  return tuple<const T1&,T2&>(arg1, arg2);
}


template<class T1, class T2>
__host__ __device__
tuple<const T1&,const T2&> forward_as_tuple(const T1& arg1, const T2& arg2)
{
  return tuple<const T1&,const T2&>(arg1, arg2);
}


template<class T1, class T2, class T3>
__host__ __device__
tuple<T1&,T2&,T3&> forward_as_tuple(T1& arg1, T2& arg2, T3& arg3)
{
  return tuple<T1&,T2&,T3&>(arg1, arg2, arg3);
}


template<class T1, class T2, class T3>
__host__ __device__
tuple<T1&,T2&,const T3&> forward_as_tuple(T1& arg1, T2& arg2, const T3& arg3)
{
  return tuple<T1&,T2&,const T3&>(arg1, arg2, arg3);
}


template<class T1, class T2, class T3>
__host__ __device__
tuple<T1&,const T2&,T3&> forward_as_tuple(T1& arg1, const T2& arg2, T3& arg3)
{
  return tuple<T1&,const T2&, T3&>(arg1, arg2, arg3);
}


template<class T1, class T2, class T3>
__host__ __device__
tuple<T1&,const T2&,const T3&> forward_as_tuple(T1& arg1, const T2& arg2, const T3& arg3)
{
  return tuple<T1&,const T2&,const T3&>(arg1, arg2, arg3);
}


template<class T1, class T2, class T3>
__host__ __device__
tuple<const T1&,T2&,T3&> forward_as_tuple(const T1& arg1, T2& arg2, T3& arg3)
{
  return tuple<const T1&,T2&,T3&>(arg1, arg2, arg3);
}


template<class T1, class T2, class T3>
__host__ __device__
tuple<const T1&,T2&,const T3&> forward_as_tuple(const T1& arg1, T2& arg2, const T3& arg3)
{
  return tuple<const T1&,T2&,const T3&>(arg1, arg2, arg3);
}


template<class T1, class T2, class T3>
__host__ __device__
tuple<const T1&,const T2&,T3&> forward_as_tuple(const T1& arg1, const T2& arg2, T3& arg3)
{
  return tuple<const T1&,const T2&, T3&>(arg1, arg2, arg3);
}


template<class T1, class T2, class T3>
__host__ __device__
tuple<const T1&,const T2&,const T3&> forward_as_tuple(const T1& arg1, const T2& arg2, const T3& arg3)
{
  return tuple<const T1&,const T2&,const T3&>(arg1, arg2, arg3);
}


template<class ArgTuple, class BoundArgTuple, size_t... I>
__host__ __device__
auto substitute_impl(ArgTuple&& arg_tuple, BoundArgTuple&& bound_arg_tuple, index_sequence<I...>)
  -> decltype(
       forward_as_tuple(
         substitute_arg(
           std::forward<ArgTuple>(arg_tuple),
           thrust::get<I>(std::forward<BoundArgTuple>(bound_arg_tuple))
         )...
       )
     )
{
  return forward_as_tuple(
    substitute_arg(
      std::forward<ArgTuple>(arg_tuple),
      thrust::get<I>(std::forward<BoundArgTuple>(bound_arg_tuple))
    )...
  );
}


template<class ArgTuple, class BoundArgTuple>
__host__ __device__
auto substitute(ArgTuple&& arg_tuple, BoundArgTuple&& bound_arg_tuple)
  -> decltype(
       substitute_impl(
         std::forward<ArgTuple>(arg_tuple),
         std::forward<BoundArgTuple>(bound_arg_tuple),
         make_index_sequence<thrust::tuple_size<decay_t<BoundArgTuple>>::value>()
       )
     )
{
  using Indices = make_index_sequence<thrust::tuple_size<decay_t<BoundArgTuple>>::value>;
  return substitute_impl(std::forward<ArgTuple>(arg_tuple), std::forward<BoundArgTuple>(bound_arg_tuple), Indices());
}


template<class F, class... BoundArgs>
class bind_expression
{
  public:
    __host__ __device__
    bind_expression(const F& f, const BoundArgs&... bound_args)
      : fun_(f),
        bound_args_(bound_args...)
    {}

    template<class... OtherArgs>
    __host__ __device__
    auto operator()(OtherArgs&&... args) const
      -> decltype(
           apply(
             *std::declval<const F*>(),
             substitute(
               forward_as_tuple(std::forward<OtherArgs>(args)...),
               *std::declval<const tuple<BoundArgs...>*>()
             )
           )
         )
    {
      return apply(
        fun_,
        substitute(
          forward_as_tuple(std::forward<OtherArgs>(args)...),
          bound_args_
        )
      );
    }

    template<class... OtherArgs>
    __host__ __device__
    auto operator()(OtherArgs&&... args)
      -> decltype(
           apply(
             *std::declval<F*>(),
             substitute(
               forward_as_tuple(std::forward<OtherArgs>(args)...),
               *std::declval<tuple<BoundArgs...>*>()
             )
           )
         )
    {
      return apply(
        fun_,
        substitute(
          forward_as_tuple(std::forward<OtherArgs>(args)...),
          bound_args_
        )
      );
    }

  private:
    F fun_;
    tuple<BoundArgs...> bound_args_;
};


} // end bind_detail
} // end detail


template<class F, class... BoundArgs>
__host__ __device__
detail::bind_detail::bind_expression<
  detail::bind_detail::decay_t<F>,
  detail::bind_detail::decay_t<BoundArgs>...
> bind(F&& f, BoundArgs&&... bound_args)
{
  using namespace thrust::experimental::detail::bind_detail;
  return bind_expression<decay_t<F>,decay_t<BoundArgs>...>(std::forward<F>(f), std::forward<BoundArgs>(bound_args)...);
}


} // end experimental
} // end thrust

#endif // __cplusplus

