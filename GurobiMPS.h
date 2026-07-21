//
//  GurobiMPS.h
//  BOMILP_2024
//
//  Authors: Lavinia Amorosi, Marianna De Santis
//
//  DEPENDENCIES:
//    - Gurobi C++ API: #include "gurobi_c++.h"
//    - Compile with: -lgurobi_c++ -lgurobi<version> and the correct include/lib paths
//
// ── Summary of optimisations vs. the original version ────────────────────────
//
// [A] Persistent GRBModel for MILPk  (was: rebuilt from scratch every iteration)
//     The base model (original constraints + f2 bound + min-f1 objective) is
//     constructed once in initMilpModel() and stored in IterState.  Each call to
//     updateMPS_and_solve() then only:
//       - updates the RHS of the f2 constraint in-place  (O(1))
//       - appends the single new no-good for the current fixing  (O(nint))
//     Total model-build cost: O(m*nvar) once, instead of O(K*m*nvar).
//
// [B] Direct variable access  (was: getVarByName() loop every iteration)
//     GRBVar references are stored in IterState.vars at construction time and
//     passed directly to solveModel(), eliminating all per-iteration name lookups.
//
// [C] Incremental no-good constraints  (was: all k no-goods rewritten at iter k)
//     Because the model persists, only the no-good for the just-processed fixing
//     needs to be added.  Total addConstr calls: O(K) instead of O(K^2/2).
//
// [D] Shared GRBEnv  (was: new GRBEnv created in computeIdealPoint2 every call)
//     IterState owns the single GRBEnv used for every Gurobi model in the session.
//     computeIdealPoint2 now receives IterState& and reuses state.env.
//     Signature change in main.cpp: pass itState as the fourth argument.
//
// [E] Fixed-variable LP without post-hoc relaxation  (was: addVar as BINARY then
//     loop to set VType=CONTINUOUS)
//     In computeIdealPoint2, integer variables are declared as GRB_CONTINUOUS with
//     lb = ub = fixed value from the start, so no relaxation loop is needed.
//
// ─────────────────────────────────────────────────────────────────────────────

#ifndef GurobiMPS_h
#define GurobiMPS_h

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cassert>
#include <cmath>
#include <iostream>
#include <stdexcept>

#include "gurobi_c++.h"

// ============================================================
// Data structures shared across all functions
// ============================================================

struct InstanceData {
    int m;           // number of constraints
    int ncont;       // number of continuous variables
    int nint;        // number of binary/integer variables
    int nvar;        // ncont + nint
    std::vector<double> f1;   // first objective function coefficients
    std::vector<double> f2;   // second objective function coefficients
    std::vector<std::vector<double>> A; // constraint matrix (m x nvar)
    std::vector<double> b;    // right-hand side vector
    std::vector<int>    ty;   // constraint types: 1=<=, 0==
    // Variable bounds for continuous variables (binary vars are always [0,1])
    // BUG A fix: bounds must be read from .dat to avoid using wrong [0,+inf] defaults
    std::vector<double> lb;   // lower bounds (size ncont; binary vars always 0)
    std::vector<double> ub;   // upper bounds (size ncont; binary vars always 1)
};

