/*
* SPDX-License-Identifier: Apache-2.0
* SPDX-FileCopyrightText: 2017 German Aerospace Center (DLR)
*
* Created: 2017-05-24 Merlin Pelz <Merlin.Pelz@dlr.de>
*/

#include "IntersectBSplines.h"
#include "BSplineAlgorithms.h"
#include "CurvesToSurface.h"
#include "Error.h"
#include "BSplineApproxInterp.h"
#include "PointsToBSplineInterpolation.h"

#include "occ_gordon_internal.h"
#include "occ_std_adapters.h"

#include <Standard_Version.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <GeomConvert.hxx>
#include <Geom2dAPI_Interpolate.hxx>
#include <GeomAPI_ExtremaCurveCurve.hxx>
#include <TColStd_Array2OfReal.hxx>
#include <TColStd_HArray1OfReal.hxx>
#include <TColStd_HArray1OfInteger.hxx>
#include <TColgp_HArray1OfPnt.hxx>
#include <TColgp_HArray1OfPnt2d.hxx>
#include <TColgp_Array1OfPnt2d.hxx>
#include <TColgp_HArray2OfPnt.hxx>
#include <BSplCLib.hxx>
#include <GeomAPI_PointsToBSpline.hxx>
#include <BRepTools.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <Precision.hxx>
#include <Geom2dAPI_ProjectPointOnCurve.hxx>
#include <GCPnts_AbscissaPoint.hxx>

#include <cmath>
#include <algorithm>
#include <cassert>

namespace
{
    struct IsInsideTolerance
    {
        IsInsideTolerance(double value, double tolerance = 1e-15)
            : _a(value), _tol(tolerance)
        {}

        bool operator()(double v)
        {
            return (fabs(_a - v) <= _tol);
        }

        double _a;
        double _tol;
    };


    std::vector<double> LinspaceWithBreaks(double umin, double umax, size_t n_values, const std::vector<double>& breaks)
    {
        double du = (umax - umin) / static_cast<double>(n_values - 1);

        std::vector<double> result(n_values);
        for (int i = 0; i < n_values; ++i) {
            result[i] = i * du + umin;
        }

        // now insert the break

        double eps = 0.3;
        // remove points, that are closer to each break point than du*eps
        for (std::vector<double>::const_iterator it = breaks.begin(); it != breaks.end(); ++it) {
            double breakpoint = *it;
            std::vector<double>::iterator pos = std::find_if(result.begin(), result.end(), IsInsideTolerance(breakpoint, du*eps));
            if (pos != result.end()) {
                // point found, replace it
                *pos = breakpoint;
            }
            else {
                // find closest element
                pos = std::find_if(result.begin(), result.end(), IsInsideTolerance(breakpoint, (0.5 + 1e-8)*du));

                if (*pos > breakpoint) {
                    result.insert(pos, breakpoint);
                }
                else {
                    result.insert(pos+1, breakpoint);
                }
            }
        }

        return result;
    }

    class helper_function_unique
    {
    public:
        helper_function_unique(double tolerance = 1e-15)
            : _tol(tolerance)
        {}

        // helper function for std::unique
        bool operator()(double a, double b)
        {
            return (fabs(a - b) < _tol);
        }
    private:
        double _tol;
    };
    
    enum SurfAdapterDir
    {
        udir = 0,
        vdir = 1
    };
    
    class SurfAdapterView
    {
    public:
        SurfAdapterView(Handle(Geom_BSplineSurface) surf, SurfAdapterDir dir)
            : _surf(surf), _dir(dir)
        {
        }

        void insertKnot(double knot, int mult, double tolerance=1e-15)
        {
            if (_dir == udir) {
                _surf->InsertUKnot(knot, mult, tolerance, false);
            }
            else {
                _surf->InsertVKnot(knot, mult, tolerance, false);
            }
        }

        double getKnot(int idx) const
        {
            if (_dir == udir) {
                return _surf->UKnot(idx);
            }
            else {
                return _surf->VKnot(idx);
            }
        }

        int getMult(int idx) const
        {
            if (_dir == udir) {
                return _surf->UMultiplicity(idx);
            }
            else {
                return _surf->VMultiplicity(idx);
            }
        }

        int getNKnots() const
        {
            if (_dir == udir) {
                return _surf->NbUKnots();
            }
            else {
                return _surf->NbVKnots();
            }
        }

        int getDegree() const
        {
            if (_dir == udir) {
                return _surf->UDegree();
            }
            else {
                return _surf->VDegree();
            }
        }
        
