#include <cassert>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

#ifdef MPI_ENABLED
#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/collectives.hpp>

#include <boost/unordered_map.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/map.hpp>
namespace mpi = boost::mpi;
#endif

#include "FeatureVector.h"

using namespace Josiah;
using namespace std;


//typedef boost::unordered_map<string,float> nvmap;
typedef map<string,float> nvmap;

struct Data {

    string a;
    float b;
};

namespace boost { namespace serialization {
    template<class Archive>
    void serialize(Archive& ar, Data& d, const unsigned int) {
        ar & d.a;
        ar & d.b;
    }
}
}

int main(int argc, char* argv[])
{
#ifdef MPI_ENABLED
    mpi::environment env(argc, argv);
    mpi::communicator world;

    //cerr << world.rank() << endl;

    float rank = world.rank();

    ostringstream ostr;
    ostr << rank;

    string filename = "mpi.log" + ostr.str();
    ofstream log(filename.c_str());
    assert(log);
    log << "MPI rank: " << rank << endl;
    log << "MPI size: " << world.size() << endl;


    FVector fv;


    FName fn_r("R", ostr.str());
    FName fn_s("S", ostr.str());
    FName fn_t("T", ostr.str());
    FName fn_i ("I", "1");
    FName fn_j ("J", "1");
    FName fn_k ("K", "1");

    fv[fn_r] = 1.2;
    fv[fn_s] = 2.1;
    fv[fn_t] = -1.2;
    fv[fn_i] = 1/(1 + rank);
    fv[fn_j] = 2;
    fv[fn_k] = 23.1;
    
    log  << "FV: " << fv << endl;

    FVector sum;

    //mpi::broadcast(world,fv,0);
    mpi::reduce(world,fv,sum,FVectorPlus(),0);


    /*
    float sent = 1  / (float)(world.rank() + 3);
    log << "sent: " << sent << endl;
    float rcvd;
    mpi::reduce(world,send,rcvd,std::plus<float>(), 0);
    if (world.rank() == 0) cerr  << "Received " << rcvd << endl;
    */

    if (rank == 0) {
       cerr << "Sum: " <<  sum << endl;
    }

    log.close();
#endif
    return 0;
}
