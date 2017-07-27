///
///  Finite-volume discretisation methods. Flux-conservative form
///
/// NOTE: EXPERIMENTAL

#ifndef __FV_OPS_H__
#define __FV_OPS_H__

#include "../field3d.hxx"
#include "../vector2d.hxx"

namespace FV {
  const Field3D Div_n_bxGrad_f_B_XPPM(const Field3D &n, const Field3D &f);
  
  // Div ( a Laplace_perp(x) )  -- Vorticity
  const Field3D Div_a_Laplace_perp(const Field3D &a, const Field3D &x);
  
  // Divergence of a parallel diffusion Div( k * Grad_par(f) )
  const Field3D Div_par_K_Grad_par(const Field3D &k, const Field3D &f, bool bndry_flux=true);
}

#endif // __FV_OPS_H__