// ============================================================
// Reads the BOMILP instance from a .dat file.
// Expected format (same structure as writeMPS.m):
//   Line 1           : m       (number of constraints)
//   Line 2           : ncont   (number of continuous variables)
//   Line 3           : nint    (number of binary/integer variables)
//   Line 4           : f1 coefficients for continuous variables (space-separated)
//   Line 5           : f1 coefficients for integer variables
//   Line 6           : f2 coefficients for continuous variables
//   Line 7           : f2 coefficients for integer variables
//   Lines 8..8+m-1   : rows of Acont (constraint matrix, continuous part)
//   Lines 8+m..8+2m-1: rows of Aint  (constraint matrix, integer part)
//   Line 8+2m        : b  (right-hand side, m values space-separated)
//   Line 8+2m+1      : ty (constraint types: 1=<=, 0==, m values)
//   Line 8+2m+2      : lower bounds for continuous variables (ncont values) [BUG A fix]
//   Line 8+2m+3      : upper bounds for continuous variables (ncont values) [BUG A fix]
//
// NOTE: if your .dat files do not have the last two bound lines, continuous
// variables will default to lb=0, ub=1e30. Add the bound lines to .dat to
// correctly represent instances with non-zero lower bounds (e.g. lb=-100).
// ============================================================
InstanceData readInstance(const std::string& datFile)
{
    InstanceData d;
    std::ifstream f(datFile);
    if (!f.is_open())
        throw std::runtime_error("Cannot open instance file: " + datFile);

    std::string line;

    auto readInt = [&]() -> int {
        std::getline(f, line);
        return std::stoi(line);
    };
    auto readDoubleVec = [&](int n) -> std::vector<double> {
        std::getline(f, line);
        std::istringstream ss(line);
        std::vector<double> v(n);
        for (int i = 0; i < n; i++) ss >> v[i];
        return v;
    };

    d.m     = readInt();
    d.ncont = readInt();
    d.nint  = readInt();
    d.nvar  = d.ncont + d.nint;

    std::vector<double> f1cont = readDoubleVec(d.ncont);
    std::vector<double> f1int  = readDoubleVec(d.nint);
    std::vector<double> f2cont = readDoubleVec(d.ncont);
    std::vector<double> f2int  = readDoubleVec(d.nint);

    d.f1.insert(d.f1.end(), f1cont.begin(), f1cont.end());
    d.f1.insert(d.f1.end(), f1int.begin(),  f1int.end());
    d.f2.insert(d.f2.end(), f2cont.begin(), f2cont.end());
    d.f2.insert(d.f2.end(), f2int.begin(),  f2int.end());

    // Constraint matrix
    d.A.resize(d.m, std::vector<double>(d.nvar, 0.0));
    for (int i = 0; i < d.m; i++) {
        std::vector<double> row = readDoubleVec(d.ncont);
        for (int j = 0; j < d.ncont; j++) d.A[i][j] = row[j];
    }
    for (int i = 0; i < d.m; i++) {
        std::vector<double> row = readDoubleVec(d.nint);
        for (int j = 0; j < d.nint; j++) d.A[i][d.ncont + j] = row[j];
    }

    d.b  = readDoubleVec(d.m); // right-hand side: m values on one line

    d.ty.resize(d.m);
    std::vector<double> tyDouble = readDoubleVec(d.m);
    for (int i = 0; i < d.m; i++) d.ty[i] = (int)tyDouble[i];

    // BUG A fix: read variable bounds for continuous variables.
    // If the lines are absent (older .dat files), fall back to lb=0, ub=1e30.
    d.lb.assign(d.ncont, 0.0);
    d.ub.assign(d.ncont, 1e30);
    if (d.ncont > 0 && !f.eof()) {
        std::string lbLine, ubLine;
        if (std::getline(f, lbLine) && !lbLine.empty()) {
            std::istringstream ss(lbLine);
            for (int i = 0; i < d.ncont; i++) ss >> d.lb[i];
        }
        if (std::getline(f, ubLine) && !ubLine.empty()) {
            std::istringstream ss(ubLine);
            for (int i = 0; i < d.ncont; i++) ss >> d.ub[i];
        }
    }

    return d;
}

// ============================================================
// Result returned by a single MILP or LP solve
// ============================================================
struct SolveResult {
    bool feasible;
    double objval;            // optimal objective value
    std::vector<double> x;   // full solution vector
    std::vector<double> xint; // integer variables only
    double f2val;             // value of f2 evaluated at x
    double f1val;             // value of f1 evaluated at x
};

// ============================================================
// Evaluates both objective functions f1 and f2 at a given solution
// ============================================================
void evalObjectives(const InstanceData& d, const std::vector<double>& x,
                    double& f1val, double& f2val)
{
    f1val = 0.0; f2val = 0.0;
    for (int i = 0; i < d.nvar; i++) {
        f1val += d.f1[i] * x[i];
        f2val += d.f2[i] * x[i];
    }
}