        void setDir(SurfAdapterDir dir)
        {
            _dir = dir;
        }

        operator const Handle(Geom_BSplineSurface)&() const
        {
            return _surf;
        }
    private:
        Handle(Geom_BSplineSurface) _surf;
        SurfAdapterDir _dir;
    };
    
    class CurveAdapterView
    {
    public:
        CurveAdapterView(Handle(Geom_BSplineCurve) curve)
            : _curve(curve)
        {
        }

        void insertKnot(double knot, int mult, double tolerance=1e-15)
        {
            _curve->InsertKnot(knot, mult, tolerance, false);
        }

        double getKnot(int idx) const
        {
            return _curve->Knot(idx);
        }

        int getMult(int idx) const
        {
            return _curve->Multiplicity(idx);
        }

        int getNKnots() const
        {
            return _curve->NbKnots();
        }

        int getDegree() const
        {
            return _curve->Degree();
        }

        operator const Handle(Geom_BSplineCurve)&() const
        {
            return _curve;
        }
 
    private:
        Handle(Geom_BSplineCurve) _curve;
    };

    template <class SplineAdapter>
    bool haveSameRange(const std::vector<SplineAdapter>& splines_vector, double par_tolerance)
    {
        double begin_param_dir = splines_vector[0].getKnot(1);
        double end_param_dir = splines_vector[0].getKnot(splines_vector[0].getNKnots());
        for (unsigned int spline_idx = 1; spline_idx < splines_vector.size(); ++spline_idx) {
            const SplineAdapter& curSpline = splines_vector[spline_idx];
            double begin_param_dir_surface = curSpline.getKnot(1);
            double end_param_dir_surface = curSpline.getKnot(curSpline.getNKnots());
            if (std::abs(begin_param_dir_surface - begin_param_dir) > par_tolerance || std::abs(end_param_dir_surface - end_param_dir) > par_tolerance) {
                return false;
            }
        }
        return true;
    }

