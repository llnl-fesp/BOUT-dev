/**************************************************************************
 * Flux-coordinate Independent parallel derivatives
 *
 **************************************************************************
 * Copyright 2014 B.D.Dudson, P. Hill
 *
 * Contact: Ben Dudson, bd512@york.ac.uk
 *
 * This file is part of BOUT++.
 *
 * BOUT++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * BOUT++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with BOUT++.  If not, see <http://www.gnu.org/licenses/>.
 *
 **************************************************************************/

#ifndef __FCI_DERIVS_H__
#define __FCI_DERIVS_H__

#include <field3d.hxx>
#include <bout/mesh.hxx>
#include <globals.hxx>
#include <utils.hxx>
#include <bout_types.hxx> // See this for codes
#include <vector>
#include <bout/constants.hxx>
#include <field_factory.hxx>
#include <fci_boundary_region.hxx>

// Field line map - contains the coefficients for interpolation
class FCIMap {
  // Private constructor - must be initialised with mesh
  FCIMap();
public:
  typedef std::vector<std::vector<std::vector<bool> > > B3vec;

  // dir MUST be either +1 or -1
  FCIMap(Mesh& mesh, int dir, bool yperiodic, bool zperiodic);

  // Direction of map
  int dir;

  int*** i_corner;      // x-index of bottom-left grid point
  int*** k_corner;      // z-index of bottom-left grid point
  B3vec x_boundary;     // boundary mask - has the field line left the domain through the x-sides
  B3vec y_boundary;     // boundary mask - has the field line left the domain through the y-sides
  B3vec z_boundary;     // boundary mask - has the field line left the domain through the z-sides
  Field3D y_prime;		// distance to intersection with boundary

  BoundaryRegionFCI* boundary;			/**< boundary region */

  // Basis functions for cubic Hermite spline interpolation
  //	see http://en.wikipedia.org/wiki/Cubic_Hermite_spline
  // The h00 and h01 basis functions are applied to the function itself
  // and the h10 and h11 basis functions are applied to its derivative
  // along the interpolation direction.

  Field3D h00_x;
  Field3D h01_x;
  Field3D h10_x;
  Field3D h11_x;
  Field3D h00_z;
  Field3D h01_z;
  Field3D h10_z;
  Field3D h11_z;
};

// A class for performing flux-coordinate independent parallel derivatives
class FCI {
private:
  // The maps hold the grid indices of the field line end-points and
  // associated interpolation coefficients
  const FCIMap forward_map;
  const FCIMap backward_map;

  // The FCI object is tied to the mesh it was created on, so the mesh must
  // not change
  Mesh& mesh;

  // Is the y-direction periodic?
  bool yperiodic;

  // Is the z-direction periodic?
  bool zperiodic;

  // Private constructor - must be initialised with mesh
  FCI();
public:
  enum BndryType { DIRICHLET, NEUMANN };

  FCI(Mesh& m) : mesh(m),
				 forward_map(m, +1, true, true),
				 backward_map(m, -1, true, true),
				 yperiodic(true),
				 zperiodic(true) {}
  FCI(Mesh& m, bool yperiodic, bool zperiodic) : 
	mesh(m),
	forward_map(m, +1, yperiodic, zperiodic),
	backward_map(m, -1, yperiodic, zperiodic),
	yperiodic(yperiodic),
	zperiodic(zperiodic) {}

  // Interpolate field in direction DIR
  void interpolate(Field3D &f, Field3D &f_next, const FCIMap &fcimap);

  // Parallel derivatives
  const Field3D Grad_par(Field3D &f, BndryType boundary, FieldGenerator* gen, BoutReal t, bool keep = false);
  const Field3D Grad2_par2(Field3D &f, BndryType boundary, FieldGenerator* gen, BoutReal t, bool keep = false);
  const Field3D Div_par(Field3D &f, BndryType boundary, FieldGenerator* gen, BoutReal t, bool keep = false);

  // Boundary conditions
  void dirichletBC(Field3D &f, Field3D &f_next, const FCIMap &fcimap, FieldGenerator* gen, BoutReal t);
  void neumannBC(Field3D &f, Field3D &f_next, const FCIMap &fcimap);

  void applyBoundary(Field3D &f, FieldGenerator* upvalue, FieldGenerator* downvalue);
  void applyBoundary(Field3D &f, FieldGenerator* value);
};

#endif // __FCI_DERIVS_H__
