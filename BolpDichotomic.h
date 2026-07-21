//
//  BolpDichotomic.h
//  BOMILP_2024
//
//  Replaces the BENSOLVE-based BOLP solve (build_vlp_new() + bensolve() +
//  define_list_extreme_solutions[_k]()) with an in-process solve using
//  Gurobi as the weighted-sum oracle for dichotomic_search.hpp.
//
//  Why this is a valid drop-in replacement:
//    Each "slice" BOLP solved here is a genuine bi-objective LINEAR program
//    (integer variables already fixed, folded into z1int/z2int and bnew).
//    The image of a polyhedron under a linear map is itself a polyhedron,
//    so its non-dominated frontier is exactly the piecewise-linear boundary
//    connecting the SUPPORTED efficient extreme points. Dichotomic search
//    finds exactly those extreme points -- the same vertices BENSOLVE's
//    Benson-type algorithm would return for this problem class -- so no
//    accuracy is lost by switching from BENSOLVE to dichotomic search here.
//    (This equivalence would NOT hold if this were used on a MILP/IP
//    instead of a continuous LP slice.)
//
//  Data layout (unchanged, reused directly from Header.h's store_data /
//  store_data_int_fixing):
//    B1[i][j]  (i=1..rows, j=1..cols) : continuous-variable constraint coeffs
//    B2[i][j]  (i=1..rows, j=1..bv)   : integer-variable constraint coeffs
//    P[1][j], P[3][j] (j=1..cols)     : continuous-variable objective coeffs
//                                        for f1, f2 respectively
//    P[2][j], P[4][j] (j=1..bv)       : integer-variable objective coeffs
//                                        for f1, f2 respectively (already
//                                        folded into z1int/z2int upstream)
//    b[i]      (i=1..rows)            : original RHS
//    c[i]      (i=1..rows)            : row type, 0 = equality, 1 = "<="
//    M                                : current bound on f2 (this iteration's
//                                        MILPk optimal f2 value)
//
//  DEPENDENCIES: dichotomic_search.hpp, gurobi_c++.h
//

#pragma once

#include "dichotomic_search.hpp"
#include "gurobi_c++.h"

#include <algorithm>
#include <cmath>
#include <list>
#include <stdexcept>
#include <string>
#include <vector>

// ============================================================
// Persistent Gurobi state for the BOLP slice problem.
//
// B1, P and c never change during a run (only the current integer fixing
// bvar, and hence bnew and M - z2int, change from one BOLP solve to the
// next). So the model structure (variables, constraint LHS coefficients,
// objective expressions) is built exactly ONCE, in initBolpModel(); every
// subsequent call to solveBOLPDichotomic() just overwrites the RHS values
// in place before re-optimizing -- mirroring the persistent-model pattern
// already used for MILPk in GurobiMPS.h.
// ============================================================
struct BolpState {
    GRBModel model;
    std::vector<GRBVar> x;              // 1-indexed, size cols+1
    std::vector<GRBConstr> rowConstr;   // 1-indexed, size rows+2
                                         // (1..rows original rows, rows+1 = f2 bound)
    GRBLinExpr f1expr;                  // continuous part of f1
    GRBLinExpr f2expr;                  // continuous part of f2
    bool ready = false;

    // Running total of oracle calls (i.e. Gurobi optimize() calls) made by
    // dichotomic search across ALL solveBOLPDichotomic() invocations in the
    // run. Each call is a continuous LP solve, not a MILP -- see the note
    // at the top of this file -- but this is exposed for reporting under
    // whatever label the caller wants (e.g. "#MILPs-Dichotomic").
    long numDichotomicSolves = 0;

    // Reuses the shared GRBEnv (e.g. itState.env) rather than creating a
    // new environment, consistent with the rest of the codebase.
    explicit BolpState(GRBEnv& env) : model(env) {
        model.set(GRB_IntParam_OutputFlag, 0);
    }
};

// ============================================================
// Builds the BOLP model once: cols continuous variables x_j >= 0, the
// `rows` original constraints (RHS set to a placeholder 0.0, overwritten on
// every solve) plus the extra f2-upper-bound row, and the two continuous
// objective expressions f1expr/f2expr. Called automatically, at most once,
// from solveBOLPDichotomic().
// ============================================================
inline void initBolpModel(BolpState& st, int rows, int cols,
                           double** B1, double** P, double* c)
{
    GRBModel& model = st.model;

    st.x.resize(cols + 1);
    for (int j = 1; j <= cols; j++)
        st.x[j] = model.addVar(0.0, GRB_INFINITY, 0.0, GRB_CONTINUOUS,
                                "y" + std::to_string(j));
    model.update();

    st.rowConstr.resize(rows + 2);
    for (int i = 1; i <= rows; i++) {
        GRBLinExpr lhs = 0;
        for (int j = 1; j <= cols; j++)
            if (B1[i][j] != 0.0) lhs += B1[i][j] * st.x[j];

        // Row type as produced by store_data(): 0 = equality, 1 = "<=".
        // (Matches build_vlp_new()'s "s"/"u" bound-type distinction.)
        if (c[i] == 0.0)
            st.rowConstr[i] = model.addConstr(lhs == 0.0, "b" + std::to_string(i));
        else
            st.rowConstr[i] = model.addConstr(lhs <= 0.0, "b" + std::to_string(i));
    }

    // Extra row (rows+1): continuous part of f2, bounded above by (M - z2int).
    {
        GRBLinExpr f2boundLhs = 0;
        for (int j = 1; j <= cols; j++)
            if (P[3][j] != 0.0) f2boundLhs += P[3][j] * st.x[j];
        st.rowConstr[rows + 1] = model.addConstr(f2boundLhs <= 0.0, "f2bound");
    }

    // Objective expressions (continuous parts only). The fixed/integer part
    // (z1int, z2int) is added back as a constant offset by the caller.
    st.f1expr = 0;
    st.f2expr = 0;
    for (int j = 1; j <= cols; j++) {
        if (P[1][j] != 0.0) st.f1expr += P[1][j] * st.x[j];
        if (P[3][j] != 0.0) st.f2expr += P[3][j] * st.x[j];
    }

    model.update();
    st.ready = true;
}

