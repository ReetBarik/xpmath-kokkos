// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
// SPDX-FileCopyrightText: Copyright (c) 2026 UChicago Argonne, LLC
//
// Throughput runner (GPU_PERF_PLAN P0.3). Times every (type, op) kernel and
// the science-shaped kernel, then writes
//   perf/results/<arch>/throughput_<xpmath-tag>.csv
//   perf/results/<arch>/science_<xpmath-tag>.csv
// Speed only: it never judges accuracy. See perf/README.md.

#include "throughput_common.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#ifndef XPK_XP_VERSION
#define XPK_XP_VERSION "unknown"
#endif
#ifndef XPK_PERF_RESULTS_DIR
#define XPK_PERF_RESULTS_DIR "perf/results"
#endif

#define XPK_DECL(name) void name(const xpk_tp::Config&, std::vector<xpk_tp::Row>&)
XPK_DECL(xpk_tp_unit_r0);
XPK_DECL(xpk_tp_unit_r1);
XPK_DECL(xpk_tp_unit_r2);
XPK_DECL(xpk_tp_unit_r3);
XPK_DECL(xpk_tp_unit_r4);
XPK_DECL(xpk_tp_unit_r5);
XPK_DECL(xpk_tp_unit_c0);
XPK_DECL(xpk_tp_unit_c1);
XPK_DECL(xpk_tp_unit_c2);
XPK_DECL(xpk_tp_unit_c3);
XPK_DECL(xpk_tp_unit_c4);
XPK_DECL(xpk_tp_unit_c5);

namespace {

struct Unit {
  const char* name;
  void (*fn)(const xpk_tp::Config&, std::vector<xpk_tp::Row>&);
};

const Unit kUnits[] = {
    {"double", xpk_tp_unit_r0}, {"float", xpk_tp_unit_r1},   {"DD", xpk_tp_unit_r2},
    {"FF", xpk_tp_unit_r3},     {"TF", xpk_tp_unit_r4},      {"QF", xpk_tp_unit_r5},
    {"cdouble", xpk_tp_unit_c0}, {"cfloat", xpk_tp_unit_c1}, {"cDD", xpk_tp_unit_c2},
    {"cFF", xpk_tp_unit_c3},    {"cTF", xpk_tp_unit_c4},     {"cQF", xpk_tp_unit_c5},
};

void usage() {
  std::fprintf(stderr,
               "usage: xpk_perf_throughput --arch <name> [--n N] [--table N] [--reps N]\n"
               "         [--types t1,t2,...] [--no-science] [--native-mult K] [--out-dir DIR]\n"
               "  --arch      names the results directory (gfx90a, sm_80, ...). Required.\n"
               "  --n         elements per launch (default 2097152)\n"
               "  --table     input table entries, power of two (default 4096)\n"
               "  --native-mult  native double/float run K x more elements, so a\n"
               "              fixed launch cost does not inflate the baseline (default 8)\n"
               "  --types     only these types: double float DD FF TF QF cdouble cfloat cDD cFF cTF cQF\n"
               "  --out-dir   default: %s\n",
               XPK_PERF_RESULTS_DIR);
}

bool wanted(const std::vector<std::string>& only, const char* name) {
  if (only.empty()) return true;
  for (const auto& s : only)
    if (s == name) return true;
  return false;
}

bool is_native(const char* name) {
  return !std::strcmp(name, "double") || !std::strcmp(name, "float") ||
         !std::strcmp(name, "cdouble") || !std::strcmp(name, "cfloat");
}

// Ratio of time per operation per element, so rows with different n compare.
std::string ratio(const xpk_tp::Row& row, const xpk_tp::Row* base) {
  if (base == nullptr) return "";
  char buf[32];
  std::snprintf(buf, sizeof buf, "%.3f", row.ns_per_op() / base->ns_per_op());
  return buf;
}

}  // namespace

