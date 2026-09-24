// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC

// Gate: Kokkos::real / Kokkos::imag on each complex type. Compile-only; not ctest.
#include <Kokkos_xpmath/dd_complex.hpp>
#include <Kokkos_xpmath/ff_complex.hpp>
#include <Kokkos_xpmath/qf_complex.hpp>
#include <Kokkos_xpmath/tf_complex.hpp>

int main() {
  {
    Kokkos::Experimental::DoubleDoubleComplex z{};
    auto r = Kokkos::real(z); (void)r;
    auto i = Kokkos::imag(z); (void)i;
  }
  {
    Kokkos::Experimental::FloatFloatComplex z{};
    auto r = Kokkos::real(z); (void)r;
    auto i = Kokkos::imag(z); (void)i;
  }
  {
    Kokkos::Experimental::QuadFloatComplex z{};
    auto r = Kokkos::real(z); (void)r;
    auto i = Kokkos::imag(z); (void)i;
  }
  {
    Kokkos::Experimental::TripleFloatComplex z{};
    auto r = Kokkos::real(z); (void)r;
    auto i = Kokkos::imag(z); (void)i;
  }
  return 0;
}
