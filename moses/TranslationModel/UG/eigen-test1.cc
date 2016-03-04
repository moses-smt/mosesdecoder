#include <iostream>
#include <fstream>
#include <sstream>
#include <unsupported/Eigen/MatrixFunctions>
#include <Eigen/Core>
#include <Eigen/Dense>

#include <map>
#include <vector>
#include <string>

using namespace std;
using namespace Eigen;
using Eigen::MatrixXd;




int main(int argc, char* argv[])
{
  vector<string> row2word;
  vector<float>  idf;
  map<string,size_t> L1word2row;
  map<string,size_t> L2word2row;
  ifstream termdocmatrix(argv[1]);
  string line;
  size_t numdocs = 0;
  size_t lastid = 0;
  map<string,size_t>* word2row = &L1word2row;
  float f;
  while (getline(termdocmatrix,line))
    {
      size_t id; string term; float idfval, dummy;
      istringstream buf(line);
      buf >> id >> term >> idfval;
      if (id < lastid) word2row = &L2word2row;
      (*word2row)[term] = row2word.size();
      row2word.push_back(term);
      idf.push_back(idfval);
      lastid = id;
      if (row2word.size() == 1) while (buf >> dummy) ++numdocs;
    }

  ifstream eigenvalues((string(argv[2]) + ".S").c_str());
  vector<float> eigenvals; 
  while (eigenvalues >> f) eigenvals.push_back(f);
  size_t num_dimensions = eigenvals.size();
  MatrixXd S(num_dimensions,num_dimensions);
  for (size_t i = 0; i < num_dimensions; ++i) 
    S(i,i) = eigenvals[i];
  
  MatrixXd T(row2word.size(),num_dimensions);
  ifstream U((string(argv[2])+".U").c_str());
  for (size_t r = 0; r < row2word.size(); ++r)
    {
      getline(U,line);
      istringstream buf(line);
      for (size_t c = 0; c < num_dimensions; ++c) 
	{
	  // cout << r << " " << c << " " << T.rows() << " " << T.cols() << endl;
	  buf >> f;
	  T(r,c) = f;
	}
    }
  MatrixXd TS = T * S.inverse();


  string word;
  MatrixXd d = MatrixXd::Zero(row2word.size(),1);
  while (cin >> word) 
    {
      map<string,size_t>::iterator m = L1word2row.find(word);
      if (m != L1word2row.end()) ++d(m->second,0);
    }
  
  for (size_t i = 0; i < d.rows(); ++i)
    if (d(i,0)) d(i,0) = (1 + log(d(i,0))) * idf[i];
  d = d.transpose() * TS;
  
  MatrixXd Srt = S.sqrt();
  
  MatrixXd dsrt = d * Srt;
  float dd = dsrt.row(0) * dsrt.row(0).transpose();

  for (map<string,size_t>::iterator m = L1word2row.begin(); m != L1word2row.end(); ++m)
    {
      
      MatrixXd tsrt = T.row(m->second) * Srt;
      float tt = tsrt.row(0) * tsrt.row(0).transpose();
      float x = (tsrt.row(0) * dsrt.row(0).transpose()); x /= (sqrt(tt) * sqrt(dd));
      cout << x << " " << m->first << endl;
    }

  for (map<string,size_t>::iterator m = L2word2row.begin(); m != L2word2row.end(); ++m)
    {
      
      MatrixXd tsrt = T.row(m->second) * Srt;
      float tt = tsrt.row(0) * tsrt.row(0).transpose();
      float x = (tsrt.row(0) * dsrt.row(0).transpose()); x /= (sqrt(tt) * sqrt(dd)); 
      cout << x << " " << m->first << endl;
    }
}
