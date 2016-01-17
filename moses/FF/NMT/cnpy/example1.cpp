#include"cnpy.h"
#include<complex>
#include<cstdlib>
#include<iostream>
#include<map>
#include<string>

const int Nx = 128;
const int Ny = 64;
const int Nz = 32;

int main()
{
    //create random data
    std::complex<double>* data = new std::complex<double>[Nx*Ny*Nz];
    for(int i = 0;i < Nx*Ny*Nz;i++) data[i] = std::complex<double>(rand(),rand());

    //save it to file
    const unsigned int shape[] = {Nz,Ny,Nx};
    cnpy::npy_save("arr1.npy",data,shape,3,"w");

    //load it into a new array
    cnpy::NpyArray arr = cnpy::npy_load("arr1.npy");
    std::complex<double>* loaded_data = reinterpret_cast<std::complex<double>*>(arr.data);
    
    //make sure the loaded data matches the saved data
    assert(arr.word_size == sizeof(std::complex<double>));
    assert(arr.shape.size() == 3 && arr.shape[0] == Nz && arr.shape[1] == Ny && arr.shape[2] == Nx);
    for(int i = 0; i < Nx*Ny*Nz;i++) assert(data[i] == loaded_data[i]);

    //append the same data to file
    //npy array on file now has shape (Nz+Nz,Ny,Nx)
    cnpy::npy_save("arr1.npy",data,shape,3,"a");

    //now write to an npz file
    //non-array variables are treated as 1D arrays with 1 element
    double myVar1 = 1.2;
    char myVar2 = 'a';
    unsigned int shape2[] = {1};
    cnpy::npz_save("out.npz","myVar1",&myVar1,shape2,1,"w"); //"w" overwrites any existing file
    cnpy::npz_save("out.npz","myVar2",&myVar2,shape2,1,"a"); //"a" appends to the file we created above
    cnpy::npz_save("out.npz","arr1",data,shape,3,"a"); //"a" appends to the file we created above

    //load a single var from the npz file
    cnpy::NpyArray arr2 = cnpy::npz_load("out.npz","arr1");

    //load the entire npz file
    cnpy::npz_t my_npz = cnpy::npz_load("out.npz");
    
    //check that the loaded myVar1 matches myVar1
    cnpy::NpyArray arr_mv1 = my_npz["myVar1"];
    double* mv1 = reinterpret_cast<double*>(arr_mv1.data);
    assert(arr_mv1.shape.size() == 1 && arr_mv1.shape[0] == 1);
    assert(mv1[0] == myVar1);

    //cleanup: note that we are responsible for deleting all loaded data
    delete[] data;
    delete[] loaded_data;
    arr2.destruct();
    my_npz.destruct();
}