// ============================================================
// Optimises a Gurobi model and returns a SolveResult.
// [B] Receives the pre-built vars vector directly; no getVarByName() calls.
// ============================================================
SolveResult solveModel(GRBModel& model, const InstanceData& d,
                       const std::vector<GRBVar>& vars)
{
    SolveResult res;
    model.optimize();

    int status = model.get(GRB_IntAttr_Status);
    res.feasible = (status == GRB_OPTIMAL);

    if (res.feasible) {
        res.objval = model.get(GRB_DoubleAttr_ObjVal);
        res.x.resize(d.nvar);
        for (int i = 0; i < d.nvar; i++)
            res.x[i] = vars[i].get(GRB_DoubleAttr_X);   // [B] direct access
        res.xint.resize(d.nint);
        for (int i = 0; i < d.nint; i++)
            res.xint[i] = res.x[d.ncont + i];
        evalObjectives(d, res.x, res.f1val, res.f2val);
    } else {
        // Infeasible or unbounded: use 10000 as sentinel value
        res.f2val  = 10000.0;
        res.f1val  = 10000.0;
        res.objval = 10000.0;
    }
    return res;
}

// ============================================================
// Compatibility wrapper used by the one-shot solves
// (writeMPS_and_solve, solveF1AndUpdateSolint).
// Builds the vars vector from names once, then calls solveModel.
// ============================================================
static SolveResult solveModelByName(GRBModel& model, const InstanceData& d)
{
    std::vector<GRBVar> vars(d.nvar);
    for (int i = 0; i < d.nvar; i++)
        vars[i] = model.getVarByName("x" + std::to_string(i + 1));
    return solveModel(model, d, vars);
}

// ============================================================
// Writes solint_new.txt: integer fixings followed by the f2 value
// ============================================================
void writeSolint(const std::string& path, const SolveResult& res)
{
    std::ofstream f(path);
    if (!res.feasible) {
        f << 10000 << "\n";
        return;
    }
    for (double xi : res.xint)
        f << xi << "\n";
    f << res.f2val << "\n";
}

// ============================================================
// Writes yk.txt: f1 value at the ideal point (first ideal point component)
// ============================================================
void writeYk(const std::string& path, double val)
{
    std::ofstream f(path);
    f << val << "\n";
}

// ============================================================
// Writes z1.txt: value of f1 evaluated at x^{id,2} (PADMe stopping reference)
// ============================================================
void writeZ1(const std::string& path, double val)
{
    std::ofstream f(path);
    f << "\t" << val << "\n";
}

// ============================================================
// Writes data.txt: ncont, nint, and the f2 objective coefficients
// ============================================================
void writeDataTxt(const std::string& path, const InstanceData& d)
{
    std::ofstream f(path);
    f << d.ncont << "\n";
    f << d.nint  << "\n";
    for (int i = 0; i < d.nvar; i++)
        f << d.f2[i] << "\n";
}

// ============================================================
// Builds a one-shot Gurobi model.
// Used only by writeMPS_and_solve and solveF1AndUpdateSolint,
// which run before the persistent MILPk model is initialised.
// ============================================================
static GRBModel buildModelOneShot(GRBEnv& env, const InstanceData& d,
                                  bool isF2,
                                  double rhsF2 = 1e6,
                                  const std::vector<double>& xintFixed = {})
{
    GRBModel model(env);
    model.set(GRB_IntParam_OutputFlag, 0);

    std::vector<GRBVar> vars(d.nvar);
    for (int i = 0; i < d.nvar; i++) {
        double objCoef = isF2 ? d.f2[i] : d.f1[i];
        if (i < d.ncont)
            vars[i] = model.addVar(d.lb[i], d.ub[i], objCoef, GRB_CONTINUOUS,
                                   "x" + std::to_string(i+1));
        else
            vars[i] = model.addVar(0.0, 1.0, objCoef, GRB_BINARY,
                                   "x" + std::to_string(i+1));
    }
    model.update();

    for (int i = 0; i < d.m; i++) {
        GRBLinExpr lhs = 0;
        for (int j = 0; j < d.nvar; j++)
            if (d.A[i][j] != 0.0)
                lhs += d.A[i][j] * vars[j];
        if (d.ty[i] == 1)
            model.addConstr(lhs <= d.b[i], "c" + std::to_string(i+1));
        else
            model.addConstr(lhs == d.b[i], "c" + std::to_string(i+1));
    }

    if (!isF2) {
        GRBLinExpr f2expr = 0;
        for (int i = 0; i < d.nvar; i++)
            if (d.f2[i] != 0.0) f2expr += d.f2[i] * vars[i];
        model.addConstr(f2expr <= rhsF2, "c_f2bound");
    }

    if (!xintFixed.empty())
        for (int i = 0; i < d.nint; i++) {
            vars[d.ncont + i].set(GRB_DoubleAttr_LB, xintFixed[i]);
            vars[d.ncont + i].set(GRB_DoubleAttr_UB, xintFixed[i]);
        }

    model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);
    return model;
}

