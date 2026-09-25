# Kokkos::complex interop

**Decision.** `Kokkos::complex<T>` does not instantiate for
`T` in {`xp::DoubleDouble`, `xp::FloatFloat`, `xp::QuadFloat`,
`xp::TripleFloat`}. The supported spelling is the standalone struct:

| real | supported complex |
|---|---|
| `Kokkos::Experimental::DoubleDouble` | `Kokkos::Experimental::DoubleDoubleComplex` |
| `Kokkos::Experimental::FloatFloat` | `Kokkos::Experimental::FloatFloatComplex` |
| `Kokkos::Experimental::QuadFloat` | `Kokkos::Experimental::QuadFloatComplex` |
| `Kokkos::Experimental::TripleFloat` | `Kokkos::Experimental::TripleFloatComplex` |

Each name is an alias of the matching `xp::*Complex` struct. There is no
conversion API to or from `Kokkos::complex<T>`, because that type is
ill-formed for these `T`.

## What was compiled

Translation unit (not a ctest target; it does not compile):

```cpp
#include <Kokkos_Core.hpp>
#include <Kokkos_Complex.hpp>
#include <xp/dd_math.hpp>
#include <xp/ff_math.hpp>
#include <xp/qf_math.hpp>
#include <xp/tf_math.hpp>

template <class T>
void touch() {
  Kokkos::complex<T> z(T(1.0), T(2.0));
  (void)z.real();
  (void)z.imag();
  (void)Kokkos::abs(z);
}

int main() {
  touch<xp::DoubleDouble>();
  touch<xp::FloatFloat>();
  touch<xp::QuadFloat>();
  touch<xp::TripleFloat>();
}
```

A second TU included `Kokkos_xpmath/dd_complex.hpp` and instantiated
`Kokkos::complex<Kokkos::Experimental::DoubleDouble>`. That alias is
`xp::DoubleDouble`, so the diagnostic is the same.

Compiler: GCC 13.3.0, `-std=c++20` (the flag this tree's smoke objects
already use; the project itself stays C++17). Kokkos 5.1.0
(`KOKKOS_VERSION 50100`) from the Serial install, which defines
`KOKKOS_ENABLE_COMPLEX_ALIGN`.

## Diagnostic

`Kokkos_Complex.hpp` rejects the type before any member is usable:

```cpp
static_assert(std::is_floating_point_v<RealType> &&
                  std::is_same_v<RealType, std::remove_cv_t<RealType>>,
              "Kokkos::complex can only be instantiated for a cv-unqualified "
              "floating point type");
```

GCC 13.3.0, all four `xp::` types (abridged to the assertion; the
instantiation stack is `touch<T>` → `Kokkos::complex<T>`):

```
Kokkos_Complex.hpp:35:22: error: static assertion failed: Kokkos::complex can only be instantiated for a cv-unqualified floating point type
   35 |   static_assert(std::is_floating_point_v<RealType> &&
      |                 ~~~~~^~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Kokkos_Complex.hpp:35:22: note: 'std::is_floating_point_v<xp::DoubleDouble>' evaluates to false
```

The same note is emitted for `xp::FloatFloat`, `xp::QuadFloat`, and
`xp::TripleFloat`. `std::is_floating_point` is true only for
`float`, `double`, and `long double`. These backends are class types.
Specializing that trait for them is not permitted.

`TripleFloat` also fails the alignment attribute on the class, because
this install defines `KOKKOS_ENABLE_COMPLEX_ALIGN` (`alignas(2 * sizeof(RealType))`)
and `sizeof(xp::TripleFloat)` is 12:

```
Kokkos_Complex.hpp:34:9: error: requested alignment '24' is not a positive power of 2
   34 |         complex {
```

The other three sizes (DD 16, FF 8, QF 16) are powers of two, so they
hit only the `static_assert`.

## Assumptions not reached

`Kokkos::complex` is written as if `RealType` were a builtin floating
type: `value_type` is defined *by* `Kokkos::complex` as `RealType` (it
does not look up `T::value_type`); constructors form `RealType(0)`,
`RealType(1.0)`, and `RealType(2.0)`; `real()` / `imag()` are members
of `Kokkos::complex`, not of `T`; arithmetic and `operator/=` call
`fabs` on `T`. None of that is instantiated. The `static_assert` makes
the specialization ill-formed first, so those assumptions were not
probed by a successful compile, and they are not worked around here.

## What users should write

Use the standalone complex type. `Kokkos::exp` / `Kokkos::real` /
`Kokkos::imag` already forward for it. Do not write
`Kokkos::complex<Kokkos::Experimental::DoubleDouble>` (or FF / QF / TF).
