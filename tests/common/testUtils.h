/* 
* SPDX-License-Identifier: Apache-2.0
* SPDX-FileCopyrightText: 2014 German Aerospace Center (DLR)
*
* Created: 2014-02-17 Tobias Stollenwerk <Tobias.Stollenwerk@dlr.de>
*/
/**
* @file
* @brief Helper functions for tests
*/

#ifndef TESTUTILS_H
#define TESTUTILS_H

#include<fstream>

#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>
// class TColgp_Array1OfPnt;

// save x-y data
void outputXY(const int & i, const double& x, const double&y, const std::string& filename);
void outputXYVector(const int& i, const double& x, const double& y, const double& vx, const double& vy, const std::string& filename);
void StoreResult(const std::string& filename, const Handle(Geom_BSplineCurve)& curve, const TColgp_Array1OfPnt& pt);

Handle(Geom_BSplineSurface) LoadBSplineSurface(const std::string& filename);
Handle(Geom_BSplineCurve) LoadBSplineCurve(const std::string& filename);

#endif // TESTUTILS_H

