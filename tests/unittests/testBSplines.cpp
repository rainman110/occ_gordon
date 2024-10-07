/*
* SPDX-License-Identifier: Apache-2.0
* SPDX-FileCopyrightText: 2015 German Aerospace Center (DLR)
*
* Created: 2015-06-01 Martin Siggel <Martin.Siggel@dlr.de>
*/

#include <gtest/gtest.h>

#include "testUtils.h"

#include "internal/Error.h"

#include <Geom_BSplineCurve.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include "internal/BSplineApproxInterp.h"

#include <BRepTools.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Compound.hxx>
#include <internal/BSplineAlgorithms.h>
#include <cmath>
#include <math_Matrix.hxx>
#include "internal/PointsToBSplineInterpolation.h"

namespace
{

void from_string(const std::string& s, double& t)
{

    t = std::stod(s);
}

inline std::vector<std::string> split_string(const std::string& mystring, char delimiter)
{
    std::vector<std::string> strings;
    std::istringstream f(mystring);
    std::string s;
    while (std::getline(f, s, delimiter)) {
        strings.push_back(s);
    }
    return strings;
}

}

// tests the bspline basis matrix function with derivative = 0
TEST(BSplines, bSplineMatDeriv0)
{
    TColStd_Array1OfReal knots(1, 6);
    knots.SetValue(1, 0.);
    knots.SetValue(2, 0.);
    knots.SetValue(3, 0.);
    knots.SetValue(4, 1.);
    knots.SetValue(5, 1.);
    knots.SetValue(6, 1.);

    TColStd_Array1OfReal params(1, 4);
    params.SetValue(1, 0.);
    params.SetValue(2, 1. / 3.);
    params.SetValue(3, 2. / 3.);
    params.SetValue(4, 1.);
    math_Matrix A = occ_gordon_internal::BSplineAlgorithms::bsplineBasisMat(2, knots, params);

    // compare with python implementation
    EXPECT_NEAR(A.Value(1,1), 1., 1e-10);
    EXPECT_NEAR(A.Value(1,2), 0., 1e-10);
    EXPECT_NEAR(A.Value(1,3), 0., 1e-10);

    EXPECT_NEAR(A.Value(2,1), 4.444444444444445308e-01, 1e-10);
    EXPECT_NEAR(A.Value(2,2), 4.444444444444444753e-01, 1e-10);
    EXPECT_NEAR(A.Value(2,3), 1.111111111111111049e-01, 1e-10);

    EXPECT_NEAR(A.Value(3,1), 1.111111111111111327e-01, 1e-10);
    EXPECT_NEAR(A.Value(3,2), 4.444444444444444753e-01, 1e-10);
    EXPECT_NEAR(A.Value(3,3), 4.444444444444444198e-01, 1e-10);

    EXPECT_NEAR(A.Value(4,1), 0., 1e-10);
    EXPECT_NEAR(A.Value(4,2), 0., 1e-10);
    EXPECT_NEAR(A.Value(4,3), 1., 1e-10);
}