// ============================================================
// Reads the instance, solves the MILP minimising f2, and writes all
// output files needed to start the PADMe iteration.
// ============================================================
InstanceData writeMPS_and_solve(const std::string& datFile,
                                const std::string& solintPath,
                                const std::string& z1Path,
                                const std::string& ykPath = "")
{
    InstanceData d = readInstance(datFile);
    writeDataTxt("data.txt", d);

    GRBEnv env;
    env.set(GRB_IntParam_OutputFlag, 0);
    GRBModel modelF2 = buildModelOneShot(env, d, /*isF2=*/true);
    SolveResult resF2 = solveModelByName(modelF2, d);

    writeSolint(solintPath, resF2);
    if (!ykPath.empty())
        writeYk(ykPath, resF2.feasible ? resF2.f1val : 10000.0);
    writeZ1(z1Path, resF2.feasible ? resF2.f1val : 10000.0);

    return d;
}

// ============================================================
// Solves the MILP minimising f1 over the original problem constraints
// and writes its optimal solution to solint_new.txt.
// ============================================================
SolveResult solveF1AndUpdateSolint(const std::string& datFile,
                                   const std::string& solintPath)
{
    InstanceData d = readInstance(datFile);
    GRBEnv env;
    env.set(GRB_IntParam_OutputFlag, 0);
    GRBModel modelF1 = buildModelOneShot(env, d, /*isF2=*/false);
    SolveResult resF1 = solveModelByName(modelF1, d);
    writeSolint(solintPath, resF1);
    return resF1;
}

// ============================================================
// IterState — persistent state across calls to updateMPS_and_solve
//
// New fields vs. the original:
//   env        [D] single GRBEnv shared for all Gurobi models in the session
//   milpModel  [A] persistent MILPk model; never rebuilt from scratch
//   vars       [B] direct GRBVar references; no per-iteration name lookups
//   f2Constr   [A] handle to the f2 <= rhs constraint, updated in-place each iter
//   modelReady     flag: true once initMilpModel() has been called
// ============================================================
struct IterState {
    int k;   // current iteration counter
    int m;   // number of original constraints
    std::vector<std::vector<double>> noGoodXints; // integer fixings accumulated across iterations
    std::vector<double>              noGoodF2s;   // f2 values accumulated across iterations

    // [D] Shared Gurobi environment — initialised once, reused everywhere
    GRBEnv   env;
    // [A] Persistent MILPk model
    GRBModel milpModel;
    // [B] Direct references to the model variables
    std::vector<GRBVar> vars;
    // [A] Handle to the f2 upper-bound constraint (updated in-place each iteration)
    GRBConstr f2Constr;
    bool modelReady = false;

    // env must be initialised before milpModel in the constructor
    IterState() : env(), milpModel(env) {
        env.set(GRB_IntParam_OutputFlag, 0);
    }
};

