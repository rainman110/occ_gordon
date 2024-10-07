/* 
* SPDX-License-Identifier: Apache-2.0
* SPDX-FileCopyrightText: 2013 German Aerospace Center (DLR)
*
* Created: 2010-08-13 Markus Litz <Markus.Litz@dlr.de>
*/
/**
* @file
* @brief  Exception class used to throw occ_gordon exceptions.
*/

#include "occ_gordon.h"

#include "internal/InterpolateCurveNetwork.h"
#include "internal/Error.h"

namespace occ_gordon
{

Handle(Geom_BSplineSurface) interpolate_curve_network(const std::vector<Handle (Geom_Curve)> &ucurves,
                                                      const std::vector<Handle (Geom_Curve)> &vcurves,
                                                      double tolerance)
{
    try {
        occ_gordon_internal::InterpolateCurveNetwork interpolator(ucurves, vcurves, tolerance);
        return interpolator.Surface();
    }
    catch(occ_gordon_internal::error& err) {
        throw std::runtime_error(std::string("Error creating gordon surface: ") + err.what());
    }
}

} // end namespace occ_gordon