// tests the bspline basis matrix function with derivative = 1
TEST(BSplines, bSplineMatDeriv1)
{
    TColStd_Array1OfReal knots(1, 6);
    knots.SetValue(1, 0.);
    knots.SetValue(2, 0.);
    knots.SetValue(3, 0.);
    knots.SetValue(4, 1.);
    knots.SetValue(5, 1.);
    knots.SetValue(6, 1.);

    TColStd_Array1OfReal params(1, 4);
    params.SetValue(1, 0.);
    params.SetValue(2, 1. / 3.);
    params.SetValue(3, 2. / 3.);
    params.SetValue(4, 1.);
    math_Matrix A = occ_gordon_internal::BSplineAlgorithms::bsplineBasisMat(2, knots, params, 1);

    // compare with python implementation
    EXPECT_NEAR(A.Value(1,1), -2., 1e-10);
    EXPECT_NEAR(A.Value(1,2), 2., 1e-10);
    EXPECT_NEAR(A.Value(1,3), 0., 1e-10);

    EXPECT_NEAR(A.Value(2,1), -1.333333333333333481e+00, 1e-10);
    EXPECT_NEAR(A.Value(2,2), 6.666666666666668517e-01, 1e-10);
    EXPECT_NEAR(A.Value(2,3), 6.666666666666666297e-01, 1e-10);

    EXPECT_NEAR(A.Value(3,1), -6.666666666666667407e-01, 1e-10);
    EXPECT_NEAR(A.Value(3,2), -6.666666666666665186e-01, 1e-10);
    EXPECT_NEAR(A.Value(3,3), 1.333333333333333259e+00, 1e-10);

    EXPECT_NEAR(A.Value(4,1), 0., 1e-10);
    EXPECT_NEAR(A.Value(4,2), -2., 1e-10);
    EXPECT_NEAR(A.Value(4,3), 2., 1e-10);
}

class BSplineInterpolation : public ::testing::Test
{
protected:
    BSplineInterpolation()
        : pnts(1, 101)
    {}

    void SetUp() override
    {
        // load in the airfoil point example
        std::string line;
        std::ifstream file ("TestData/airfoil_points.txt");
        ASSERT_TRUE(file.is_open());
        Standard_Integer irow = 1;
        while(!file.eof()) {
            std::getline(file, line);
            std::vector<std::string> cols = split_string(line, ' ');
            if (cols.size() != 8) {
                continue;
            }
            double px, py, pz, t;
            from_string(cols[0], px);
            from_string(cols[2], py);
            from_string(cols[4], pz);
            from_string(cols[7], t);
            pnts.SetValue(irow, gp_Pnt(px, py, pz));
            parms.push_back(t);
            irow++;
        }
        ASSERT_EQ(parms.size(), pnts.Length());
    }

    void TearDown() override
    {
    }

    std::vector<double> parms;
    TColgp_Array1OfPnt pnts;
};

TEST_F(BSplineInterpolation, approxAndInterpolate)
{
    occ_gordon_internal::BSplineApproxInterp app(pnts, 30, 3);
    app.InterpolatePoint(0);
    app.InterpolatePoint(50);
    app.InterpolatePoint(100);
    occ_gordon_internal::ApproxResult result = app.FitCurve(parms);
    // Value from different splinelib implementation
    EXPECT_NEAR(0.01317089, result.error, 1e-5);
    EXPECT_NEAR(0.0, result.curve->Value(parms[0]).Distance(pnts.Value(1)), 1e-10);
    EXPECT_NEAR(0.0, result.curve->Value(parms[50]).Distance(pnts.Value(51)), 1e-10);
    EXPECT_NEAR(0.0, result.curve->Value(parms[100]).Distance(pnts.Value(101)), 1e-10);

    result = app.FitCurveOptimal(parms);
    EXPECT_NEAR(0.000393704, result.error, 1e-5);
    EXPECT_NEAR(0.0, result.curve->Value(parms[0]).Distance(pnts.Value(1)), 1e-10);
    EXPECT_NEAR(0.0, result.curve->Value(parms[50]).Distance(pnts.Value(51)), 1e-10);
    EXPECT_NEAR(0.0, result.curve->Value(parms[100]).Distance(pnts.Value(101)), 1e-10);

    StoreResult("TestData/analysis/BSplineInterpolation-approxAndInterpolate.brep", result.curve, pnts);
}

