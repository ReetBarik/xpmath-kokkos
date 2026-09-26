// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// K5 — bit-identity: the Kokkos wrapper changes nothing.
//
// THE ONLY QUESTION THIS TEST ASKS
// Does a value computed through Kokkos::Experimental:: / Kokkos:: equal, bit
// for bit (raw limb patterns, including NaN payloads), the value xp:: computes
// directly? No oracle. No ulps. No digits. No tolerances.
//
// EXECUTION-SPACE PAIRING (load-bearing — do not "fix")
//
//   Serial (login-node ctest): wrapper inside Kokkos::parallel_for versus
//   the core called directly on the host. Same machine.
//
//   Cuda / HIP (Cobalt A100 / MI250 jobs): BOTH calls inside the same
//   kernel — wrapper-on-device versus core-on-device. Never compare a device
//   wrapper result with a host core result. A device value may legitimately
//   differ from the host; that difference is xpmath's DEVICE_PRECISION
//   measurement and is forbidden here.
//
// Complex values are the standalone xp::*Complex /
// Kokkos::Experimental::*Complex structs. There is no Kokkos::complex<T>
// path (see docs/COMPLEX_INTEROP.md).
//
// On the first differing cell: print limb patterns and exit nonzero. Do not
// patch the wrapper or the vendor tree to make it pass.

#include <Kokkos_xpmath/Kokkos_xpmath.hpp>

#include "sweep_inputs.hpp"
#include "sweep_ops_core.hpp"
#include "sweep_ops_wrap.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

#ifndef XPMATH_KOKKOS_SWEEP_GRID
#error "XPMATH_KOKKOS_SWEEP_GRID must be defined to the path of tests/data/sweep_grid.csv"
#endif

namespace {

// Device spaces: both evaluations inside one kernel. Serial: wrapper in the
// parallel_for, core on the host after. Detected from the default space.
#if defined(KOKKOS_ENABLE_CUDA) || defined(KOKKOS_ENABLE_HIP) || \
    defined(KOKKOS_ENABLE_SYCL)
constexpr bool kDeviceBothInKernel = !std::is_same_v<Kokkos::DefaultExecutionSpace,
                                                     Kokkos::Serial>;
#else
constexpr bool kDeviceBothInKernel = false;
#endif

using exec_space = Kokkos::DefaultExecutionSpace;

struct FailInfo {
  int backend = -1;  // 0=DD,1=FF,2=QF,3=TF
  int kind = -1;     // 0=real,1=complex
  int op = -1;
  int point = -1;
  // Raw bytes of wrap vs core (max sizeof complex QF / DD = 32).
  unsigned char wrap_bytes[32]{};
  unsigned char core_bytes[32]{};
  int nbytes = 0;
  int found = 0;  // 0/1 flag written from device
};

KOKKOS_INLINE_FUNCTION void store_bytes(void const* p, int n,
                                        unsigned char* out) {
  auto const* b = static_cast<unsigned char const*>(p);
  for (int i = 0; i < n; ++i) out[i] = b[i];
}

KOKKOS_INLINE_FUNCTION bool bytes_equal(unsigned char const* a,
                                        unsigned char const* b, int n) {
  for (int i = 0; i < n; ++i) {
    if (a[i] != b[i]) return false;
  }
  return true;
}

void print_hex(unsigned char const* b, int n) {
  for (int i = 0; i < n; ++i) std::printf("%02x", b[i]);
}

char const* backend_name(int b) {
  static char const* names[] = {"DD", "FF", "QF", "TF"};
  return (b >= 0 && b < 4) ? names[b] : "?";
}

char const* op_name(int kind, int op) {
  if (kind == 0) return xpsweep::kReal[op].name;
  return xpsweep::kComplex[op].name;
}

// ---------------------------------------------------------------------------
// Real: one kernel per (backend, op). Compile-time op id (HIP oversized-switch).
// ---------------------------------------------------------------------------
template <class S, int ID>
struct RealBothKernel {
  double const* a;
  double const* b;
  double const* c;
  FailInfo* fail;
  int backend;
  int npoints;

  KOKKOS_INLINE_FUNCTION void operator()(int i) const {
    if (fail->found) return;
    S const sa(a[i]), sb(b[i]), sc(c[i]);
    S const wrap = xpsweep::eval_real_wrap_ct<S, ID>(sa, sb, sc);
    S const core = xpsweep::eval_real_ct<S, ID>(sa, sb, sc);
    constexpr int nb = static_cast<int>(sizeof(S));
    unsigned char wb[sizeof(S)], cb[sizeof(S)];
    store_bytes(&wrap, nb, wb);
    store_bytes(&core, nb, cb);
    if (!bytes_equal(wb, cb, nb)) {
      // First writer wins; races only matter for which failure is reported.
      if (Kokkos::atomic_fetch_add(&fail->found, 1) == 0) {
        fail->backend = backend;
        fail->kind = 0;
        fail->op = ID;
        fail->point = i;
        fail->nbytes = nb;
        store_bytes(wb, nb, fail->wrap_bytes);
        store_bytes(cb, nb, fail->core_bytes);
      }
    }
  }
};

template <class S, int ID>
struct RealWrapOnlyKernel {
  double const* a;
  double const* b;
  double const* c;
  S* out;

