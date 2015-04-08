#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <iostream>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <limits>
#include <sstream>
#include <boost/algorithm/string.hpp>



namespace MosesTuning {

namespace M2 {

typedef std::vector<float> Stats;

typedef std::vector<std::string> Sentence;

std::ostream& operator<<(std::ostream& o, Sentence s);

const std::string ToLower(const std::string& str);

struct Annot {
    size_t i;
    size_t j;
    
    std::string type;
    std::string edit;
    
    size_t annotator;
    
    bool operator<(Annot a) const {
        return i < a.i || (i == a.i && j < a.j)
            || (i == a.i && j == a.j && annotator < a.annotator)
            || (i == a.i && j == a.j && annotator == a.annotator && edit < a.edit);
    }
};

typedef std::set<Annot> Annots;
typedef std::pair<Sentence, Annots> Unit;

typedef std::vector<Unit> M2File;

//std::ostream& operator<<(std::ostream& o, Annot a) {
//    return o << a.annotator << " (" << a.i << "," << a.j << ") : \"" << a.edit << "\"";
//}

struct Edit {
  Edit(float c = 1.0, size_t ch = 0, size_t unch = 1, std::string e = "")
  : cost(c), changed(ch), unchanged(unch), edit(e) {}
  
  float cost;
  size_t changed;
  size_t unchanged;
  std::string edit;
};

Edit operator+(Edit& e1, Edit& e2);

struct Vertex {
  Vertex(size_t a = 0, size_t b = 0) : i(a), j(b) {}
  
  bool operator<(const Vertex &v) const {
    return i < v.i || (i == v.i && j < v.j);
  }

  bool operator==(const Vertex &v) const {
    return i == v.i && j == v.j;
  }
  
  size_t i;
  size_t j;
};

//std::ostream& operator<<(std::ostream &o, Vertex v) {
//    return o << "(" << v.i << "," << v.j << ")";
//}

struct Edge {
  Edge(Vertex vv = Vertex(), Vertex uu = Vertex(), Edit editt = Edit())
  : v(vv), u(uu), edit(editt) {}
   
  bool operator<(const Edge &e) const {
    return v < e.v || (v == e.v && u < e.u);
  }
  
  Vertex v;
  Vertex u;
  Edit edit;
};

Edge operator+(Edge e1, Edge e2);

//std::ostream& operator<<(std::ostream &o, Edge e) {
//    return o << e.v << " -> " << e.u << " : \"" << e.edit.edit << "\" " << e.edit.cost ;
//}

typedef std::vector<size_t> Row;
typedef std::vector<Row> Matrix;

struct Info {
  Info(Vertex vv = Vertex(), Edit editt = Edit())
  : v(vv), edit(editt) {}

  bool operator<(const Info &i) const {
    return v < i.v;
  }

  Vertex v;
  Edit edit;
};

typedef std::set<Info> Track;
typedef std::vector<Track> TrackRow;
typedef std::vector<TrackRow> TrackMatrix;

typedef std::set<Vertex> Vertices;
typedef std::set<Edge> Edges;

class M2 {
  private:
    M2File m_m2;
    
    size_t m_max_unchanged;
    float m_beta;
    bool m_lowercase;
    
    std::set<size_t> m_annotators; 
    
  public:
    M2() : m_max_unchanged(2), m_beta(0.5), m_lowercase(true) {}
    M2(size_t max_unchanged, float beta, bool truecase) : m_max_unchanged(max_unchanged), m_beta(beta), m_lowercase(!truecase) {}
    
    float Beta() {
        return m_beta;
    }
    
    void ReadM2(const std::string& filename) {
        std::ifstream m2file(filename.c_str());
        std::string line;
        
        //M2 m2;
        Unit unit;
        bool first = true;
        
        while(std::getline(m2file, line)) {
            if(line.size() > 2) {
              if(line.substr(0, 2) == "S ") {
                if(!first)
                    m_m2.push_back(unit);
                first = false;
    
                unit.first = Sentence();
                unit.second = Annots();
    
                std::string sentenceLine = line.substr(2);
                
                if(m_lowercase)
                    sentenceLine = ToLower(sentenceLine);
                
                //std::cout << sentenceLine << std::endl;
                
                boost::split(unit.first, sentenceLine, boost::is_any_of(" "), boost::token_compress_on);
              }
              if(line.substr(0, 2) == "A ") {
                std::string annotLine = line.substr(2);
                
                std::vector<std::string> annot;
                boost::iter_split(annot, annotLine, boost::algorithm::first_finder("|||"));
                
                if(annot[1] != "noop") {
                    Annot a;
                    std::stringstream rangeStr(annot[0]);            
                    rangeStr >> a.i >> a.j;
                    a.type = annot[1];
                    a.edit = annot[2];            
                    
                    if(m_lowercase)
                        a.edit = ToLower(a.edit);
                    
                    std::stringstream annotStr(annot[5]);
                    annotStr >> a.annotator;
                    
                    m_annotators.insert(a.annotator);
                    unit.second.insert(a);
                    //std::cout << a << std::endl;
                }
              }
            }
        }
        m_m2.push_back(unit);   
        //std::cout << m2.size() << std::endl;
    }
    