// tests whether the approximation of a given unit circle is C2 continuous at the closing without interpolating any points
TEST_F(BSplineInterpolation, approxAndInterpolateContinuous1)
{
    int nPoints = 21;
    TColgp_Array1OfPnt pnt2(1, nPoints);

    for (int i = 0; i < nPoints; ++i) {
        pnt2.SetValue(i + 1, gp_Pnt(cos((i * 2.*M_PI) / static_cast<double>(nPoints - 1)),
                                    0.,
                                    sin((i * 2.*M_PI) / static_cast<double>(nPoints - 1))));
    }
    
    occ_gordon_internal::BSplineApproxInterp app(pnt2, 9, 3, true);
    occ_gordon_internal::ApproxResult result = app.FitCurve();
    Handle(Geom_BSplineCurve) curve = result.curve;

    // tests if closed curve is C2-continuous at the boundaries:
    gp_Pnt p1;
    gp_Vec deriv1_1;
    gp_Vec deriv1_2;
    curve->D2(0., p1, deriv1_1, deriv1_2);
    gp_Pnt p2;
    gp_Vec deriv2_1;
    gp_Vec deriv2_2;
    curve->D2(1., p2, deriv2_1, deriv2_2);

    EXPECT_TRUE(p1.IsEqual(p2, 1e-10));
    EXPECT_TRUE(deriv1_1.IsEqual(deriv2_1, 1e-10, 1e-10));
    EXPECT_TRUE(deriv1_2.IsEqual(deriv2_2, 1e-10, 1e-10));

    StoreResult("TestData/analysis/BSplineInterpolation-approxAndInterpolateContinuous1.brep", curve, pnt2);
}

// tests whether the approximation of a given unit circle is C2 continuous at the closing with interpolating the first and the last point
TEST_F(BSplineInterpolation, approxAndInterpolateContinuous2)
{
    int nPoints = 21;
    TColgp_Array1OfPnt pnt2(1, nPoints);

    for (int i = 0; i < nPoints; ++i) {
        pnt2.SetValue(i + 1, gp_Pnt(cos((i * 2.*M_PI) / static_cast<double>(nPoints - 1)),
                                    0.,
                                    sin((i * 2.*M_PI) / static_cast<double>(nPoints - 1))));
    }
    
    occ_gordon_internal::BSplineApproxInterp app(pnt2, 9, 3, true);
    app.InterpolatePoint(0);
    app.InterpolatePoint(nPoints - 1);
    occ_gordon_internal::ApproxResult result = app.FitCurve();
    Handle(Geom_BSplineCurve) curve = result.curve;

    // tests if closed curve is C2-continuous at the boundaries:
    gp_Pnt p1;
    gp_Vec deriv1_1;
    gp_Vec deriv1_2;
    curve->D2(0., p1, deriv1_1, deriv1_2);
    gp_Pnt p2;
    gp_Vec deriv2_1;
    gp_Vec deriv2_2;
    curve->D2(1., p2, deriv2_1, deriv2_2);

    EXPECT_TRUE(p1.IsEqual(p2, 1e-10));
    EXPECT_TRUE(deriv1_1.IsEqual(deriv2_1, 1e-10, 1e-10));
    EXPECT_TRUE(deriv1_2.IsEqual(deriv2_2, 1e-10, 1e-10));

    StoreResult("TestData/analysis/BSplineInterpolation-approxAndInterpolateContinuous2.brep", curve, pnt2);
}

// tests whether the BSplineApproxInterp method works also for a non-closed part of a circle
TEST_F(BSplineInterpolation, approxAndInterpolateContinuous3)
{
    int nPoints = 21;
    TColgp_Array1OfPnt pnt2(1, nPoints - 1);

    for (int i = 0; i < nPoints - 1; ++i) {
        pnt2.SetValue(i + 1, gp_Pnt(cos((i * 2.*M_PI) / static_cast<double>(nPoints - 1)),
                                    0.,
                                    sin((i * 2.*M_PI) / static_cast<double>(nPoints - 1))));
    }
    
    occ_gordon_internal::BSplineApproxInterp app(pnt2, 9, 3, true);
    occ_gordon_internal::ApproxResult result = app.FitCurve();
    Handle(Geom_BSplineCurve) curve = result.curve;

    StoreResult("TestData/analysis/BSplineInterpolation-approxAndInterpolateContinuous3.brep", curve, pnt2);
}

