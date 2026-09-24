// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// Kokkos::Experimental compatibility wrapper. Numerics live under
// vendor/xpmath/xp/ at the recorded upstream tag and retain their upstream
// SPDX identifiers (see NOTICE.md). This file is only aliases and one-line
// Kokkos:: forwards — no new arithmetic.

#pragma once

#include <Kokkos_xpmath/dd_math.hpp>
#include <Kokkos_xpmath/dd_complex.hpp>
#include <Kokkos_xpmath/ff_math.hpp>
#include <Kokkos_xpmath/ff_complex.hpp>
#include <Kokkos_xpmath/qf_math.hpp>
#include <Kokkos_xpmath/qf_complex.hpp>
#include <Kokkos_xpmath/tf_math.hpp>
#include <Kokkos_xpmath/tf_complex.hpp>