int main(int argc, char** argv) {
  Kokkos::initialize(argc, argv);
  int rc = 0;
  {
    xpk_tp::Config cfg;
    std::string arch, out_dir = XPK_PERF_RESULTS_DIR;
    std::vector<std::string> only;
    int native_mult = 8;
    for (int i = 1; i < argc; ++i) {
      const std::string a = argv[i];
      auto next = [&]() -> const char* {
        if (i + 1 >= argc) {
          usage();
          std::exit(2);
        }
        return argv[++i];
      };
      if (a == "--arch") arch = next();
      else if (a == "--n") cfg.n = std::atoi(next());
      else if (a == "--table") cfg.table = std::atoi(next());
      else if (a == "--native-mult") native_mult = std::atoi(next());
      else if (a == "--reps") cfg.reps = std::atoi(next());
      else if (a == "--out-dir") out_dir = next();
      else if (a == "--no-science") cfg.science = false;
      else if (a == "--types") {
        std::stringstream ss(next());
        for (std::string t; std::getline(ss, t, ',');) only.push_back(t);
      } else {
        usage();
        std::exit(2);
      }
    }
    if (arch.empty() || native_mult <= 0 || cfg.n <= 0 || cfg.reps <= 0 || cfg.table <= 0 ||
        (cfg.table & (cfg.table - 1)) != 0) {
      usage();
      std::exit(2);
    }

    std::printf("xpk_perf_throughput: xpmath %s, exec space %s, n=%d (native x%d) table=%d warmup=%d reps=%d\n",
                XPK_XP_VERSION, Kokkos::DefaultExecutionSpace().name(), cfg.n, native_mult,
                cfg.table, cfg.warmup, cfg.reps);
    std::vector<xpk_tp::Row> rows;
    for (const Unit& u : kUnits) {
      if (!wanted(only, u.name)) continue;
      const std::size_t first = rows.size();
      xpk_tp::Config c = cfg;
      if (is_native(u.name)) c.n *= native_mult;
      u.fn(c, rows);
      for (std::size_t r = first; r < rows.size(); ++r) {
        const auto& row = rows[r];
        std::printf("  %-8s %-8s %9.3f ms  %10.4f ns/op%s\n", row.type.c_str(), row.op.c_str(),
                    row.ms_median, row.ns_per_op(),
                    row.nonfinite ? "  NON-FINITE OUTPUT" : "");
        std::fflush(stdout);
        if (row.nonfinite) rc = 3;
      }
    }

    std::map<std::string, const xpk_tp::Row*> by_key;
    for (const auto& row : rows) by_key[row.type + "/" + row.op] = &row;
    auto base = [&](const xpk_tp::Row& row, const char* real_t) -> const xpk_tp::Row* {
      const bool complex_row = row.kind != "science" && row.type[0] == 'c';
      const std::string t = complex_row ? std::string("c") + real_t : std::string(real_t);
      auto it = by_key.find(t + "/" + row.op);
      return it == by_key.end() ? nullptr : it->second;
    };

    std::string tag = XPK_XP_VERSION;
    for (char& c : tag)
      if (c == '/' || c == ' ') c = '_';
    const std::filesystem::path dir = std::filesystem::path(out_dir) / arch;
    std::filesystem::create_directories(dir);
    const auto tp_path = dir / ("throughput_" + tag + ".csv");
    const auto sci_path = dir / ("science_" + tag + ".csv");

    std::ofstream tp(tp_path), sci;
    tp << "type,op,kind,ops_per_elem,n,ms_median,ms_min,ms_max,ns_per_op,ratio_double,ratio_float,"
          "nonfinite\n";
    bool any_sci = false;
    for (const auto& row : rows) {
      if (row.kind == "science") {
        if (!any_sci) {
          sci.open(sci_path);
          sci << "type,n,ops_per_elem,ms_median,ms_min,ms_max,ns_per_element,ratio_double,"
                 "ratio_float,nonfinite\n";
          any_sci = true;
        }
        char buf[256];
        std::snprintf(buf, sizeof buf, "%s,%d,%d,%.4f,%.4f,%.4f,%.4f,", row.type.c_str(), row.n,
                      row.ops_per_elem, row.ms_median, row.ms_min, row.ms_max,
                      row.ms_median * 1e6 / row.n);
        sci << buf << ratio(row, base(row, "double")) << ","
            << ratio(row, base(row, "float")) << "," << row.nonfinite << "\n";
        continue;
      }
      char buf[256];
      std::snprintf(buf, sizeof buf, "%s,%s,%s,%d,%d,%.4f,%.4f,%.4f,%.5f,", row.type.c_str(),
                    row.op.c_str(), row.kind.c_str(), row.ops_per_elem, row.n, row.ms_median,
                    row.ms_min, row.ms_max, row.ns_per_op());
      tp << buf << ratio(row, base(row, "double")) << ","
         << ratio(row, base(row, "float")) << "," << row.nonfinite << "\n";
    }
    std::printf("wrote %s\n", tp_path.string().c_str());
    if (any_sci) std::printf("wrote %s\n", sci_path.string().c_str());
    if (rc == 3) std::fprintf(stderr, "WARNING: non-finite output in some kernels; timings suspect\n");
  }
  Kokkos::finalize();
  return rc;
}