    int LevenshteinMatrix(const Sentence &s1, const Sentence &s2, Matrix &d, TrackMatrix &bt) {
        size_t n = s1.size();
        size_t m = s2.size();
          
        if (n == 0)
          return m;
        if (m == 0)
          return n;
        
        d.resize(n + 1, Row(m + 1, 0));
        bt.resize(n + 1, TrackRow(m + 1));
        
        for(size_t i = 0; i <= n; ++i) {
          d[i][0] = i;
          if(i > 0)
              bt[i][0].insert(Info(Vertex(i - 1, 0), Edit(1, 1, 0, "")));
        }
        for(size_t j = 0; j <= m; ++j) {
          d[0][j] = j;
          if(j > 0)
              bt[0][j].insert(Info(Vertex(0, j - 1), Edit(1, 1, 0, s2[j - 1])));
        }
        
        int cost;
        for(size_t i = 1; i <= n; ++i) {
          for(size_t j = 1; j <= m; ++j) {
            if(s1[i-1] == s2[j-1])
              cost = 0;
            else 
              cost = 2;
              
            size_t left = d[i][j - 1] + 1;
            size_t down = d[i - 1][j] + 1;
            size_t diag = d[i - 1][j - 1] + cost;
                  
            d[i][j] = std::min(left, std::min(down, diag));
            
            if(d[i][j] == left)
              bt[i][j].insert(Info(Vertex(i, j - 1), Edit(1, 1, 0, s2[j - 1])));
            if(d[i][j] == down)
              bt[i][j].insert(Info(Vertex(i - 1, j), Edit(1, 1, 0, "")));
            if(d[i][j] == diag)
              bt[i][j].insert(Info(Vertex(i - 1, j - 1), cost ? Edit(1, 1, 0, s2[j - 1]) : Edit(1, 0, 1, s2[j - 1]) ));
          }
        }
        
        /*
        for(size_t i = 0; i < d.size(); ++i) {
          for(size_t j = 0; j < d[i].size(); ++j) {
              std::cout << (d[i][j] < 10 ? " " : "") << d[i][j] << " ";
          }
          std::cout << std::endl;
        }
        */
      
        return d[n][m];
    }
      
      
    void BuildGraph(const TrackMatrix &bt, Vertices &V, Edges &E) {    
        Vertex start(bt.size() - 1, bt[0].size() - 1);
        
        /*
        for(size_t i = 0; i < bt.size(); ++i) {
            for(size_t j = 0; j < bt[i].size(); ++j) {
                std::cout << i << " " << j << " : " << (bt[i][j].size() > 0 ? bt[i][j].begin()->v : Vertex(-1,-1)) << std::endl;
            }
        }
        */
        
        std::queue<Vertex> Q;
        Q.push(start);
        while(!Q.empty()) {
            Vertex v = Q.front();
            Q.pop();
            if(V.count(v) > 0)
                continue;
            V.insert(v);
            
            for(Track::iterator it = bt[v.i][v.j].begin();
                it != bt[v.i][v.j].end(); ++it) {
                Edge e(it->v, v, it->edit);
                E.insert(e);
                if(V.count(e.v) == 0)
                    Q.push(e.v);
            }
        }
        
        Edges newE;
        do {
            newE.clear();
            for(Edges::iterator it1 = E.begin(); it1 != E.end(); ++it1) {
                for(Edges::iterator it2 = E.begin(); it2 != E.end(); ++it2) {
                    if(it1->u == it2->v) {
                        Edge e = *it1 + *it2;
                        if(e.edit.changed > 0 &&
                           e.edit.unchanged <= m_max_unchanged &&
                           E.count(e) == 0)
                            newE.insert(e);
                    }
                }
            }
            E.insert(newE.begin(), newE.end());
        } while(newE.size() > 0); 
    }
    
    void AddWeights(Edges &E, const Unit &u) {
        for(Edges::iterator it1 = E.begin(); it1 != E.end(); ++it1) {
            if(it1->edit.changed > 0) {
                const_cast<float&>(it1->edit.cost) += 0.001;   
                for(Annots::iterator it2 = u.second.begin(); it2 != u.second.end(); ++it2) {
                    if(it1->v.i == it2->i && it1->u.i == it2->j && it1->edit.edit == it2->edit) {
                        int newWeight = -(m_max_unchanged + 1) * E.size();
                        const_cast<float&>(it1->edit.cost) = newWeight;
                    }
                }
            }
        }
    }
    
