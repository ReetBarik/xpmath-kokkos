// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC

// Compile smoke: include exactly one wrapper header and instantiate its type.
#include <Kokkos_xpmath/ff_math.hpp>

int main() {
  Kokkos::Experimental::FloatFloat x{};
  (void)x;
  return 0;
}
