# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2024 German Aerospace Center (DLR)

from OCC.Core.BRep import BRep_Tool
from OCC.Display.SimpleGui import init_display

from occ_gordon import interpolate_curve_network
from occ_gordon.occ_helpers.topology import read_brep, iter_edges

import os

base_dir = os.path.dirname(os.path.abspath(__file__))

# load curves from files
print("Reading in curve network from disk")
guides_shape = read_brep(os.path.join(base_dir, "data", "wing_guides.brep"))
profiles_shape = read_brep(os.path.join(base_dir, "data", "wing_profiles.brep"))

# Extract curves from the topods_shapes
guide_curves = [BRep_Tool.Curve(edge)[0] for edge in iter_edges(guides_shape)]
profile_curves = [BRep_Tool.Curve(edge)[0] for edge in iter_edges(profiles_shape)]

# create the gordon surface
print("Compute curve network interpolation")
surface = interpolate_curve_network(profile_curves, guide_curves, 1.e-5)

# display curves and resulting surface
print("Show the result")
display, start_display, add_menu, add_function_to_menu = init_display()
display.Context.SetDeviationCoefficient(0.0001)

for curve in guide_curves:
    display.DisplayShape(curve)

for curve in profile_curves:
    display.DisplayShape(curve)

display.DisplayShape(surface)
display.FitAll()

start_display()