TEST_F(BSplineInterpolation, interpolateAll)
{
    int nPoints = 8;
    TColgp_Array1OfPnt pnt2(1, nPoints);

    double dx = 2. * M_PI / (nPoints - 1);
    for (int i = 0; i < nPoints; ++i) {
        pnt2.SetValue(i + 1, gp_Pnt(cos(i*dx),
                                    0.,
                                    sin(i*dx)));
    }
    
    occ_gordon_internal::BSplineApproxInterp app(pnt2, nPoints, 2, false);
    for (int i = 0; i < nPoints; ++i) {
        app.InterpolatePoint(static_cast<size_t>(i));
    }
    
    occ_gordon_internal::ApproxResult result = app.FitCurve();
    Handle(Geom_BSplineCurve) curve = result.curve;

    StoreResult("TestData/analysis/BSplineInterpolation-interpolateAll.brep", curve, pnt2);
}

TEST_F(BSplineInterpolation, interpolateAllContinuous)
{
    int nPoints = 8;
    TColgp_Array1OfPnt pnt2(1, nPoints);

    double dx = 2. * M_PI / (nPoints - 1);
    for (int i = 0; i < nPoints; ++i) {
        pnt2.SetValue(i + 1, gp_Pnt(cos(i*dx),
                                    0.,
                                    sin(i*dx)));
    }

    // We need two more control points than interpolation points since we force c2 continuity
    occ_gordon_internal::BSplineApproxInterp app(pnt2, nPoints + 2, 2, true);
    for (int i = 0; i < nPoints; ++i) {
        app.InterpolatePoint(static_cast<size_t>(i));
    }
    
    occ_gordon_internal::ApproxResult result = app.FitCurve();
    Handle(Geom_BSplineCurve) curve = result.curve;

    StoreResult("TestData/analysis/BSplineInterpolation-interpolateAllCont.brep", curve, pnt2);
}

TEST_F(BSplineInterpolation, interpolateAllContinuousHalfCircle)
{
    int nPoints = 8;
    TColgp_Array1OfPnt pnt2(1, nPoints);

    double dx = 2.*M_PI / (nPoints - 1);
    for (int i = 0; i < nPoints; ++i) {
        pnt2.SetValue(i + 1, gp_Pnt(cos(i*dx),
                                    0.,
                                    sin(i*dx)));
    }

    // We need two more control points than interpolation points since we force c2 continuity
    occ_gordon_internal::BSplineApproxInterp app(pnt2, nPoints + 2, 2, true);
    for (int i = 0; i < nPoints; ++i) {
        app.InterpolatePoint(static_cast<size_t>(i));
    }
    
    occ_gordon_internal::ApproxResult result = app.FitCurve();
    Handle(Geom_BSplineCurve) curve = result.curve;

    StoreResult("TestData/analysis/BSplineInterpolation-interpolateAllCont.brep", curve, pnt2);
}

TEST_F(BSplineInterpolation, interpolateErrors)
{
    int nPoints = 8;
    TColgp_Array1OfPnt pnt2(1, nPoints);

    double dx = 2.*M_PI / (nPoints - 1);
    for (int i = 0; i < nPoints; ++i) {
        pnt2.SetValue(i + 1, gp_Pnt(cos(i*dx),
                                    0.,
                                    sin(i*dx)));
    }
    
    
    occ_gordon_internal::BSplineApproxInterp app(pnt2, nPoints, 2, true);
    for (int i = 0; i < nPoints; ++i) {
        app.InterpolatePoint(static_cast<size_t>(i));
    }

    // too few control points
    ASSERT_THROW(app.FitCurve(), occ_gordon_internal::error);
    
    occ_gordon_internal::BSplineApproxInterp app2(pnt2, nPoints + 1, 2, false);
    for (int i = 0; i < nPoints; ++i) {
        app2.InterpolatePoint(static_cast<size_t>(i));
    }

    // too many control points
    ASSERT_THROW(app2.FitCurve(), occ_gordon_internal::error);
}