// ============================================================
// Builds the base MILPk model once and stores it in state.
// Called automatically on the first invocation of updateMPS_and_solve.
//
// The model contains:
//   - all problem variables (continuous with per-instance bounds, binary)
//   - all original constraints
//   - the f2(x) <= rhsF2 constraint  (stored in state.f2Constr for in-place updates)
//   - objective: minimise f1
//
// No-good constraints are added incrementally by updateMPS_and_solve. [C]
// ============================================================
static void initMilpModel(IterState& state, const InstanceData& d, double rhsF2)
{
    GRBModel& model = state.milpModel;
    model.set(GRB_IntParam_OutputFlag, 0);

    // No-good cuts added below (in updateMPS_and_solve) each exclude exactly
    // one specific 0/1 point and are otherwise completely inactive -- they
    // never bind at any fractional LP-relaxation solution, only at the one
    // exact integer point they were built to exclude. That makes them a
    // textbook case for Gurobi's lazy-constraint mechanism: rather than
    // being included in every node's LP relaxation (which, with hundreds of
    // them accumulated over a long run, adds real per-node overhead for
    // essentially zero relaxation-tightening benefit), a lazy constraint is
    // left out until Gurobi finds an integer-feasible candidate that
    // violates it, at which point it's pulled in and that candidate is
    // rejected. Must be enabled here, before the first optimize() call.
    model.set(GRB_IntParam_LazyConstraints, 1);

    // Add variables and store direct references [B]
    state.vars.resize(d.nvar);
    for (int i = 0; i < d.nvar; i++) {
        double objCoef = d.f1[i];   // objective: minimise f1
        if (i < d.ncont)
            state.vars[i] = model.addVar(d.lb[i], d.ub[i], objCoef,
                                         GRB_CONTINUOUS, "x" + std::to_string(i+1));
        else
            state.vars[i] = model.addVar(0.0, 1.0, objCoef,
                                         GRB_BINARY, "x" + std::to_string(i+1));
    }
    model.update();

    // Original constraints
    for (int i = 0; i < d.m; i++) {
        GRBLinExpr lhs = 0;
        for (int j = 0; j < d.nvar; j++)
            if (d.A[i][j] != 0.0) lhs += d.A[i][j] * state.vars[j];
        if (d.ty[i] == 1)
            model.addConstr(lhs <= d.b[i], "c" + std::to_string(i+1));
        else
            model.addConstr(lhs == d.b[i], "c" + std::to_string(i+1));
    }

    // f2 upper-bound constraint — [A] saved for in-place RHS updates
    {
        GRBLinExpr f2expr = 0;
        for (int i = 0; i < d.nvar; i++)
            if (d.f2[i] != 0.0) f2expr += d.f2[i] * state.vars[i];
        state.f2Constr = model.addConstr(f2expr <= rhsF2, "c_f2bound");
    }

    model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);
    model.update();
    state.modelReady = true;
}

// ============================================================
// Reads the integer fixings from solint_new.txt
// ============================================================
std::vector<double> readXintFromSolint(const std::string& solintPath, int nint)
{
    std::vector<double> xint(nint);
    std::ifstream f(solintPath);
    for (int i = 0; i < nint; i++) f >> xint[i];
    return xint;
}

// ============================================================
// updateMPS_and_solve — optimised version
//
// On the first call, builds the base model via initMilpModel() [A].
// On every subsequent call:
//   - updates the f2 constraint RHS in-place  (O(1))         [A]
//   - adds only the new no-good for the previous fixing       [C]
//   - reads the solution via direct var references            [B]
// ============================================================
void updateMPS_and_solve(const InstanceData& d,
                         IterState& state,
                         const std::string& solintPath,
                         const std::string& ykPath = "")
{
    // 1) Read the current integer fixing from solint_new.txt
    std::vector<double> xintPrev(d.nint);
    double f2Prev = 0.0;
    {
        std::ifstream f(solintPath);
        for (int i = 0; i < d.nint; i++) f >> xintPrev[i];
        f >> f2Prev;
    }

    // 2) Append the current fixing to the accumulated history
    state.noGoodXints.push_back(xintPrev);
    state.noGoodF2s.push_back(f2Prev);

    try {
        // [A] Build the base model once; reuse it on every subsequent call.
        if (!state.modelReady)
            initMilpModel(state, d, f2Prev);

        GRBModel& model = state.milpModel;

        // [A] Update the f2 RHS in-place — O(1), no model rebuild needed.
        // NOTE: Gurobi's C++ API only provides the pointer+length overload
        // of set() for batch RHS updates (no std::vector convenience
        // overload), so we build a length-1 array here instead.
        {
            GRBConstr constrs[] = {state.f2Constr};
            double    vals[]    = {f2Prev};
            model.set(GRB_DoubleAttr_RHS, constrs, vals, 1);
        }

        // [C] Add only the single new no-good for the fixing just read.
        //     All previous no-goods are already present in the persistent model.
        //
        //     No-good logic (same as the original updateMPS.m):
        //       if xint[i] == 1 (rounded): coef = +1, RHS += 1
        //       if xint[i] == 0 (rounded): coef = -1
        //     Resulting constraint: sum(coef[i] * x[ncont+i]) <= RHS - 1
        //
        //     BUG C fix (carried over from original): binary values from Gurobi
        //     may be 0.9999... or 1.0000001; use > 0.5 to round correctly.
        {
            GRBLinExpr ng = 0;
            double b_ng = -1.0;
            for (int i = 0; i < d.nint; i++) {
                double coef = (xintPrev[i] > 0.5) ? 1.0 : -1.0;
                if (xintPrev[i] > 0.5) b_ng += 1.0;
                ng += coef * state.vars[d.ncont + i];   // [B] direct access
            }
            GRBConstr ngConstr = model.addConstr(ng <= b_ng,
                "nogood_" + std::to_string(state.m + 1 +
                            (int)state.noGoodXints.size()));
            // Mark lazy -- see the comment in initMilpModel(). Level 1 is the
            // mildest setting (Gurobi still considers it fairly early); try
            // 2 or 3 if profiling shows it's still being pulled in too
            // often. Requires GRB_IntParam_LazyConstraints=1 on the model,
            // set once in initMilpModel().
            ngConstr.set(GRB_IntAttr_Lazy, 1);
        }

        model.update();

        // [B] Solve and extract solution using direct var references.
        SolveResult res = solveModel(model, d, state.vars);

        // 4) Write solint_new.txt with the new MILPk solution
        writeSolint(solintPath, res);

        // 5) Write yk.txt: zID1k = optimal f1 value = first component of ideal point
        if (!ykPath.empty())
            writeYk(ykPath, res.feasible ? res.objval : 10000.0);

    } catch (GRBException& e) {
        std::cerr << "Gurobi error in updateMPS_and_solve: " << e.getMessage() << "\n";
        // Write infeasible sentinel
        std::ofstream f(solintPath);
        f << 10000 << "\n";
    }
}