  KOKKOS_INLINE_FUNCTION void operator()(int i) const {
    S const sa(a[i]), sb(b[i]), sc(c[i]);
    out[i] = xpsweep::eval_real_wrap_ct<S, ID>(sa, sb, sc);
  }
};

template <class S, class Z, int ID>
struct ComplexBothKernel {
  double const* are;
  double const* aim;
  double const* bre;
  double const* bim;
  FailInfo* fail;
  int backend;
  int npoints;

  KOKKOS_INLINE_FUNCTION void operator()(int i) const {
    if (fail->found) return;
    Z const a{S(are[i]), S(aim[i])};
    Z const b{S(bre[i]), S(bim[i])};
    bool w_is_real = false;
    bool c_is_real = false;
    Z const wrap = xpsweep::eval_complex_wrap_ct<S, Z, ID>(a, b, w_is_real);
    Z const core = xpsweep::eval_complex_ct<S, Z, ID>(a, b, c_is_real);
    constexpr int nb = static_cast<int>(sizeof(Z));
    unsigned char wb[sizeof(Z)], cb[sizeof(Z)];
    store_bytes(&wrap, nb, wb);
    store_bytes(&core, nb, cb);
    if (!bytes_equal(wb, cb, nb)) {
      if (Kokkos::atomic_fetch_add(&fail->found, 1) == 0) {
        fail->backend = backend;
        fail->kind = 1;
        fail->op = ID;
        fail->point = i;
        fail->nbytes = nb;
        store_bytes(wb, nb, fail->wrap_bytes);
        store_bytes(cb, nb, fail->core_bytes);
      }
    }
  }
};

template <class S, class Z, int ID>
struct ComplexWrapOnlyKernel {
  double const* are;
  double const* aim;
  double const* bre;
  double const* bim;
  Z* out;