TEST_F(BSplineInterpolation, approxOnly)
{
    occ_gordon_internal::BSplineApproxInterp app(pnts, 15, 3);
    occ_gordon_internal::ApproxResult result = app.FitCurve(parms);
    // Value from different splinelib implementation
    EXPECT_NEAR(0.01898, result.error, 1e-5);

    result = app.FitCurveOptimal(parms);
    EXPECT_NEAR(0.00238, result.error, 1e-5);

    StoreResult("TestData/analysis/BSplineInterpolation-approxOnly.brep", result.curve, pnts);
}

TEST_F(BSplineInterpolation, gordonIssue)
{
    occ_gordon_internal::BSplineApproxInterp app(pnts, 31, 3);
    app.InterpolatePoint(0);
    app.InterpolatePoint(100);
    occ_gordon_internal::ApproxResult result = app.FitCurve(parms);
    EXPECT_NEAR(0.0, result.curve->Value(parms[0]).Distance(pnts.Value(1)), 1e-10);
    EXPECT_NEAR(0.0, result.curve->Value(parms[100]).Distance(pnts.Value(101)), 1e-10);
    EXPECT_NEAR(0.0055298, result.error, 1e-5);

    StoreResult("TestData/analysis/BSplineInterpolation-gordonIssue.brep", result.curve, pnts);
}

TEST_F(BSplineInterpolation, ownParms)
{
    occ_gordon_internal::BSplineApproxInterp app(pnts, 31, 3);
    app.InterpolatePoint(0);
    app.InterpolatePoint(100);
    occ_gordon_internal::ApproxResult result = app.FitCurve(parms);
    result = app.FitCurveOptimal();
    EXPECT_NEAR(0.0, result.curve->Value(parms[0]).Distance(pnts.Value(1)), 1e-10);
    EXPECT_NEAR(0.0, result.curve->Value(parms[100]).Distance(pnts.Value(101)), 1e-10);

    StoreResult("TestData/analysis/BSplineInterpolation-ownParms.brep", result.curve, pnts);
}

TEST_F(BSplineInterpolation, tipKink)
{
    occ_gordon_internal::BSplineApproxInterp app(pnts, 31, 3);
    app.InterpolatePoint(0);
    app.InterpolatePoint(50, true);
    app.InterpolatePoint(100);
    occ_gordon_internal::ApproxResult result = app.FitCurveOptimal();
    EXPECT_NEAR(0.0, result.curve->Value(parms[0]).Distance(pnts.Value(1)), 1e-10);
    EXPECT_NEAR(0.0, result.curve->Value(parms[100]).Distance(pnts.Value(101)), 1e-10);

    StoreResult("TestData/analysis/BSplineInterpolation-tipKink.brep", result.curve, pnts);
}

