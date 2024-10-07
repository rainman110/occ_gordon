/*
* SPDX-License-Identifier: Apache-2.0
* SPDX-FileCopyrightText: 2015 German Aerospace Center (DLR)
*
* Created: 2015-11-23 Martin Siggel <Martin.Siggel@dlr.de>
*/

%module (package="occ_gordon") occ_gordon_native

%include common.i

%{
#include <TColStd_module.hxx>
#include <TCollection_module.hxx>
#include <TColgp_module.hxx>
#include <Geom_module.hxx>
%}

%import Geom.i

%{
#include <occ_gordon/occ_gordon.h>
%}

%feature("autodoc", "3");

%catch_exceptions()

%template(CurveList) std::vector<Handle(Geom_Curve)>;

%include "occ_gordon/occ_gordon.h"



