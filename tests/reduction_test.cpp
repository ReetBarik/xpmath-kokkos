// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// K2: Kokkos::reduction_identity bit-identity gate for the eight wrappers.
//
// Serial execution space: bit-identity is required between
//   (a) Kokkos::parallel_reduce over Kokkos::Experimental::T, and
//   (b) the same left-fold performed serially through the core xp::T.
// N = 0 and N = 1 especially exercise the identity itself; N = 0 must pass
// for every type × {sum, prod}.
//
// Device spaces (CUDA/HIP/SYCL), if ever exercised here: do NOT require
// bit-identity against the serial fold. Non-associative multi-word arithmetic
// plus nondeterministic reduction trees only permit a finiteness /
// re-association envelope (result finite and within the range spanned by the
// serial answer and a re-association of it). This file currently runs Serial
// only; that distinction is stated so a future reader does not "fix" a
// device assertion into a false bit-identity gate.

#include <Kokkos_xpmath/Kokkos_xpmath.hpp>

#include <cstdio>
#include <cstdlib>

namespace {

int g_failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);     \
      ++g_failures;                                                            \
    }                                                                          \
  } while (0)

enum class ReduceOp { Sum, Prod };

template <class T>
T sum_elem(int i) {
  return T(static_cast<double>(i + 1));
}

template <class T>
T prod_elem(int i) {
  return (i == 0) ? T(2.0) : T(1.0);
}

template <class WrapT, class CoreT, class ElemFn>
void check_reduce(char const* label, int N, ElemFn elem, ReduceOp op) {
  using exec_space = Kokkos::Serial;

  Kokkos::View<WrapT*, exec_space> v("v", static_cast<size_t>(N));
  auto h = Kokkos::create_mirror_view(v);
  for (int i = 0; i < N; ++i) h(i) = elem(i);
  Kokkos::deep_copy(v, h);

  WrapT parallel_result;
  CoreT serial{};

  if (op == ReduceOp::Sum) {
    parallel_result = Kokkos::reduction_identity<WrapT>::sum();
    Kokkos::parallel_reduce(
        "sum", Kokkos::RangePolicy<exec_space>(0, N),
        KOKKOS_LAMBDA(int i, WrapT & update) { update += v(i); },
        Kokkos::Sum<WrapT>(parallel_result));

    serial = CoreT(0.0);
    for (int i = 0; i < N; ++i) {
      serial += static_cast<CoreT>(elem(i));
    }
  } else {
    parallel_result = Kokkos::reduction_identity<WrapT>::prod();
    Kokkos::parallel_reduce(
        "prod", Kokkos::RangePolicy<exec_space>(0, N),
        KOKKOS_LAMBDA(int i, WrapT & update) { update *= v(i); },
        Kokkos::Prod<WrapT>(parallel_result));

    serial = CoreT(1.0);
    for (int i = 0; i < N; ++i) {
      serial *= static_cast<CoreT>(elem(i));
    }
  }

  CoreT got = static_cast<CoreT>(parallel_result);
  if (!(got == serial)) {
    std::fprintf(stderr, "FAIL bit-identity %s N=%d\n", label, N);
    ++g_failures;
  }
}

template <class WrapT, class CoreT>
void run_type(char const* name) {
  static int const Ns[] = {0, 1, 2, 10000};
  for (int N : Ns) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s/sum", name);
    check_reduce<WrapT, CoreT>(buf, N, sum_elem<WrapT>, ReduceOp::Sum);
    std::snprintf(buf, sizeof(buf), "%s/prod", name);
    check_reduce<WrapT, CoreT>(buf, N, prod_elem<WrapT>, ReduceOp::Prod);
  }
  CHECK(Kokkos::reduction_identity<WrapT>::sum() == WrapT(0.0));
  CHECK(Kokkos::reduction_identity<WrapT>::prod() == WrapT(1.0));
}

}  // namespace

int main(int argc, char* argv[]) {
  Kokkos::initialize(argc, argv);
  {
    run_type<Kokkos::Experimental::DoubleDouble, xp::DoubleDouble>("dd");
    run_type<Kokkos::Experimental::FloatFloat, xp::FloatFloat>("ff");
    run_type<Kokkos::Experimental::QuadFloat, xp::QuadFloat>("qf");
    run_type<Kokkos::Experimental::TripleFloat, xp::TripleFloat>("tf");

    run_type<Kokkos::Experimental::DoubleDoubleComplex, xp::DoubleDoubleComplex>(
        "ddc");
    run_type<Kokkos::Experimental::FloatFloatComplex, xp::FloatFloatComplex>(
        "ffc");
    run_type<Kokkos::Experimental::QuadFloatComplex, xp::QuadFloatComplex>(
        "qfc");
    run_type<Kokkos::Experimental::TripleFloatComplex, xp::TripleFloatComplex>(
        "tfc");
  }
  Kokkos::finalize();

  if (g_failures != 0) {
    std::fprintf(stderr, "%d failure(s)\n", g_failures);
    return 1;
  }
  std::printf("reduction_test: OK (8 types × {sum,prod} × N={0,1,2,10000})\n");
  return 0;
}
