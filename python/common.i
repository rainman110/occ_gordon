/*
* SPDX-License-Identifier: Apache-2.0
* SPDX-FileCopyrightText: 2015 German Aerospace Center (DLR)
*
* Created: 2015-11-20 Martin Siggel <Martin.Siggel@dlr.de>
*/

%include std_string.i
%include std_vector.i

%include std_shared_ptr.i

#define OCC_GORDON_EXPORT

#ifdef PYTHONNOCC_LEGACY
#define Handle(ClassName)  Handle_##ClassName
#endif

%{
#include <occ_gordon/occ_gordon.h>
%}


%feature("director:except") {
    if ($error != NULL) {
        throw Swig::DirectorMethodException();
    }
}

%define %catch_exceptions()

%exception {
    try {
        $action
    }
    catch (std::exception& err) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char*>(err.what()));
        SWIG_fail;
    }
    catch(...) {
        PyErr_SetString(PyExc_RuntimeError, const_cast<char*>("An unknown error occurred!"));
        SWIG_fail;
    }
}

%enddef