    template <class SplineAdapter>
    bool haveSameDegree(const std::vector<SplineAdapter>& splines)
    {
        int degree = splines[0].getDegree();
        for (unsigned int splineIdx = 1; splineIdx < splines.size(); ++splineIdx) {
            if (splines[splineIdx].getDegree() != degree) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief createCommonKnotsVectorImpl:
     *          Creates a common knot vector in u- or v-direction of the given vector of B-splines
     *          The common knot vector contains all knots in u- or v-direction of all splines with the highest multiplicity of all splines.
     * @param old_splines_vector:
     *          the given vector of B-spline splines that could have a different knot vector in u- or v-direction
     */
    template <class SplineAdapter>
    void makeGeometryCompatibleImpl(std::vector<SplineAdapter>& splines_vector, double par_tolerance)
    {
        // all B-spline splines must have the same parameter range in the chosen direction
        if (!haveSameRange(splines_vector, par_tolerance)) {
            throw occ_gordon_internal::error("B-splines don't have the same parameter range at least in one direction (u / v) in method createCommonKnotsVectorImpl!", occ_gordon_internal::MATH_ERROR);
        }

        // all B-spline splines must have the same degree in the chosen direction
        if (!haveSameDegree(splines_vector)) {
            throw occ_gordon_internal::error("B-splines don't have the same degree at least in one direction (u / v) in method createCommonKnotsVectorImpl!", occ_gordon_internal::MATH_ERROR);
        }

        // The parametric tolerance must be smaller than half of the minimum knot distance
        for (typename std::vector<SplineAdapter>::const_iterator splineIt = splines_vector.begin(); splineIt != splines_vector.end(); ++splineIt) {
            const SplineAdapter& spline = *splineIt;
            for (int iKnot = 1; iKnot < spline.getNKnots(); ++iKnot) {
                double knotDist = spline.getKnot(iKnot+1) - spline.getKnot(iKnot);
                par_tolerance = std::min(par_tolerance, knotDist / 2.);
            }
        }

        // insert all knots in first spline
        SplineAdapter& firstSpline = splines_vector[0];
        for (typename std::vector<SplineAdapter>::const_iterator splineIt = splines_vector.begin()+1; splineIt != splines_vector.end(); ++splineIt) {
            const SplineAdapter& spline = *splineIt;
            for (int knot_idx = 2; knot_idx < spline.getNKnots(); ++knot_idx) {
                double knot = spline.getKnot(knot_idx);
                int mult = spline.getMult(knot_idx);
                firstSpline.insertKnot(knot, mult, par_tolerance);
            }
        }


        // now insert knots from first into all others
        for (typename std::vector<SplineAdapter>::iterator splineIt = splines_vector.begin()+1; splineIt != splines_vector.end(); ++splineIt) {
            SplineAdapter& spline = *splineIt;
            for (int knot_idx = 2; knot_idx < firstSpline.getNKnots(); ++knot_idx) {
                double knot = firstSpline.getKnot(knot_idx);
                int mult = firstSpline.getMult(knot_idx);
                spline.insertKnot(knot, mult, par_tolerance);
            }
            if (spline.getNKnots() != firstSpline.getNKnots()) {
                throw occ_gordon_internal::error("Unexpected error in Algorithm makeGeometryCompatibleImpl.\nPlease contact the developers.");
            }
        }


    } // makeGeometryCompatibleImpl
    
    template <class OccMatrix, class OccVector, class OccHandleVector>
    OccHandleVector array2GetColumn(const OccMatrix& matrix, int colIndex)
    {
        OccHandleVector colVector =  new OccVector(matrix.LowerRow(), matrix.UpperRow());

        for (int rowIdx = matrix.LowerRow(); rowIdx <= matrix.UpperRow(); ++rowIdx) {
            colVector->SetValue(rowIdx, matrix(rowIdx, colIndex));
        }

        return colVector;
    }
    
    template <class OccMatrix, class OccVector, class OccHandleVector>
    OccHandleVector array2GetRow(const OccMatrix& matrix, int rowIndex)
    {
        OccHandleVector rowVector = new OccVector(matrix.LowerCol(), matrix.UpperCol());
        
        for (int colIdx = matrix.LowerCol(); colIdx <= matrix.UpperCol(); ++colIdx) {
            rowVector->SetValue(colIdx, matrix(rowIndex, colIdx));
        }
        
        return rowVector;
    }

    Handle_TColgp_HArray1OfPnt pntArray2GetColumn(const TColgp_Array2OfPnt& matrix, int colIndex)
    {
        return array2GetColumn<TColgp_Array2OfPnt, TColgp_HArray1OfPnt, Handle_TColgp_HArray1OfPnt>(matrix, colIndex);
    }

    Handle_TColgp_HArray1OfPnt pntArray2GetRow(const TColgp_Array2OfPnt& matrix, int rowIndex)
    {
        return array2GetRow<TColgp_Array2OfPnt, TColgp_HArray1OfPnt, Handle_TColgp_HArray1OfPnt>(matrix, rowIndex);
    }
    		
} // namespace


namespace occ_gordon_internal
{

const double BSplineAlgorithms::REL_TOL_CLOSED = 1e-8;
const double BSplineAlgorithms::PAR_CHECK_TOL = 1e-5;

bool BSplineAlgorithms::isUDirClosed(const TColgp_Array2OfPnt& points, double tolerance)
{
    bool uDirClosed = true;
    int ulo = points.LowerRow();
    int uhi = points.UpperRow();
    // check that first row and last row are the same
    for (int v_idx = points.LowerCol(); v_idx <= points.UpperCol(); ++v_idx) {
        gp_Pnt pfirst = points.Value(ulo, v_idx);
        gp_Pnt pLast = points.Value(uhi, v_idx);
        uDirClosed = uDirClosed & pfirst.IsEqual(pLast, tolerance);
    }
    return uDirClosed;
}

bool BSplineAlgorithms::isVDirClosed(const TColgp_Array2OfPnt& points, double tolerance)
{
    bool vDirClosed = true;
    int vlo = points.LowerCol();
    int vhi = points.UpperCol();
    for (int u_idx = points.LowerRow(); u_idx <= points.UpperRow(); ++u_idx) {
        vDirClosed = vDirClosed & points.Value(u_idx, vlo).IsEqual(points.Value(u_idx, vhi), tolerance);
    }
    return vDirClosed;
}

std::vector<double> BSplineAlgorithms::knotsFromCurveParameters(std::vector<double> &params, unsigned int degree, bool closedCurve)
{
    if (params.size() < 2) {
        throw error("Parameters must contain two or more elements.");
    }

    size_t nCP = params.size();
    if (closedCurve) {
        // For each continuity condition, we have to add one control point
        nCP += degree - 1;
    }
    size_t nInnerKnots = nCP - degree + 1;

    std::vector<double> innerKnots(nInnerKnots);
    innerKnots.front() = params.front();
    innerKnots.back() = params.back();

    std::vector<double> knots;

    if (closedCurve && degree % 2 == 0) {
        size_t m = params.size() - 2;

        // build difference vector
        std::vector<double> dparm(m + 1, 0.);
        for (size_t iparm = 0; iparm <= m; ++iparm) {
            dparm[iparm] = (params[iparm+1] - params[iparm]);
        }

        innerKnots[1] = innerKnots[0] + 0.5 * (dparm[0] + dparm[m]);
        for (size_t iparm = 1; iparm < m; ++iparm) {
            innerKnots[iparm + 1] = innerKnots[iparm] + 0.5 * (dparm[iparm-1] + dparm[iparm]);
        }

        // shift parameters
        for (size_t iparm = 0; iparm < params.size(); ++iparm) {
            params[iparm] += dparm[m] / 2.;
        }

    }
    else if (closedCurve) {
        assert(innerKnots.size() == params.size());
        innerKnots = params;
    }
    else {
        // averaging
        for (size_t j = 1; j < params.size() - degree; ++j) {
            double sum = 0.;
            // average
            for (size_t i = j; i <= j + degree - 1; ++i) {
                sum += params[i];
            }

            innerKnots[j] = sum / static_cast<double>(degree);
        }
    }

    if (closedCurve) {
        double offset = innerKnots[0] - innerKnots[nInnerKnots -1];
        for (size_t iknot = 0; iknot < degree; ++iknot) {
            knots.push_back(offset + innerKnots[nInnerKnots - degree -1 + iknot]);
        }
        for (size_t iknot = 0; iknot < nInnerKnots; ++iknot) {
            knots.push_back(innerKnots[iknot]);
        }
        for (size_t iknot = 0; iknot < degree; ++iknot) {
            knots.push_back(-offset + innerKnots[iknot + 1]);
        }
    }
    else {
        for (size_t iknot = 0; iknot < degree; ++iknot) {
            knots.push_back(innerKnots[0]);
        }
        for (size_t iknot = 0; iknot < nInnerKnots; ++iknot) {
            knots.push_back(innerKnots[iknot]);
        }
        for (size_t iknot = 0; iknot < degree; ++iknot) {
            knots.push_back(innerKnots[nInnerKnots - 1]);
        }
    }

    if (closedCurve && degree <= 1) {
        size_t nKnots = knots.size();
        knots[0] = knots[1];
        knots[nKnots - 1] = knots[nKnots - 2];
    }

    return knots;
}

std::vector<Handle(Geom_BSplineCurve)> BSplineAlgorithms::toBSplines(const std::vector<Handle(Geom_Curve)>& curves)
{
    std::vector<Handle(Geom_BSplineCurve)> result;

    std::transform(curves.begin(), curves.end(), std::back_inserter(result), [](const auto& curve) {
        return  GeomConvert::CurveToBSplineCurve(curve);
    });

    return result;
}

double BSplineAlgorithms::scale(const TColgp_Array2OfPnt& points)
{
    double theScale = 0.;
    for (int uidx = points.LowerRow(); uidx <= points.UpperRow(); ++uidx) {
        gp_Pnt pFirst = points.Value(uidx, points.LowerCol());
        for (int vidx = points.LowerCol() + 1; vidx <= points.UpperCol(); ++vidx) {
            double dist = pFirst.Distance(points.Value(uidx, vidx));
            theScale = std::max(theScale, dist);
        }
    }
    return theScale;
}

double BSplineAlgorithms::scale(const TColgp_Array1OfPnt& points)
{
    double theScale = 0.;

    for (int i = points.Lower(); i <= points.Upper(); ++i) {
        for (int j = i + 1; j < points.Upper(); ++j) {
            double dist = points.Value(i).Distance(points.Value(j));
            theScale = std::max(theScale, dist);
        }
    }
    return theScale;
}

std::vector<double> BSplineAlgorithms::computeParamsBSplineCurve(const Handle(TColgp_HArray1OfPnt)& points, const double alpha)
{
    return computeParamsBSplineCurve(points, 0., 1., alpha);
}

std::vector<double> BSplineAlgorithms::computeParamsBSplineCurve(const Handle(TColgp_HArray1OfPnt)& points, double umin, double umax, const double alpha)
{
    if ( umax <= umin ) {
        throw error("The specified start parameter is larger than the specified end parameter");
    }

    std::vector<double> parameters(static_cast<size_t>(points->Length()));

    parameters[0] = 0.;

    for (size_t i = 1; i < parameters.size(); ++i) {
        int iArray = static_cast<int>(i) + points->Lower();
        double length = pow(points->Value(iArray).SquareDistance(points->Value(iArray - 1)), alpha / 2.);
        parameters[i] = parameters[i - 1] + length;
    }

    double totalLength = parameters.back();


    for (size_t i = 0; i < parameters.size(); ++i) {
        double ratio = 0.;
        if (totalLength < 1e-10) {
            ratio = static_cast<double>(i) / static_cast<double>(parameters.size()-1);
        }
        else {
            ratio = parameters[i] / totalLength;
        }
        parameters[i] = (umax - umin) * ratio + umin;
    }

    return parameters;
}

std::pair<std::vector<double>, std::vector<double> >
BSplineAlgorithms::computeParamsBSplineSurf(const TColgp_Array2OfPnt& points, double alpha)
{
    // first for parameters in u-direction:
    std::vector<double> paramsU(static_cast<size_t>(points.ColLength()), 0.);
    for (int vIdx = points.LowerCol(); vIdx <= points.UpperCol(); ++vIdx) {
        std::vector<double> parameters_u_line = computeParamsBSplineCurve(pntArray2GetColumn(points, vIdx), alpha);

        // average over columns
        for (size_t uIdx = 0; uIdx < parameters_u_line.size(); ++uIdx) {
            paramsU[uIdx] += parameters_u_line[uIdx]/(double)points.RowLength();
        }
    }


    // now for parameters in v-direction:
    std::vector<double> paramsV(static_cast<size_t>(points.RowLength()), 0.);
    for (int uIdx = points.LowerRow(); uIdx <= points.UpperRow(); ++uIdx) {
        std::vector<double> parameters_v_line = computeParamsBSplineCurve(pntArray2GetRow(points, uIdx), alpha);

        // average over rows
        for (size_t vIdx = 0; vIdx < parameters_v_line.size(); ++vIdx) {
            paramsV[vIdx] += parameters_v_line[vIdx]/(double)points.ColLength();
        }
    }

    // put computed parameters for both u- and v-direction in output tuple
    return std::make_pair(paramsU, paramsV);

}


std::vector<Handle(Geom_BSplineCurve)> BSplineAlgorithms::createCommonKnotsVectorCurve(const std::vector<Handle(Geom_BSplineCurve)>& splines_vector, double tol)
{
    // Match parameter range
    matchParameterRange(splines_vector, tol);

    // Create a copy that we can modify
    std::vector<CurveAdapterView> splines_adapter;
    for (size_t i = 0; i < splines_vector.size(); ++i) {
        splines_adapter.push_back(Handle(Geom_BSplineCurve)::DownCast(splines_vector[i]->Copy()));
    }

    makeGeometryCompatibleImpl(splines_adapter, tol);

    return std::vector<Handle(Geom_BSplineCurve)>(splines_adapter.begin(), splines_adapter.end());
}

std::vector<Handle(Geom_BSplineSurface) > BSplineAlgorithms::createCommonKnotsVectorSurface(const std::vector<Handle(Geom_BSplineSurface) >& old_surfaces_vector, SurfaceDirection dir)
{
    // Create a copy that we can modify
    std::vector<SurfAdapterView> adapterSplines;
    for (size_t i = 0; i < old_surfaces_vector.size(); ++i) {
        adapterSplines.push_back(SurfAdapterView(Handle(Geom_BSplineSurface)::DownCast(old_surfaces_vector[i]->Copy()), udir));
    }

    if (dir == SurfaceDirection::u || dir == SurfaceDirection::both) {
        // first in u direction
        makeGeometryCompatibleImpl(adapterSplines, 1e-14);
    }

    if (dir == SurfaceDirection::v || dir == SurfaceDirection::both) {
         // now in v direction
        for (size_t i = 0; i < old_surfaces_vector.size(); ++i) adapterSplines[i].setDir(vdir);
        makeGeometryCompatibleImpl(adapterSplines, 1e-14);
    }

    return std::vector<Handle(Geom_BSplineSurface)>(adapterSplines.begin(), adapterSplines.end());
}

void BSplineAlgorithms::matchParameterRange(std::vector<Handle(Geom_BSplineCurve)> const& bsplines, double tolerance)
{
    Standard_Real umin = bsplines[0]->FirstParameter();
    Standard_Real umax = bsplines[0]->LastParameter();
    for (unsigned iP=1; iP<bsplines.size(); ++iP) {
        Handle(Geom_BSplineCurve) bspl = bsplines[iP];
        if (fabs(bspl->FirstParameter() - umin) > tolerance ||
            fabs(bspl->LastParameter() - umax) > tolerance ) {
            reparametrizeBSpline(*bspl, umin, umax, tolerance);
        }
    }
}

void BSplineAlgorithms::matchDegree(const std::vector<Handle(Geom_BSplineCurve) >& bsplines)
{
    int maxDegree = 0;
    for (std::vector<Handle(Geom_BSplineCurve) >::const_iterator it = bsplines.begin(); it != bsplines.end(); ++it) {
        int curDegree = (*it)->Degree();
        if (curDegree > maxDegree) {
            maxDegree = curDegree;
        }
    }

    for (std::vector<Handle(Geom_BSplineCurve) >::const_iterator it = bsplines.begin(); it != bsplines.end(); ++it) {
        int curDegree = (*it)->Degree();
        if (curDegree < maxDegree) {
            (*it)->IncreaseDegree(maxDegree);
        }
    }
}

ApproxResult BSplineAlgorithms::reparametrizeBSplineContinuouslyApprox(const Handle(Geom_BSplineCurve) spline,
                                                                                 const std::vector<double>& old_parameters,
                                                                                 const std::vector<double>& new_parameters,
                                                                                 size_t n_control_pnts)
{
    if (old_parameters.size() != new_parameters.size()) {
        throw error("parameter sizes dont match");
    }

    // create a B-spline as a function for reparametrization
    Handle(TColgp_HArray1OfPnt2d) old_parameters_pnts = new TColgp_HArray1OfPnt2d(1, static_cast<Standard_Integer>(old_parameters.size()));
    for (size_t parameter_idx = 0; parameter_idx < old_parameters.size(); ++parameter_idx) {
        int occIdx = static_cast<int>(parameter_idx + 1);
        old_parameters_pnts->SetValue(occIdx, gp_Pnt2d(old_parameters[parameter_idx], 0));
    }

    Geom2dAPI_Interpolate interpolationObject(old_parameters_pnts, OccFArray(new_parameters), false, 1e-15);
    interpolationObject.Perform();

    // check that interpolation was successful
    if (!interpolationObject.IsDone()) {
        throw error("Cannot reparametrize", MATH_ERROR);
    }

    Handle(Geom2d_BSplineCurve) reparametrizing_spline = interpolationObject.Curve();

    // Create a vector of parameters including the intersection parameters
    std::vector<double> breaks;
    for (size_t ipar = 1; ipar < new_parameters.size() - 1; ++ipar) {
        breaks.push_back(new_parameters[ipar]);
    }

    double par_tol = 1e-10;

#define MODEL_KINKS
#ifdef MODEL_KINKS
    // remove kinks from breaks
    std::vector<double> kinks = BSplineAlgorithms::getKinkParameters(spline);
    // convert kink parameters into reparametrized parameter using the
    // inverse reparametrization function
    for (size_t ikink = 0; ikink < kinks.size(); ++ikink) {
        kinks[ikink] = Geom2dAPI_ProjectPointOnCurve(gp_Pnt2d(kinks[ikink], 0.), reparametrizing_spline)
                           .LowerDistanceParameter();
    }

    for (size_t ikink = 0; ikink < kinks.size(); ++ikink) {
        double kink = kinks[ikink];
        std::vector<double>::iterator it = std::find_if(breaks.begin(), breaks.end(), IsInsideTolerance(kink, par_tol));
        if (it != breaks.end()) {
            breaks.erase(it);
        }
    }
#endif

    // create equidistance array of parameters, including the breaks
    std::vector<double> parameters = LinspaceWithBreaks(new_parameters.front(),
                                                        new_parameters.back(),
                                                        std::max(static_cast<size_t>(101), n_control_pnts*2),
                                                        breaks);
#ifdef MODEL_KINKS
    // insert kinks into parameters array at the correct position
    for (size_t ikink = 0; ikink < kinks.size(); ++ikink) {
        double kink = kinks[ikink];
        parameters.insert( 
            std::upper_bound( parameters.begin(), parameters.end(), kink),
            kink);
    }
#endif

    // Compute points on spline at the new parameters
    // Those will be approximated later on
    TColgp_Array1OfPnt points(1, static_cast<Standard_Integer>(parameters.size()));
    for (size_t i = 1; i <= parameters.size(); ++i) {
        double oldParameter = reparametrizing_spline->Value(parameters[i-1]).X();
        points(static_cast<Standard_Integer>(i)) = spline->Value(oldParameter);
    }

    bool makeContinuous = spline->IsClosed() &&
            spline->DN(spline->FirstParameter(), 1).Angle(spline->DN(spline->LastParameter(), 1)) < 6. / 180. * M_PI;

    // Create the new spline as a interpolation of the old one
    BSplineApproxInterp approximationObj(points, static_cast<int>(n_control_pnts), 3, makeContinuous);

    breaks.insert(breaks.begin(), new_parameters.front());
    breaks.push_back(new_parameters.back());
    // Interpolate points at breaking parameters (required for gordon surface)
    for (size_t ibreak = 0; ibreak < breaks.size(); ++ibreak) {
        double thebreak = breaks[ibreak];
        size_t idx = static_cast<size_t>(
            std::find_if(parameters.begin(), parameters.end(), IsInsideTolerance(thebreak)) -
            parameters.begin());
        approximationObj.InterpolatePoint(idx);
    }

#ifdef MODEL_KINKS
    for (size_t ikink = 0; ikink < kinks.size(); ++ikink) {
        double kink = kinks[ikink];
        size_t idx = static_cast<size_t>(
            std::find_if(parameters.begin(), parameters.end(), IsInsideTolerance(kink, par_tol)) -
            parameters.begin());
        approximationObj.InterpolatePoint(idx, true);
    }
#endif
    
    ApproxResult result = approximationObj.FitCurveOptimal(parameters);

    assert(!result.curve.IsNull());

    return result;
}

Handle(Geom_BSplineSurface) BSplineAlgorithms::flipSurface(const Handle(Geom_BSplineSurface) surface)
{
    Handle(Geom_BSplineSurface) result = Handle(Geom_BSplineSurface)::DownCast(surface->Copy());
    result->ExchangeUV();
    return result;
}

Handle(Geom_BSplineSurface) BSplineAlgorithms::pointsToSurface(const TColgp_Array2OfPnt& points,
                                                                    const std::vector<double>& uParams,
                                                                    const std::vector<double>& vParams,
                                                                    bool uContinuousIfClosed, bool vContinuousIfClosed)
{

    double tolerance = REL_TOL_CLOSED * scale(points);
    bool makeVDirClosed = vContinuousIfClosed & isVDirClosed(points, tolerance);
    bool makeUDirClosed = uContinuousIfClosed & isUDirClosed(points, tolerance);

    // first interpolate all points by B-splines in u-direction
    std::vector<Handle(Geom_Curve)> uSplines;
    for (int cpVIdx = points.LowerCol(); cpVIdx <= points.UpperCol(); ++cpVIdx) {
        Handle_TColgp_HArray1OfPnt points_u = pntArray2GetColumn(points, cpVIdx);
        PointsToBSplineInterpolation interpolationObject(points_u, uParams, 3, makeUDirClosed);

        Handle(Geom_Curve) curve = interpolationObject.Curve();
        uSplines.push_back(curve);
    }

    // now create a skinned surface with these B-splines which represents the interpolating surface
    CurvesToSurface skinner(uSplines, vParams, makeVDirClosed );
    Handle(Geom_BSplineSurface) interpolatingSurf = skinner.Surface();

    return interpolatingSurf;
}


std::vector<std::pair<double, double> > BSplineAlgorithms::intersections(const Handle(Geom_BSplineCurve) spline1, const Handle(Geom_BSplineCurve) spline2, double tolerance) {

    // find out the average scale of the two B-splines in order to being able to handle a more approximate curves and find its intersections
    double splines_scale = (BSplineAlgorithms::scale(spline1) + BSplineAlgorithms::scale(spline2)) / 2.;

    std::vector<std::pair<double, double> > intersection_params_vector;

    auto results = IntersectBSplines(spline1, spline2, tolerance*splines_scale);
    for (const auto& r : results) {
        intersection_params_vector.push_back({r.parmOnCurve1, r.parmOnCurve2});
    }

    return intersection_params_vector;
}

double BSplineAlgorithms::scale(const std::vector<Handle(Geom_BSplineCurve)>& splines_vector)
{
    double maxScale = 0.;
    for (std::vector<Handle(Geom_BSplineCurve)>::const_iterator it = splines_vector.begin(); it != splines_vector.end(); ++it) {
        maxScale = std::max(scale(*it), maxScale);
    }

    return maxScale;
}

double BSplineAlgorithms::scale(const Handle(Geom_BSplineCurve)& spline)
{
    double scale = 0.;
    gp_Pnt first_ctrl_pnt = spline->Pole(1);
    for (int ctrl_pnt_idx = 2; ctrl_pnt_idx <= spline->NbPoles(); ++ctrl_pnt_idx) {
        // compute distance of the first control point to the others and save biggest distance
        double distance = first_ctrl_pnt.Distance(spline->Pole(ctrl_pnt_idx));

        scale = std::max(scale, distance);
    }
    return scale;
}

void BSplineAlgorithms::reparametrizeBSpline(Geom_BSplineCurve& spline, double umin, double umax, double tol)
{
    if (std::abs(spline.Knot(1) - umin) > tol || std::abs(spline.Knot(spline.NbKnots()) - umax) > tol) {
        TColStd_Array1OfReal aKnots (1, spline.NbKnots());
        spline.Knots (aKnots);
        BSplCLib::Reparametrize (umin, umax, aKnots);
        spline.SetKnots (aKnots);
    }
}

math_Matrix BSplineAlgorithms::bsplineBasisMat(int degree, const TColStd_Array1OfReal& knots, const TColStd_Array1OfReal& params, unsigned int derivOrder)
{
    Standard_Integer ncp = knots.Length() - degree - 1;
    math_Matrix mx(1, params.Length(), 1, ncp);
    mx.Init(0.);
    math_Matrix bspl_basis(1, derivOrder + 1, 1, degree + 1);
    bspl_basis.Init(0.);
    for (Standard_Integer iparm = 1; iparm <= params.Length(); ++iparm) {
        Standard_Integer basis_start_index = 0;
#if OCC_VERSION_HEX >= VERSION_HEX_CODE(7,1,0)
        BSplCLib::EvalBsplineBasis(derivOrder, degree + 1, knots, params.Value(iparm), basis_start_index, bspl_basis);
#else
        BSplCLib::EvalBsplineBasis(1, derivOrder, degree + 1, knots, params.Value(iparm), basis_start_index, bspl_basis);
#endif
        if(derivOrder > 0) {
            math_Vector help_vector(1, ncp);
            help_vector.Init(0.);
            help_vector.Set(basis_start_index, basis_start_index + degree, bspl_basis.Row(derivOrder + 1));
            mx.SetRow(iparm, help_vector);
        }
        else {
            mx.Set(iparm, iparm, basis_start_index, basis_start_index + degree, bspl_basis);
        }
    }
    return mx;
}

std::vector<double> BSplineAlgorithms::getKinkParameters(const Handle(Geom_BSplineCurve)& curve)
{
    if (curve.IsNull()) {
        throw error("Null Pointer curve", NULL_POINTER);
    }

    double eps = 1e-8;

    std::vector<double> kinks;
    for (int knotIndex = 2; knotIndex < curve->NbKnots(); ++knotIndex) {
        if (curve->Multiplicity(knotIndex) == curve->Degree()) {
            double knot = curve->Knot(knotIndex);
            // check if really a kink
            double angle = curve->DN(knot + eps, 1).Angle(curve->DN(knot - eps, 1));
            if (angle > 6./180. * M_PI) {
                kinks.push_back(knot);
            }
        }
    }

    return kinks;
}

BSplineAlgorithms::SurfaceKinks BSplineAlgorithms::getKinkParameters(const Handle(Geom_BSplineSurface)& surface)
{
    if (surface.IsNull()) {
        throw error("Null Pointer curve", NULL_POINTER);
    }

    SurfaceKinks kinks;

    for (int knotIndex = 2; knotIndex < surface->NbUKnots(); ++knotIndex) {
        if (surface->UMultiplicity(knotIndex) == surface->UDegree()) {
            double knot = surface->UKnot(knotIndex);
            kinks.u.push_back(knot);
        }
    }

    for (int knotIndex = 2; knotIndex < surface->NbVKnots(); ++knotIndex) {
        if (surface->VMultiplicity(knotIndex) == surface->VDegree()) {
            double knot = surface->VKnot(knotIndex);
            kinks.v.push_back(knot);
        }
    }

    return kinks;
}


Handle(Geom_BSplineCurve) BSplineAlgorithms::trimCurve(const Handle(Geom_BSplineCurve)& curve, double umin, double umax)
{
    Handle(Geom_BSplineCurve) copy = Handle(Geom_BSplineCurve)::DownCast(curve->Copy());
    copy->Segment(umin, umax);
    return copy;
}

} // namespace occ_gordon_internal
