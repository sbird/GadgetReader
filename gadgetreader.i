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


%include "gadgetreader.hpp"

/* int64_t GetBlock(std::string BlockName, float *block, int64_t npart_toread, int64_t start_part, int skip_type){*/
/*%apply (float* IN_ARRAY1, int DIMS1) {(float *block, int64_t npart_toread,)};*/
