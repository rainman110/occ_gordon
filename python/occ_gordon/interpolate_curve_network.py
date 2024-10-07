# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)

import occ_gordon.occ_gordon_native as occg_native

def geomcurve_vector(geomcurve_list):
    """
    Creates a std::vector of Handle(Geom_Curve) to be passed
    to different geometry algorithms of geoml

    :param geomcurve_list: list of Geom_Curve
    :return: VectorHandleGeom_Curve
    """

    vec = occg_native.CurveList()
    for curve in geomcurve_list:
        vec.push_back(curve)

    return vec

def interpolate_curve_network(profiles, guides, tolerance=1e-4):
    """
    Interpolates a network of curves with a B-spline surface.
    Internally, this is done with a Gordon surface.

    Gordon surfaces normally require the input curves to have
    compatible intersections (i.e. intersections at the same parameter).
    Here, we overcome this limitation by re-parametrization of the
    input curves.

    :param profiles: List of profiles (List of Geom_Curves)
    :param guides: List of guides (List of Geom_Curves)
    :param tolerance: Maximum allowed distance between each guide and profile
                     (in theory they must intersect and the distance is zero)

    :return: The final surface (Geom_BSplineSurface)
    """
    return occg_native.interpolate_curve_network(geomcurve_vector(profiles),
                                            geomcurve_vector(guides),
                                            tolerance)