  KOKKOS_INLINE_FUNCTION void operator()(int i) const {
    Z const a{S(are[i]), S(aim[i])};
    Z const b{S(bre[i]), S(bim[i])};
    bool is_real = false;
    out[i] = xpsweep::eval_complex_wrap_ct<S, Z, ID>(a, b, is_real);
  }
};

// ---------------------------------------------------------------------------
// Host operand fill + dispatch over compile-time op ids.
// ---------------------------------------------------------------------------
template <class S, int ID>
int run_real_op(int backend, xpsweep::Grid const& grid,
                Kokkos::View<FailInfo*, Kokkos::HostSpace> const& h_fail) {
  std::size_t const N = grid.real.size();
  std::vector<double> ha(N), hb(N), hc(N);
  xpsweep::Rng rng(xpsweep::stream_seed(xpsweep::kDefaultSeed,
                                        xpsweep::kReal[ID].name, 0u));
  for (std::size_t i = 0; i < N; ++i) {
    double a = grid.real[i].re;
    double b = 0.0, c = 0.0;
    xpsweep::fill_real_operands(ID, i, a, rng, b, c);
    xpsweep::repair_real(ID, a, b, c);
    ha[i] = a;
    hb[i] = b;
    hc[i] = c;
  }

  Kokkos::View<double*, exec_space> a("a", N), b("b", N), c("c", N);
  {
    auto ah = Kokkos::create_mirror_view(a);
    auto bh = Kokkos::create_mirror_view(b);
    auto ch = Kokkos::create_mirror_view(c);
    for (std::size_t i = 0; i < N; ++i) {
      ah(i) = ha[i];
      bh(i) = hb[i];
      ch(i) = hc[i];
    }
    Kokkos::deep_copy(a, ah);
    Kokkos::deep_copy(b, bh);
    Kokkos::deep_copy(c, ch);
  }

  Kokkos::View<FailInfo*, exec_space> d_fail("fail", 1);
  Kokkos::deep_copy(d_fail, h_fail);

  if (kDeviceBothInKernel) {
    Kokkos::parallel_for(
        "bit_id_real_both", Kokkos::RangePolicy<exec_space>(0, static_cast<int>(N)),
        RealBothKernel<S, ID>{a.data(), b.data(), c.data(), d_fail.data(),
                              backend, static_cast<int>(N)});
    Kokkos::fence();
    Kokkos::deep_copy(h_fail, d_fail);
    return h_fail(0).found ? 1 : 0;
  }

  // Serial pairing: wrapper in parallel_for, core on host.
  Kokkos::View<S*, exec_space> wrap_out("wrap", N);
  Kokkos::parallel_for(
      "bit_id_real_wrap", Kokkos::RangePolicy<exec_space>(0, static_cast<int>(N)),
      RealWrapOnlyKernel<S, ID>{a.data(), b.data(), c.data(), wrap_out.data()});
  Kokkos::fence();
  auto wrap_h = Kokkos::create_mirror_view_and_copy(Kokkos::HostSpace{}, wrap_out);

  for (std::size_t i = 0; i < N; ++i) {
    S const sa(ha[i]), sb(hb[i]), sc(hc[i]);
    S const core = xpsweep::eval_real_ct<S, ID>(sa, sb, sc);
    S const wrap = wrap_h(i);
    constexpr int nb = static_cast<int>(sizeof(S));
    unsigned char wb[sizeof(S)], cb[sizeof(S)];
    std::memcpy(wb, &wrap, sizeof(S));
    std::memcpy(cb, &core, sizeof(S));
    if (std::memcmp(wb, cb, sizeof(S)) != 0) {
      h_fail(0).found = 1;
      h_fail(0).backend = backend;
      h_fail(0).kind = 0;
      h_fail(0).op = ID;
      h_fail(0).point = static_cast<int>(i);
      h_fail(0).nbytes = nb;
      std::memcpy(h_fail(0).wrap_bytes, wb, sizeof(S));
      std::memcpy(h_fail(0).core_bytes, cb, sizeof(S));
      return 1;
    }
  }
  return 0;
}

template <class S, class Z, int ID>
int run_complex_op(int backend, xpsweep::Grid const& grid,
                   Kokkos::View<FailInfo*, Kokkos::HostSpace> const& h_fail) {
  std::size_t const N = grid.complx.size();
  std::vector<double> hare(N), haim(N), hbre(N), hbim(N);
  xpsweep::Rng rng(xpsweep::stream_seed(xpsweep::kDefaultSeed,
                                        xpsweep::kComplex[ID].name, 1u));
  for (std::size_t i = 0; i < N; ++i) {
    double are = grid.complx[i].re;
    double aim = grid.complx[i].im;
    double bre = 0.0, bim = 0.0;
    xpsweep::fill_complex_operands(ID, i, grid.complx, are, aim, rng, bre, bim);
    hare[i] = are;
    haim[i] = aim;
    hbre[i] = bre;
    hbim[i] = bim;
  }

  Kokkos::View<double*, exec_space> are("are", N), aim("aim", N), bre("bre", N),
      bim("bim", N);
  {
    auto are_h = Kokkos::create_mirror_view(are);
    auto aim_h = Kokkos::create_mirror_view(aim);
    auto bre_h = Kokkos::create_mirror_view(bre);
    auto bim_h = Kokkos::create_mirror_view(bim);
    for (std::size_t i = 0; i < N; ++i) {
      are_h(i) = hare[i];
      aim_h(i) = haim[i];
      bre_h(i) = hbre[i];
      bim_h(i) = hbim[i];
    }
    Kokkos::deep_copy(are, are_h);
    Kokkos::deep_copy(aim, aim_h);
    Kokkos::deep_copy(bre, bre_h);
    Kokkos::deep_copy(bim, bim_h);
  }

  Kokkos::View<FailInfo*, exec_space> d_fail("fail", 1);
  Kokkos::deep_copy(d_fail, h_fail);

  if (kDeviceBothInKernel) {
    Kokkos::parallel_for(
        "bit_id_cplx_both", Kokkos::RangePolicy<exec_space>(0, static_cast<int>(N)),
        ComplexBothKernel<S, Z, ID>{are.data(), aim.data(), bre.data(),
                                    bim.data(), d_fail.data(), backend,
                                    static_cast<int>(N)});
    Kokkos::fence();
    Kokkos::deep_copy(h_fail, d_fail);
    return h_fail(0).found ? 1 : 0;
  }

  Kokkos::View<Z*, exec_space> wrap_out("wrap", N);
  Kokkos::parallel_for(
      "bit_id_cplx_wrap", Kokkos::RangePolicy<exec_space>(0, static_cast<int>(N)),
      ComplexWrapOnlyKernel<S, Z, ID>{are.data(), aim.data(), bre.data(),
                                      bim.data(), wrap_out.data()});
  Kokkos::fence();
  auto wrap_h = Kokkos::create_mirror_view_and_copy(Kokkos::HostSpace{}, wrap_out);

  for (std::size_t i = 0; i < N; ++i) {
    Z const a{S(hare[i]), S(haim[i])};
    Z const b{S(hbre[i]), S(hbim[i])};
    bool is_real = false;
    Z const core = xpsweep::eval_complex_ct<S, Z, ID>(a, b, is_real);
    Z const wrap = wrap_h(i);
    constexpr int nb = static_cast<int>(sizeof(Z));
    unsigned char wb[sizeof(Z)], cb[sizeof(Z)];
    std::memcpy(wb, &wrap, sizeof(Z));
    std::memcpy(cb, &core, sizeof(Z));
    if (std::memcmp(wb, cb, sizeof(Z)) != 0) {
      h_fail(0).found = 1;
      h_fail(0).backend = backend;
      h_fail(0).kind = 1;
      h_fail(0).op = ID;
      h_fail(0).point = static_cast<int>(i);
      h_fail(0).nbytes = nb;
      std::memcpy(h_fail(0).wrap_bytes, wb, sizeof(Z));
      std::memcpy(h_fail(0).core_bytes, cb, sizeof(Z));
      return 1;
    }
  }
  return 0;
}

// Unroll op ids with a compile-time integer sequence so each kernel is one op.
template <class S, class Seq>
struct RealOpRunner;
template <class S, int... IDs>
struct RealOpRunner<S, std::integer_sequence<int, IDs...>> {
  static int run(int backend, xpsweep::Grid const& grid,
                 Kokkos::View<FailInfo*, Kokkos::HostSpace> const& h_fail) {
    int rc = 0;
    // Fold: stop at first failure without a runtime switch over 39 cases.
    ((rc != 0 ? 0 : (rc = run_real_op<S, IDs>(backend, grid, h_fail))), ...);
    return rc;
  }
};

template <class S, class Z, class Seq>
struct ComplexOpRunner;
template <class S, class Z, int... IDs>
struct ComplexOpRunner<S, Z, std::integer_sequence<int, IDs...>> {
  static int run(int backend, xpsweep::Grid const& grid,
                 Kokkos::View<FailInfo*, Kokkos::HostSpace> const& h_fail) {
    int rc = 0;
    ((rc != 0 ? 0 : (rc = run_complex_op<S, Z, IDs>(backend, grid, h_fail))),
     ...);
    return rc;
  }
};

template <int N>
using make_ids = std::make_integer_sequence<int, N>;

template <class S, class Z>
int run_backend(int backend, xpsweep::Grid const& grid,
                Kokkos::View<FailInfo*, Kokkos::HostSpace> const& h_fail) {
  if (RealOpRunner<S, make_ids<xpsweep::R_COUNT>>::run(backend, grid, h_fail))
    return 1;
  if (ComplexOpRunner<S, Z, make_ids<xpsweep::C_COUNT>>::run(backend, grid,
                                                             h_fail))
    return 1;
  return 0;
}

void report_fail(FailInfo const& f) {
  std::printf("FAIL bit-identity backend=%s kind=%s op=%s point=%d\n",
              backend_name(f.backend), f.kind == 0 ? "real" : "complex",
              op_name(f.kind, f.op), f.point);
  std::printf("  wrap limbs: ");
  print_hex(f.wrap_bytes, f.nbytes);
  std::printf("\n  core limbs: ");
  print_hex(f.core_bytes, f.nbytes);
  std::printf("\n");
}

}  // namespace

