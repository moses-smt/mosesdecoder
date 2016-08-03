
#include <iostream>
#include <functional>
#include <cmath>

template <class F, class E>
class Expr1 {
  public:
    Expr1(F f, E e) : f_(f), e_(e) {}

    operator float() {
      std::cerr << "calc1" << std::endl;
      return f_(e_);
    }

  private:
    F f_;
    E e_;
};

template <class F, class E1, class E2>
class Expr2 {
  public:
    Expr2(F f, E1 e1, E2 e2) : f_(f), e1_(e1), e2_(e2) {}

    operator float() {
      std::cerr << "calc2" << std::endl;
      return f_(e1_, e2_);
    }

  private:
    F f_;
    E1 e1_;
    E2 e2_;
};

template <class F, class E>
Expr1<F, E> expr(F f, E e) {
    return Expr1<F, E>(f, e);
}

template <class F, class E1, class E2>
Expr2<F, E1, E2> expr(F f, E1 e1, E2 e2) {
    return Expr2<F, E1, E2>(f, e1, e2);
}

template <class E1, class E2>
auto operator+(E1 e1, E2 e2) -> decltype(expr(std::plus<float>(), e1, e2))  {
    return expr(std::plus<float>(), e1, e2);
}

template <class E1, class E2>
auto operator*(E1 e1, E2 e2) -> decltype(expr(std::multiplies<float>(), e1, e2))  {
    return expr(std::multiplies<float>(), e1, e2);
}

int main(int argc, char** argv) {
   auto e =  5.0 + ((expr(exp, expr(log, 3)) + 1.0) * -2.0);
   std::cerr << "t" << std::endl;
   float f = e;
   std::cerr << f << std::endl;

   return 0;

  //Exp exp1 = eTanh(ePlus(eMult(eMinus(_1, _2), _3), eMult(_3, _4)));
  //Exp exp1 = tanh(((_1 - _2) * _3) + (_2 * _4));
  //TupleIterator(M1, M2, M3, M4, M5.begin());
}