TEST_F(BSplineInterpolation, tipKink2)
{
    std::vector<double> parms2;
    parms2.push_back(0. / 4.);
    parms2.push_back(0.5 / 4.);
    parms2.push_back(1.0 / 4.);
    parms2.push_back(1.5 / 4.);
    parms2.push_back(2.0 / 4.);
    parms2.push_back(2.5 / 4.);
    parms2.push_back(3.0 / 4.);
    parms2.push_back(3.5 / 4.);
    parms2.push_back(4.0 / 4.);

    TColgp_Array1OfPnt pnt2(1, 9);
    pnt2.SetValue(1, gp_Pnt(0., 0., 0.));
    pnt2.SetValue(2, gp_Pnt(0.5, 0., -0.2));
    pnt2.SetValue(3, gp_Pnt(1.0, 0., 0.));
    pnt2.SetValue(4, gp_Pnt(0.8, 0., 0.5));
    pnt2.SetValue(5, gp_Pnt(1.0, 0., 1.0));
    pnt2.SetValue(6, gp_Pnt(0.5, 0., 1.2));
    pnt2.SetValue(7, gp_Pnt(0.0, 0., 1.0));
    pnt2.SetValue(8, gp_Pnt(0.2, 0., 0.5));
    pnt2.SetValue(9, gp_Pnt(0., 0., 0.));
    
    occ_gordon_internal::BSplineApproxInterp app(pnt2, 4, 2);
    app.InterpolatePoint(0);
    app.InterpolatePoint(2, true);
    app.InterpolatePoint(4, true);
    app.InterpolatePoint(6, true);
    app.InterpolatePoint(8);
    occ_gordon_internal::ApproxResult result = app.FitCurve(parms2);

    StoreResult("TestData/analysis/BSplineInterpolation-tipKink2.brep", result.curve, pnt2);
}

TEST_F(BSplineInterpolation, interpolationContinuous)
{

    Handle(TColgp_HArray1OfPnt) pnt2 = new TColgp_HArray1OfPnt(1, 13);
    pnt2->SetValue(1, gp_Pnt(-0.5, 0., 0.5));
    pnt2->SetValue(2, gp_Pnt(-0.5, 0., 1.5));
    pnt2->SetValue(3, gp_Pnt(0.5, 0., 1.5));
    pnt2->SetValue(4, gp_Pnt(0.5, 0., 0.5));
    pnt2->SetValue(5, gp_Pnt(1.5, 0., 0.5));
    pnt2->SetValue(6, gp_Pnt(1.5, 0., -0.5));
    pnt2->SetValue(7, gp_Pnt(0.5, 0., -0.5));
    pnt2->SetValue(8, gp_Pnt(0.5, 0., -3.5));
    pnt2->SetValue(9, gp_Pnt(-0.5, 0., -3.5));
    pnt2->SetValue(10, gp_Pnt(-0.5, 0., -0.5));
    pnt2->SetValue(11, gp_Pnt(-1.5, 0., -0.5));
    pnt2->SetValue(12, gp_Pnt(-1.5, 0., 0.5));
    pnt2->SetValue(13, gp_Pnt(-0.5, 0., 0.5));

    for (int degree = 1; degree <= 4; ++degree) {

        occ_gordon_internal::PointsToBSplineInterpolation app(pnt2, degree, true);
        Handle(Geom_BSplineCurve) result = app.Curve();

        // test interpolation accuracy
        const std::vector<double>& params = app.Parameters();
        for (int iparm = 0; iparm < params.size(); ++iparm) {
            gp_Pnt p = result->Value(params[iparm]);
            EXPECT_NEAR(0., p.Distance(pnt2->Value(iparm + 1)), 1e-10);
        }

        std::stringstream str;
        str << "TestData/analysis/BSplineInterpolation-interpolationContinuousDegree" << degree << ".brep";
        StoreResult(str.str(), result, pnt2->Array1());
    }
}

