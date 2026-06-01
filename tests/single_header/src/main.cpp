#include "occ_gordon_single.hpp"

#include <Geom_Line.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <vector>

namespace
{

Handle(Geom_Curve) make_line(double x1, double y1, double z1,
                             double x2, double y2, double z2)
{
    return new Geom_Line(gp_Pnt(x1, y1, z1), gp_Dir(gp_Vec(gp_Pnt(x1, y1, z1), gp_Pnt(x2, y2, z2))));
}

} // namespace

int main()
{
    const std::vector<Handle(Geom_Curve)> profiles{
        make_line(0.0, 0.0, 0.0, 0.0, 1.0, 0.0),
        make_line(1.0, 0.0, 0.0, 1.0, 1.0, 0.0),
    };
    const std::vector<Handle(Geom_Curve)> guides{
        make_line(0.0, 0.0, 0.0, 1.0, 0.0, 0.0),
        make_line(0.0, 1.0, 0.0, 1.0, 1.0, 0.0),
    };

    const Handle(Geom_BSplineSurface) surface =
        occ_gordon::interpolate_curve_network(profiles, guides, 1e-6);

    return surface.IsNull() ? 1 : 0;
}
