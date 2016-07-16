// -*- mode: c++; indent-tabs-mode: nil; tab-width:2  -*-
#include "LSA.h"
#include "moses/TranslationModel/UG/generic/file_io/ug_stream.h"

using namespace std;

Moses::LsaModel LSA;

void
fold_in1(std::string const& fname, std::vector<float>& d)
{
  boost::iostreams::filtering_istream doc;
  ugdiss::open_input_stream(fname, doc);
  std::string line, w1,w2;
  while (getline(doc,line))
    {
      std::istringstream buf(line);
      if (buf>>w1)
        {
          LSA.fold_in(LSA.row1(w1),1,d);      
          while (buf>>w2)
            {
              LSA.fold_in(LSA.row1(w2),1,d);      
              LSA.fold_in(LSA.row1(w1 + " " + w2),1,d);
              w1 = w2;
            }
        }
    }
}

void
fold_in2(std::string const& fname, std::vector<float>& d)
{
  boost::iostreams::filtering_istream doc;
  ugdiss::open_input_stream(fname, doc);
  std::string line, w1,w2;
  while (getline(doc,line))
    {
      std::istringstream buf(line);
      if (buf>>w1)
        {
          LSA.fold_in(LSA.row2(w1),1,d);      
          while (buf>>w2)
            {
              LSA.fold_in(LSA.row2(w2),1,d);      
              LSA.fold_in(LSA.row2(w1 + " " + w2),1,d);
              w1 = w2;
            }
        }
    }
}

float
cosine(std::vector<float> const& A, std::vector<float> const& B)
{
  float a = 0, b = 0, c = 0;
  for (size_t i = 0; i < A.size(); ++i)
    {
      a += A[i] * A[i];
      b += B[i] * B[i];
      c += A[i] * B[i];
    }
  return c / (sqrt(a) * sqrt(b));
}

int main(int argc, char* argv[])
{
  LSA.open(argv[1], argv[2], argv[3], atoi(argv[4]));

  std::vector<float> d1(LSA.S().size(), 0);
  fold_in1(argv[5], d1);

  for (int i = 6; i < argc; ++i)
    {
      std::vector<float> d2(LSA.S().size(), 0);
      fold_in2(argv[i], d2);
      printf("%.6f %s\n", cosine(d1,d2), argv[i]);
    }

  // std::vector<float> cache(LSA.T().size(), -2);
  // for (int i = 6; i < argc; ++i)
  //   {
  //     boost::iostreams::filtering_istream cnd_doc;
  //     ugdiss::open_input_stream(argv[i], cnd_doc);
  //     float ret = 0;
  //     size_t wcnt = 0;
  //     while (getline(cnd_doc,line))
  //       {
  //         std::istringstream buf(line);
  //         if (buf>>w1)
  //           {
  //             uint32_t rid = LSA.row2(w1);
  //             float f;
  //             if ((f = cache[rid]) < -1)
  //               f = cache[rid] = LSA.term_doc_sim(rid, d);
  //             ++wcnt;
  //             if (f < 0) ret -= log(-f);
  //             else if (f > 0) ret += log(f);
  //             while (buf>>w2)
  //               {
  //                 wcnt += 2;
  //                 uint32_t rid = LSA.row2(w2);
  //                 if ((f = cache[rid]) < -1)
  //                   f = cache[rid] = LSA.term_doc_sim(rid, d);
  //                 cerr << f << " |" << w2 << "|" << endl;
  //                 if (f < 0) ret -= log(-f);
  //                 else if (f > 0) ret += log(f);
                  
  //                 rid = LSA.row2(w1 + " " + w2);
  //                 if ((f = cache[rid]) < -1)
  //                   f = cache[rid] = LSA.term_doc_sim(rid, d);
  //                 cerr << f << " |" << w1 << " " << w2 << "|" << endl;
  //                 // ret += f;
  //                 w1 = w2;
  //               }
  //           }
  //       }
  //     printf("%f %s\n", exp(ret/wcnt), argv[i]);
  
  // LSA.fold_in(LSA.row1(argv[5]),v);
  // // BOOST_FOREACH(float& f, v) f *= argc-3;
  // for (int i = 6; i < argc; ++i)
  //   {
  //     uint32_t r = LSA.row1(argv[i]);
  //     if (r == std::numeric_limits<uint32_t>::max()) continue;
  //     LSA.fold_in(r,v);
  //     LSA.fold_in(LSA.row1(std::string(argv[i-1]) + " " + argv[i]), v);
  //   }
  // for (size_t i = LSA.first_L2_ID(); i <= LSA.last_L2_ID(); ++i)
  //   {
  //     // if (LSA.rlabel(i).find(' ') != std::string::npos) continue;
  //     float score = LSA.term_doc_sim(i, v);
  //     if (score == 0) continue;
  //     std::cout << score << " " << LSA.rlabel(i) << std::endl;
  //   }
}