int main(int argc, char* argv[]) {
  Kokkos::initialize(argc, argv);
  int rc = 0;
  {
    std::printf("bit_identity_test: exec_space=%s both_in_kernel=%s\n",
                exec_space::name(), kDeviceBothInKernel ? "yes" : "no");

    xpsweep::Grid grid;
    std::string err;
    if (!xpsweep::read_grid(XPMATH_KOKKOS_SWEEP_GRID, grid, err)) {
      std::fprintf(stderr, "FAIL: %s\n", err.c_str());
      Kokkos::finalize();
      return 2;
    }
    std::printf("grid: %zu real, %zu complex points\n", grid.real.size(),
                grid.complx.size());

    Kokkos::View<FailInfo*, Kokkos::HostSpace> h_fail("h_fail", 1);
    h_fail(0) = FailInfo{};

    // Types are the Experimental aliases (= xp:: types). Core evaluators take
    // xp::; wrap evaluators take the same types via Kokkos:: forwards.
    if (run_backend<xp::DoubleDouble, xp::DoubleDoubleComplex>(0, grid, h_fail) ||
        run_backend<xp::FloatFloat, xp::FloatFloatComplex>(1, grid, h_fail) ||
        run_backend<xp::QuadFloat, xp::QuadFloatComplex>(2, grid, h_fail) ||
        run_backend<xp::TripleFloat, xp::TripleFloatComplex>(3, grid, h_fail)) {
      report_fail(h_fail(0));
      rc = 1;
    } else {
      std::printf(
          "bit_identity_test: OK (4 backends × 63 ops × grid, zero differing "
          "cells)\n");
    }
  }
  Kokkos::finalize();
  return rc;
}