// ============================================================
// Solves the current BOLP slice problem via dichotomic search, using
// Gurobi as the weighted-sum oracle, and returns the supported
// non-dominated extreme points -- already shifted by (z1int, z2int) to
// match what define_list_extreme_solutions[_k]() used to return -- sorted
// by increasing f1, with duplicates removed.
//
// This is the direct replacement for:
//     build_vlp_new(rows, cols, bv, NB, NP, NP2, z1Fix, z2Fix, M,
//                    bvar, c, B1, B2, P, b, pathVlp);
//     bensolve(pathVlp);
//     define_list_extreme_solutions[_k](..., pathBensSol, intFixings);
//
// Parameters mirror build_vlp_new()'s (minus the output file path and the
// counts NB/NP/NP2, which were only needed to size the .vlp header).
//
// oracleBudgetFraction caps the number of LP solves dichotomic search is
// allowed to spend on THIS BOLP, as a fraction of `cols` (the number of
// continuous variables in the slice). 1.0 (the default) means unlimited --
// the exact supported frontier, identical to the original behavior.
// Any value in (0, 1) trades exactness for speed: dichotomic search
// returns an approximate (but still genuinely non-dominated) partial
// frontier once the budget -- max(2, ceil(oracleBudgetFraction * cols))
// oracle calls -- is exhausted, prioritizing the largest gaps in the
// frontier first so the approximation stays evenly spread.
//
// IMPORTANT: if this BOLP's extremePointsCurr is later used for the
// cut-based discard test (interior + extreme-direction cuts), an
// approximate frontier weakens that test's soundness guarantee: a cut
// derived from an incomplete polyline can claim domination in a region
// where the true (fully resolved) frontier would actually have had a
// better point that just wasn't found due to the budget cutoff. Use a
// fraction < 1.0 only where that tradeoff is acceptable.
// ============================================================
inline std::list<std::vector<double>> solveBOLPDichotomic(
    int rows, int cols, int bv,
    double** B1, double** B2, double** P,
    double* b, double* c, int* bvar,
    double M, double z1int, double z2int,
    BolpState& state,
    double oracleBudgetFraction = 1.0)
{
    if (!state.ready)
        initBolpModel(state, rows, cols, B1, P, c);

    // Recompute the RHS for the current integer fixing:
    //   bnew[i] = b[i] - sum_j B2[i][j] * bvar[j]      for i = 1..rows
    //   f2-bound row (rows+1)                          = M - z2int
    std::vector<GRBConstr> constrsToUpdate;
    std::vector<double> rhsToUpdate;
    constrsToUpdate.reserve(rows + 1);
    rhsToUpdate.reserve(rows + 1);

    for (int i = 1; i <= rows; i++) {
        double bapp = 0.0;
        for (int j = 1; j <= bv; j++) bapp += B2[i][j] * bvar[j];
        constrsToUpdate.push_back(state.rowConstr[i]);
        rhsToUpdate.push_back(b[i] - bapp);
    }
    constrsToUpdate.push_back(state.rowConstr[rows + 1]);
    rhsToUpdate.push_back(M - z2int);

    // NOTE: Gurobi's C++ API only provides the pointer+length overload of
    // set() for batch RHS updates (no std::vector convenience overload).
    state.model.set(GRB_DoubleAttr_RHS, constrsToUpdate.data(),
                     rhsToUpdate.data(), (int)constrsToUpdate.size());

    // Weighted-sum oracle: solves min w1*f1_cont(x) + w2*f2_cont(x)
    // subject to the constraints just updated above.
    Oracle oracle = [&state](const Weight& w) -> Point {
        GRBLinExpr obj = w.w1 * state.f1expr + w.w2 * state.f2expr;
        state.model.setObjective(obj, GRB_MINIMIZE);
        state.model.optimize();
        state.numDichotomicSolves++;

        int status = state.model.get(GRB_IntAttr_Status);
        if (status != GRB_OPTIMAL)
            throw std::runtime_error(
                "solveBOLPDichotomic: Gurobi did not reach optimality (status " +
                std::to_string(status) + ")");

        return Point{state.f1expr.getValue(), state.f2expr.getValue()};
    };

    int maxOracleCalls = -1; // unlimited by default
    if (oracleBudgetFraction > 0.0 && oracleBudgetFraction < 1.0) {
        maxOracleCalls = std::max(2, (int)std::ceil(oracleBudgetFraction * cols));
    }

    DichotomicSearch search(oracle, 1e-7, maxOracleCalls);
    std::vector<Point> pts = search.run();

    std::list<std::vector<double>> result;
    for (const auto& p : pts)
        result.push_back(std::vector<double>{p.f1 + z1int, p.f2 + z2int});
    return result;
}

// ============================================================
// Merges freshly found extreme points into the running "all extreme
// points" list, keeping it sorted and free of duplicates. Equivalent to
// what define_list_extreme_solutions() used to do by re-parsing and
// re-sorting the whole accumulated file each time.
// ============================================================
inline void mergeExtremePoints(std::list<std::vector<double>>& all,
                                const std::list<std::vector<double>>& fresh)
{
    all.insert(all.end(), fresh.begin(), fresh.end());
    all.sort();
    all.unique();
}
