/* SWIG interface file for GadgetReader:
 * module gadgetreader*/
%module gadgetreader
%{
#include "../gadgetreader.hpp"

%}
/* Numpy integration: 
 * This works somewhat poorly and isn't in my numpy install*/
/*%{
#define SWIG_FILE_WITH_INIT
%}
%include "numpy.i"
%init %{
import_array();
%}*/

/*STL declarations. */
/*Gives us access to std::string as native types*/
%include "std_string.i"

/*Swig doesn't support this in Perl yet*/
#if !defined(SWIGPERL)
/*We want to wrap std::set<std::string>*/
%include "std_set.i"
namespace std{
   %template(StrSet) set<string>;
}
#endif

/*We want to wrap std::vector<std::string>*/
%include "std_vector.i"
namespace std{
   %template(StrVector) vector<string>;
   %template(IntVector) vector<int>;
   %template(FloatVector) vector<float>;
}
%include "stdint.i"

/* Let us use arrays: this may be needed in future to 
 * allow access to the npart and mass arrays of gadgetheader*/
/*%include "carrays.i"
%array_class(int,IntArr);
%array_class(double,DoubArr);*/

%feature("autodoc","1");
%include "gadgetreader.hpp"

/* Methods to access the arrays in gadget_header, which SWIG can't wrap properly*/
namespace GadgetReader{
%extend gadget_header{
        uint32_t GetNpart(int i){
                if(i >= N_TYPE || i<0)
                        return 0;
                else 
                        return $self->npart[i];
        }
        double GetMass(int i){
                if(i >= N_TYPE || i<0)
                        return 0;
                else 
                        return $self->mass[i];
        }
};
}
/* int64_t GetBlock(std::string BlockName, float *block, int64_t npart_toread, int64_t start_part, int skip_type){*/
/*%apply (float* IN_ARRAY1, int DIMS1) {(float *block, int64_t npart_toread,)};*/