// ============================================================
// Computes the second component of the ideal point of the k-th slice problem
// by solving an LP with the integer variables fixed.
//
// [D] Receives IterState& to reuse state.env instead of creating a new GRBEnv.
// [E] Integer variables are declared as GRB_CONTINUOUS with lb = ub = fixed value,
//     so no post-hoc relaxation loop is needed.
//
// Parameters:
//   d          : instance data
//   xintFixed  : current integer fixings (read from solint_new.txt by the caller)
//   id2kPath   : path to id2k.txt (written with the optimal f2 value)
//   state      : iteration state — provides the shared GRBEnv  [D]
// ============================================================
double computeIdealPoint2(const InstanceData& d,
                          const std::vector<double>& xintFixed,
                          const std::string& id2kPath,
                          IterState& state)
{
    try {
        // [D] Reuse the shared env; no new GRBEnv allocation per call.
        GRBModel model(state.env);
        model.set(GRB_IntParam_OutputFlag, 0);

        // Build the LP: minimise f2 with integer variables pinned to xintFixed.
        // [E] Integer variables are GRB_CONTINUOUS with lb = ub = fixed value;
        //     this avoids the separate relaxation loop present in the original.
        std::vector<GRBVar> vars(d.nvar);
        for (int i = 0; i < d.nvar; i++) {
            double objCoef = d.f2[i];
            if (i < d.ncont) {
                vars[i] = model.addVar(d.lb[i], d.ub[i], objCoef,
                                       GRB_CONTINUOUS, "x" + std::to_string(i+1));
            } else {
                double fixedVal = xintFixed[i - d.ncont];
                vars[i] = model.addVar(fixedVal, fixedVal, objCoef,
                                       GRB_CONTINUOUS, "x" + std::to_string(i+1));
            }
        }
        model.update();

        for (int i = 0; i < d.m; i++) {
            GRBLinExpr lhs = 0;
            for (int j = 0; j < d.nvar; j++)
                if (d.A[i][j] != 0.0) lhs += d.A[i][j] * vars[j];
            if (d.ty[i] == 1)
                model.addConstr(lhs <= d.b[i], "c" + std::to_string(i+1));
            else
                model.addConstr(lhs == d.b[i], "c" + std::to_string(i+1));
        }

        model.set(GRB_IntAttr_ModelSense, GRB_MINIMIZE);
        model.update();

        SolveResult res = solveModel(model, d, vars);
        double fxk = res.feasible ? res.objval : 10000.0;

        // Write id2k.txt with the optimal f2 value
        std::ofstream f(id2kPath);
        if (res.feasible)
            f << std::scientific << fxk << "\n";
        else
            f << 10000 << "\n";

        return fxk;

    } catch (GRBException& e) {
        std::cerr << "Gurobi error in computeIdealPoint2: " << e.getMessage() << "\n";
        std::ofstream f(id2kPath);
        f << 10000 << "\n";
        return 10000.0;
    }
}

#endif /* GurobiMPS_h */