    void BellmanFord(Vertices &V, Edges &E) {
        Vertex source(0, 0);
        std::map<Vertex, float> distance;
        std::map<Vertex, Vertex> predecessor;
        
        for(Vertices::iterator it = V.begin(); it != V.end(); ++it) {
            if(*it == source)
                distance[*it] = 0;
            else {
                distance[*it] = std::numeric_limits<float>::infinity();
            }
        }
        
        for(size_t i = 1; i < V.size(); ++i) {
            for(Edges::iterator it = E.begin(); it != E.end(); ++it) {
                if(distance[it->v] + it->edit.cost < distance[it->u]) {
                    distance[it->u] = distance[it->v] + it->edit.cost;
                    predecessor[it->u] = it->v;
                }
            }
        }
        
        Edges newE;
        
        Vertex v = *V.rbegin();
        while(true) {
            //std::cout << predecessor[v] << " -> " << v << std::endl;
            Edges::iterator it = E.find(Edge(predecessor[v], v));
            if(it != E.end()) {
                Edge f = *it;
                //std::cout << f << std::endl;
                newE.insert(f);
                
                v = predecessor[v];
                if(v == source)
                    break;
            }
            else {
                std::cout << "Error" << std::endl;
                break;
            }
        }
        E.clear();
        E.insert(newE.begin(), newE.end());
    }
    
    void AddStats(const Edges &E, const Annots &a, Stats &stats) {
        std::map<size_t, Stats> statsPerAnnotator;
        for(std::set<size_t>::iterator it = m_annotators.begin();
            it != m_annotators.end(); ++it) {
            statsPerAnnotator[*it] = Stats(4, 0);
        }
        
        for(Annots::iterator it = a.begin(); it != a.end(); it++)
            statsPerAnnotator[it->annotator][2]++;
        
        for(Edges::iterator eit = E.begin(); eit != E.end(); ++eit) {
          //std::cout << *it << std::endl;
          if(eit->edit.changed > 0) {
            for(std::set<size_t>::iterator ait = m_annotators.begin();
                ait != m_annotators.end(); ++ait) {
                statsPerAnnotator[*ait][1]++;
                
                Annot f;
                f.i = eit->v.i;
                f.j = eit->u.i;
                f.annotator = *ait;
                f.edit = eit->edit.edit;
                Annots::iterator fit = a.find(f);
                if(fit != a.end()) {
                    statsPerAnnotator[*ait][0]++;
                }
            }
          }
        }
        
        size_t bestAnnot = 0;
        float  bestF = -1;
        for(std::set<size_t>::iterator it = m_annotators.begin();
            it != m_annotators.end(); ++it) {
            Stats localStats = stats;
            localStats[0] += statsPerAnnotator[*it][0];
            localStats[1] += statsPerAnnotator[*it][1];
            localStats[2] += statsPerAnnotator[*it][2];

            float f = FScore(localStats);
            if(f > bestF) {
                bestF = f;
                bestAnnot = *it;
            }
        }
        stats[0] += statsPerAnnotator[bestAnnot][0];
        stats[1] += statsPerAnnotator[bestAnnot][1];
        stats[2] += statsPerAnnotator[bestAnnot][2];
    }
    
    void SufStats(const std::string &sStr, size_t i, Stats &stats) {        
        Matrix d;
        TrackMatrix bt;
        
        Vertices V;
        Edges E;
        
        Sentence s;
        
        std::string temp = sStr;
        if(m_lowercase)
            temp = ToLower(temp);
        
        boost::split(s, temp, boost::is_any_of(" "), boost::token_compress_on);
        
        Unit &unit = m_m2[i];
                
        int distance = LevenshteinMatrix(unit.first, s, d, bt);
        if(distance > unit.first.size()) {
            std::cerr << "Levenshtein distance is greater than source size." << std::endl;
            stats[0] =  0;
            stats[1] = distance;
            stats[2] =  0;
            stats[3] = unit.first.size();
            return;
        }
        else if(distance > 0) {      
            BuildGraph(bt, V, E);
            AddWeights(E, unit);    
            BellmanFord(V, E);        
        }
        
        //std::cout << s << std::endl;
        //std::cout << unit.first << std::endl;
        
        AddStats(E, unit.second, stats);
        
        //std::cerr << s << std::endl;
        //std::cerr << unit.first << std::endl;
        //std::cerr << stats[0] << " " << stats[1] << " " << stats[2] << std::endl;
        
        // source length
        stats[3] = unit.first.size();
    }
    
    float FScore(const Stats& stats) {
        float p = 1.0;
        if(stats[1] != 0)
            p = (float)stats[0] / (float)stats[1];
        
        float r = 1.0;
        if(stats[2] != 0)
            r = (float)stats[0] / (float)stats[2];
        
        float denom = (m_beta * m_beta * p + r);
        float f = 0.0;
        if(denom != 0)
            f = ((1 + m_beta * m_beta) * p * r) / denom;
        return f;
    }
    
    void FScore(const Stats& stats, float &p, float &r, float &f) {
        p = 1.0;
        if(stats[1] != 0)
            p = (float)stats[0] / (float)stats[1];
        
        r = 1.0;
        if(stats[2] != 0)
            r = (float)stats[0] / (float)stats[2];
        
        float denom = (m_beta * m_beta * p + r);
        f = 0.0;
        if(denom != 0)
            f = ((1 + m_beta * m_beta) * p * r) / denom;
    }
};

}

}