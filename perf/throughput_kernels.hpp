// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// Throughput kernels (GPU_PERF_PLAN P0.3), built on the types, traits, ops and
// input domains of kernels.hpp. Speed only: no oracle, no ulps, no tolerances.
//
// Two kernel shapes, both storing one double per element (the sum of every
// limb of the result, so no limb is dead code):
//   chain  add mul div: a dependent chain of 64 operations on operands held in
//          registers, with values that stay bounded (multiplier/divisor near 1).
//   func   sqrt exp log sin atan2 abs: 4 evaluations per element on 4 inputs.
// Inputs come from a small table (4096 entries) indexed by `i & mask`, so the
// native double/float baselines are not limited by memory bandwidth.

#pragma once

#include "science_kernel.hpp"
#include "throughput_common.hpp"

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace xpk_perf {

constexpr int kChainLen = 64;
constexpr int kChainUnroll = 16;  // explicit, so loop overhead is the same for every type
constexpr int kFuncEvals = 4;

template <class Op>
inline constexpr bool is_chain_v =
    std::is_same_v<Op, op::Add> || std::is_same_v<Op, op::Mul> || std::is_same_v<Op, op::Div>;

inline const char* op_name(op::Add) { return "add"; }
inline const char* op_name(op::Mul) { return "mul"; }
inline const char* op_name(op::Div) { return "div"; }
inline const char* op_name(op::Sqrt) { return "sqrt"; }
inline const char* op_name(op::Exp) { return "exp"; }
inline const char* op_name(op::Log) { return "log"; }
inline const char* op_name(op::Sin) { return "sin"; }
inline const char* op_name(op::Atan2) { return "atan2"; }
inline const char* op_name(op::Abs) { return "abs"; }

template <class T>
KOKKOS_FORCEINLINE_FUNCTION T make_val(double re, double im, double t) {
  if constexpr (IsComplex<T>::value) return ComplexTraits<T>::make(re, im, t);
  else return RealTraits<T>::make(re, t);
}

template <class T>
constexpr double exp_max_of() {
  if constexpr (IsComplex<T>::value) return ComplexTraits<T>::exp_max;
  else return RealTraits<T>::exp_max;
}

// Column k uniform in [lo[k], hi[k]); the `t` column is uniform in [-1, 1).
inline Inputs make_uniform_inputs(int n, std::uint64_t seed, const double (&lo)[4],
                                  const double (&hi)[4]) {
  Inputs in;
  in.n = n;
  Kokkos::View<double*>* col[5] = {&in.a, &in.b, &in.c, &in.d, &in.t};
  const char* names[5] = {"tp_a", "tp_b", "tp_c", "tp_d", "tp_t"};
  std::uint64_t s = seed;
  for (int k = 0; k < 5; ++k) {
    *col[k] = Kokkos::View<double*>(Kokkos::view_alloc(std::string(names[k]), Kokkos::WithoutInitializing), n);
    auto h = Kokkos::create_mirror_view(*col[k]);
    const double l = k < 4 ? lo[k] : -1.0;
    const double u = k < 4 ? hi[k] : 1.0;
    for (int i = 0; i < n; ++i) h(i) = detail::uniform(s, l, u);
    Kokkos::deep_copy(*col[k], h);
  }
  return in;
}

template <class T, class Op>
struct ChainKernel {
  Inputs in;
  Kokkos::View<double*> out;

  template <std::size_t... K>
  static KOKKOS_FORCEINLINE_FUNCTION void step(T& x, const T& y, std::index_sequence<K...>) {
    ((x = apply(Op{}, x, y), static_cast<void>(K)), ...);
  }

  KOKKOS_INLINE_FUNCTION void operator()(const int i) const {
    const int j = i & (in.n - 1);
    T x = make_val<T>(in.a(j), in.b(j), in.t(j));
    const T y = make_val<T>(in.c(j), in.d(j), in.t(j));
#pragma unroll 1
    for (int r = 0; r < kChainLen / kChainUnroll; ++r)
      step(x, y, std::make_index_sequence<kChainUnroll>{});
    out(i) = sink(x);
  }
};

template <class T, class Op>
struct FuncKernel {
  Inputs p, q;  // q is used by complex types only (4 complex inputs need 8 doubles)
  Kokkos::View<double*> out;

  KOKKOS_INLINE_FUNCTION void operator()(const int i) const {
    const int j = i & (p.n - 1);
    T v[kFuncEvals];
    if constexpr (IsComplex<T>::value) {
      v[0] = make_val<T>(p.a(j), p.b(j), p.t(j));
      v[1] = make_val<T>(p.c(j), p.d(j), p.t(j));
      v[2] = make_val<T>(q.a(j), q.b(j), q.t(j));
      v[3] = make_val<T>(q.c(j), q.d(j), q.t(j));
    } else {
      v[0] = make_val<T>(p.a(j), 0.0, p.t(j));
      v[1] = make_val<T>(p.b(j), 0.0, p.t(j));
      v[2] = make_val<T>(p.c(j), 0.0, p.t(j));
      v[3] = make_val<T>(p.d(j), 0.0, p.t(j));
    }
    // Unary ops ignore the second operand; atan2 sees (v0,v1) (v1,v2) (v2,v3) (v3,v0).
    double acc = sink(apply(Op{}, v[0], v[1]));
    acc += sink(apply(Op{}, v[1], v[2]));
    acc += sink(apply(Op{}, v[2], v[3]));
    acc += sink(apply(Op{}, v[3], v[0]));
    out(i) = acc;
  }
};

template <class T, class Op>
void run_op(const xpk_tp::Config& cfg, const char* tname, std::vector<xpk_tp::Row>& rows) {
  xpk_tp::Row row;
  row.type = tname;
  row.op = op_name(Op{});
  row.n = cfg.n;
  Kokkos::View<double*> out(Kokkos::view_alloc(Kokkos::WithoutInitializing, "tp_out"), cfg.n);
  if constexpr (is_chain_v<Op>) {
    row.kind = "chain";
    row.ops_per_elem = kChainLen;
    const double lo[4] = {0.5, 0.5, 0.999, -0.001};
    const double hi[4] = {1.5, 1.5, 1.001, 0.001};
    ChainKernel<T, Op> k{make_uniform_inputs(cfg.table, 0xC0FFEEull, lo, hi), out};
    xpk_tp::time_launches(
        cfg, [&] { Kokkos::parallel_for("xpk_tp_chain", Policy(0, cfg.n), k); }, row);
  } else {
    row.kind = "func";
    row.ops_per_elem = kFuncEvals;
    constexpr double em = exp_max_of<T>();
    constexpr Dom dom = IsComplex<T>::value ? complex_domain<Op>() : real_domain<Op>();
    FuncKernel<T, Op> k{make_inputs(cfg.table, dom, em), make_inputs(cfg.table, dom, em, 0x14057B7EF767814Full),
                        out};
    xpk_tp::time_launches(
        cfg, [&] { Kokkos::parallel_for("xpk_tp_func", Policy(0, cfg.n), k); }, row);
  }
  row.nonfinite = xpk_tp::count_nonfinite(out);
  rows.push_back(row);
}

template <class T, class... Ops>
void run_ops(const xpk_tp::Config& cfg, const char* tname, std::vector<xpk_tp::Row>& rows,
             std::tuple<Ops...>) {
  (run_op<T, Ops>(cfg, tname, rows), ...);
}

// Science kernel for the (R, Z) pair; the row is keyed by the real type's name.
template <class R, class Z>
void run_science(const xpk_tp::Config& cfg, const char* rname, std::vector<xpk_tp::Row>& rows) {
  xpk_tp::Row row;
  row.type = rname;
  row.op = "science";
  row.kind = "science";
  row.ops_per_elem = ScienceKernel<R, Z>::ops_per_element;
  row.n = cfg.n;
  Kokkos::View<double*> out(Kokkos::view_alloc(Kokkos::WithoutInitializing, "tp_out"), cfg.n);
  const double lo[4] = {-0.7, -0.7, -0.7, -0.7};
  const double hi[4] = {0.7, 0.7, 0.7, 0.7};
  ScienceKernel<R, Z> k{make_uniform_inputs(cfg.table, 0x5C1E4CEull, lo, hi), out};
  xpk_tp::time_launches(
      cfg, [&] { Kokkos::parallel_for("xpk_tp_science", Policy(0, cfg.n), k); }, row);
  row.nonfinite = xpk_tp::count_nonfinite(out);
  rows.push_back(row);
}

}  // namespace xpk_perf