TEST_F(BSplineInterpolation, interpolationDiscontinuous)
{

    Handle(TColgp_HArray1OfPnt) pnt2 = new TColgp_HArray1OfPnt(1, 12);
    pnt2->SetValue(1, gp_Pnt(-0.5, 0., 0.5));
    pnt2->SetValue(2, gp_Pnt(-0.5, 0., 1.5));
    pnt2->SetValue(3, gp_Pnt(0.5, 0., 1.5));
    pnt2->SetValue(4, gp_Pnt(0.5, 0., 0.5));
    pnt2->SetValue(5, gp_Pnt(1.5, 0., 0.5));
    pnt2->SetValue(6, gp_Pnt(1.5, 0., -0.5));
    pnt2->SetValue(7, gp_Pnt(0.5, 0., -0.5));
    pnt2->SetValue(8, gp_Pnt(0.5, 0., -3.5));
    pnt2->SetValue(9, gp_Pnt(-0.5, 0., -3.5));
    pnt2->SetValue(10, gp_Pnt(-0.5, 0., -0.5));
    pnt2->SetValue(11, gp_Pnt(-1.5, 0., -0.5));
    pnt2->SetValue(12, gp_Pnt(-1.5, 0., 0.5));


    for (int degree = 1; degree <= 4; ++degree) {

        occ_gordon_internal::PointsToBSplineInterpolation app(pnt2, degree, true);
        Handle(Geom_BSplineCurve) result = app.Curve();

        // test interpolation accuracy
        const std::vector<double>& params = app.Parameters();
        for (int iparm = 0; iparm < params.size(); ++iparm) {
            gp_Pnt p = result->Value(params[iparm]);
            EXPECT_NEAR(0., p.Distance(pnt2->Value(iparm + 1)), 1e-10);
        }

        std::stringstream str;
        str << "TestData/analysis/BSplineInterpolation-interpolationDiscontinuousDegree" << degree << ".brep";
        StoreResult(str.str(), result, pnt2->Array1());
    }
}

TEST_F(BSplineInterpolation, interpolationLinear)
{

    Handle(TColgp_HArray1OfPnt) pnt2 = new TColgp_HArray1OfPnt(1, 2);
    pnt2->SetValue(1, gp_Pnt(-0.5, 0., 0.5));
    pnt2->SetValue(2, gp_Pnt(-0.5, 0., 1.5));


    int degree = 3;

    occ_gordon_internal::PointsToBSplineInterpolation app(pnt2, degree, true);
    Handle(Geom_BSplineCurve) result = app.Curve();

    // test interpolation accuracy
    const std::vector<double>& params = app.Parameters();
    for (int iparm = 0; iparm < params.size(); ++iparm) {
        gp_Pnt p = result->Value(params[iparm]);
        EXPECT_NEAR(0., p.Distance(pnt2->Value(iparm + 1)), 1e-10);
    }

    std::stringstream str;
    str << "TestData/analysis/BSplineInterpolation-interpolationLinear.brep";
    StoreResult(str.str(), result, pnt2->Array1());

}

TEST_F(BSplineInterpolation, interpolationClosedIssue1)
{

    Handle(TColgp_HArray1OfPnt) pnt2 = new TColgp_HArray1OfPnt(1, 5);

    pnt2->SetValue(1, gp_Pnt(-428.410051, 0.000000, 869.975281));
    pnt2->SetValue(2, gp_Pnt(-310.053449, -937.418377, -247.074247));
    pnt2->SetValue(3, gp_Pnt(-224.887685, -28.428971, -1041.352947));
    pnt2->SetValue(4, gp_Pnt(-358.904397, 904.415830, 218.280826));
    pnt2->SetValue(5, gp_Pnt(-428.410051, -0.000000, 869.975291));

    std::vector<double> params;
    params.push_back(0.);
    params.push_back(0.28329579);
    params.push_back(0.49656342);
    params.push_back(0.80054936);
    params.push_back(1.0);

    unsigned int degree = 3;

    occ_gordon_internal::PointsToBSplineInterpolation app(pnt2, params, degree, true);
    Handle(Geom_BSplineCurve) curve = app.Curve();

    EXPECT_EQ(5, curve->NbKnots());
    EXPECT_EQ(3, curve->Degree());

    // check first and second derivative
    EXPECT_NEAR(0., (curve->DN(0., 1) - curve->DN(1., 1)).Magnitude(), 1e-10);
    EXPECT_NEAR(0., (curve->DN(0., 2) - curve->DN(1., 2)).Magnitude(), 1e-10);

    std::stringstream str;
    str << "TestData/analysis/BSplineInterpolation-interpolationClosedIssue1.brep";
    StoreResult(str.str(), curve, pnt2->Array1());
}
