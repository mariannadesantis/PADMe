//
//  main_check_ideal_point.cpp
//  BOMILP_2024 - PADMe algorithm: ideal point checking variant
//
//  Authors: Lavinia Amorosi, Marianna De Santis
//
//  Difference from main_check_cuts.cpp:
//    Before solving a BOLP, the ideal point of the current slice problem is
//    computed. If the ideal point is dominated by all active cutting planes,
//    the BOLP is skipped entirely, saving computation.
//

#include <stdlib.h>
#include <cstdlib>
#include <stdio.h>
#include <sstream>
#include <iostream>
#include <string>
#include <cstring>
#include <cmath>
#include <chrono>
#include "Header.h"
#include "GurobiMPS.h"
#include "dichotomic_search.hpp"
#include "BolpDichotomic.h"
#include <sys/time.h>

// Wall-clock timing helper. NOTE: we deliberately use std::chrono's
// steady_clock (elapsed real time) rather than clock()/CLOCKS_PER_SEC
// (CPU time). clock() on Linux sums CPU-seconds across ALL threads of the
// process, and Gurobi solves using multiple threads by default -- so a
// MILP solve that takes 2 real seconds on 8 threads would be reported by
// clock() as ~16 "seconds", growing/shrinking with however many threads
// Gurobi happens to use on a given solve. steady_clock instead measures
// the actual elapsed time a person would experience, independent of
// threading, which is what all the timing breakdown below is meant to show.
using Clock = std::chrono::steady_clock;
static inline double elapsedSeconds(const Clock::time_point& t0) {
    return std::chrono::duration<double>(Clock::now() - t0).count();
}

node *tree = NULL;

int main(int argc, const char * argv[])
{
    // ── File paths ────────────────────────────────────────────────────────────
    // Directory containing the instance .dat files and where all working/
    // output files are written. Defaults to the current working directory;
    // override by setting the BOMILP_INSTANCES_DIR environment variable,
    // e.g.:  export BOMILP_INSTANCES_DIR=/path/to/instances
    std::string BASE = "./";
    if (const char* envBase = std::getenv("BOMILP_INSTANCES_DIR")) {
        BASE = envBase;
        if (!BASE.empty() && BASE.back() != '/') BASE += '/';
    }

    // Fraction of `cols` (continuous variables in the current BOLP slice)
    // used as a soft cap on the number of LP solves ("LPs-Dichotomic")
    // dichotomic search is allowed to spend on any single BOLP.
    //   1.0            -> no cap: exact supported frontier (original behavior)
    //   e.g. 0.2 (20%) -> approximate frontier, ~5x fewer LP solves per BOLP
    // NOTE: values < 1.0 weaken the soundness of the cut-based discard test
    // (see the comment on solveBOLPDichotomic() in BolpDichotomic.h), since
    // it then reasons over a possibly-incomplete frontier.
    //
    // Optional 3rd command-line argument. Usage:
    //   ./padme <instance_name> <it_index> [budget_fraction]
    // If omitted, defaults to 1.0 (unlimited/exact, original behavior).
    double BOLP_ORACLE_BUDGET_FRACTION = 1.0;
    if (argc >= 4) {
        try {
            BOLP_ORACLE_BUDGET_FRACTION = std::stod(argv[3]);
        } catch (const std::exception&) {
            std::cerr << "Warning: could not parse budget fraction '" << argv[3]
                      << "', defaulting to 1.0 (unlimited).\n";
            BOLP_ORACLE_BUDGET_FRACTION = 1.0;
        }
        if (BOLP_ORACLE_BUDGET_FRACTION <= 0.0 || BOLP_ORACLE_BUDGET_FRACTION > 1.0) {
            std::cerr << "Warning: budget fraction " << BOLP_ORACLE_BUDGET_FRACTION
                      << " out of range (0, 1], defaulting to 1.0 (unlimited).\n";
            BOLP_ORACLE_BUDGET_FRACTION = 1.0;
        }
    }

    // Gurobi's MIPGap for the MILPk branch-and-bound solves. Gurobi's own
    // default is 1e-4 (B&B stops once within 0.01% of the true optimum).
    // Raising it trades exactness for speed -- the no-good-constraint /
    // cut-discard logic assumes each MILPk's reported f1 IS the true
    // optimum, so a looser gap can silently shift the ideal point, make the
    // extreme-inequality cut checks wrong near the margin, or miss a
    // genuinely non-dominated point close to one already found. Sanity-check
    // the resulting Pareto front against an exact run before trusting a
    // looser value on larger instances.
    //
    // Optional 4th command-line argument. Usage:
    //   ./padme <instance_name> <it_index> [budget_fraction] [mip_gap]
    // If omitted, defaults to 1e-4 (Gurobi's own default).
    double GRB_MIP_GAP = 1e-4;
    if (argc >= 5) {
        try {
            GRB_MIP_GAP = std::stod(argv[4]);
        } catch (const std::exception&) {
            std::cerr << "Warning: could not parse MIP gap '" << argv[4]
                      << "', defaulting to 1e-4 (Gurobi's default).\n";
            GRB_MIP_GAP = 1e-4;
        }
        if (GRB_MIP_GAP < 0.0) {
            std::cerr << "Warning: MIP gap " << GRB_MIP_GAP
                      << " is negative, defaulting to 1e-4 (Gurobi's default).\n";
            GRB_MIP_GAP = 1e-4;
        }
    }

    // Whether to print progress messages (std::cout) during the run.
    // Optional 5th command-line argument, parsed as an int: 0 = silent,
    // anything else = verbose. Usage:
    //   ./padme <instance_name> <it_index> [budget_fraction] [mip_gap] [verbose]
    // If omitted, defaults to 1 (verbose, original behavior).
    bool VERBOSE = true;
    if (argc >= 6) {
        try {
            VERBOSE = (std::stoi(argv[5]) != 0);
        } catch (const std::exception&) {
            std::cerr << "Warning: could not parse verbose flag '" << argv[5]
                      << "', defaulting to verbose (1).\n";
            VERBOSE = true;
        }
    }

    const std::string inst_name  = argv[1];   // instance name (without extension)
    const std::string itIdx = argv[2];   // iteration-file index (e.g. "1")

    const std::string pathInstance    = BASE + inst_name + ".dat";            // BOMILP instance
    const std::string pathSolint   = BASE + "solint_new.txt";         // integer fixings + f2 value
    const std::string pathZ1= BASE + "z1.txt";                 // f1 value at x^{id,2} (stop ref.)
    const std::string pathYk  = BASE + "yk.txt";                 // f1 component of ideal point
    const std::string pathId2k= BASE + "id2k.txt";              // f2 component of ideal point
    const std::string pathIterIn   = BASE + "it" + itIdx + ".txt";   // iteration counter input
    const std::string pathIterOut   = BASE + "it_new.txt";             // updated iteration counter
    // NOTE: pathBensSolMain/pathBensSolCut/pathVlpMain/pathVlpCut and the
    // .vlp/.sol file round-trip through BENSOLVE are no longer needed: the
    // BOLP is now solved in-process via Gurobi + dichotomic search (see
    // BolpDichotomic.h), so there is nothing left to write or parse.

    std::ostringstream pathResults;
    pathResults << BASE + inst_name + "_solution_ideal_point.txt";
    //const std::string pathResults   = BASE + inst_name + "_solution_ideal_point.txt";

    // ── Algorithm data structures ─────────────────────────────────────────────
    InstanceData inst;   // instance data loaded from .dat file
    IterState    itState;  // iteration state for no-good constraints
    // Persistent Gurobi state for the BOLP slice problem, reusing itState's
    // shared GRBEnv. Built once on first use, then only RHS-updated.
    BolpState    bolpState(itState.env);

    // ── Gurobi MILP tuning ──────────────────────────────────────────────────
    // The CPU-time breakdown at the end of the results file shows MILP
    // branch-and-bound (updateMPS_and_solve) dominates total runtime (>98% in
    // typical runs). MIPGap is the parameter that actually moves the needle;
    // see the warning above where GRB_MIP_GAP is parsed. Applied once here,
    // before initMilpModel() ever builds/solves anything, so it's in effect
    // for every MILPk solve for the rest of the run.
    itState.milpModel.set(GRB_DoubleParam_MIPGap, GRB_MIP_GAP);

    // Force single-threaded solves. IMPORTANT: a GRBModel snapshots its
    // environment's parameters at CONSTRUCTION time -- changes made to the
    // environment afterward do NOT retroactively propagate to models that
    // already exist. itState.milpModel and bolpState.model were both
    // already constructed above, so Threads must be set directly on each
    // of them; env.set() alone would silently do nothing for those two.
    // computeIdealPoint2() builds a fresh GRBModel(state.env) on every
    // call, so for that one the env-level setting below is sufficient.
    //
    // Useful for reproducible, directly-comparable wall-clock timings (no
    // thread-count-dependent variance run to run) and for isolating
    // whether multithreading itself is contributing to any inconsistency.
    // Remove these lines (or set to 0, Gurobi's "use all available cores"
    // default) to go back to parallel.
    itState.milpModel.set(GRB_IntParam_Threads, 1);
    bolpState.model.set(GRB_IntParam_Threads, 1);
    itState.env.set(GRB_IntParam_Threads, 1); // covers computeIdealPoint2()

    std::vector<double> intFixings;
    std::vector<double> idealPoint;  // ideal point of the current slice problem
    // List of all non-dominated extreme points found across all integer fixings
    std::list<std::vector<double>> extremePointsAll;
    // Non-dominated extreme points for the current (k-th) integer fixing
    std::list<std::vector<double>> extremePointsCurr;
    // Temporary buffer (used in the cut variant; kept for structural symmetry)
    std::list<std::vector<double>> extremePointsCurrBuf;

    std::list<std::vector<double>>::iterator it;

    // ── Step 1-2: Write MPS files and solve the initial MILPs ─────────────────
    // Reads the instance, solves the MILP minimising f2, writes:
    //   solint_new.txt : integer fixings and f2 value
    //   z1.txt         : f1 value at x^{id,2}  (PADMe stopping reference)
    //   yk.txt         : first component of the ideal point
    Clock::time_point startTime = Clock::now();

    // ── Timing breakdown ──────────────────────────────────────────────────────
    // Accumulators for where CPU time actually goes, so a change like the
    // dichotomic-search budget can be judged against the real bottleneck
    // instead of assumed. Printed to the results file at the end.
    double milpSolveTime = 0.0;       // all genuine MILP (branch-and-bound) solves
    double bolpSolveTime = 0.0;       // all solveBOLPDichotomic() calls
    double idealPointSolveTime = 0.0; // computeIdealPoint2() LP solves

    Clock::time_point t0 = Clock::now();
    inst = writeMPS_and_solve(pathInstance, pathSolint, pathZ1, pathYk);
    milpSolveTime += elapsedSeconds(t0);
    itState.m = inst.m;
    itState.k = 0;

    // z  = history of f2 optimal values (one entry per MILP solve)
    // y  = history of f1 values at each ideal point (zID1k)
    std::vector<double> z, y;
    update_z(pathSolint,    z);   // f2 optimal value from the initial MILP
    update_z(pathZ1, y);   // f1 value at x^{id,2} (stopping threshold)
    
    
    //Reads the instance, solves the MILP minimising f1, writes:
    t0 = Clock::now();
    SolveResult sr = solveF1AndUpdateSolint(pathInstance, pathSolint);
    milpSolveTime += elapsedSeconds(t0);
    

    // ── Read instance dimensions and build VLP data structures ────────────────
    int rows, cols, numIntVars;
    read_file(pathInstance, rows, cols, numIntVars);

    double **B1, **B2, **P, *b, *c, *bShifted;
    int intVarIdx[numIntVars + 1];
    B1   = new double*[rows + 1];
    B2   = new double*[rows + 1];
    P    = new double*[5];
    b    = new double[rows + 1];
    c    = new double[rows + 1];
    bShifted = new double[rows + 1];
    double M;
    int NB = 0, NP1 = 0, NP2 = 0, NP = 0;
    double z1Fix, z2Fix;
    *intVarIdx = intVarIdx[numIntVars + 1];

    // Populate the VLP data structures from the instance file
    store_data(pathInstance, numIntVars, intVarIdx, rows, cols, B1, B2, P, b, c, NB, NP1, NP2, NP, bShifted, z1Fix, z2Fix);
    // Set up integer-fixing data from the first MILP solution
    intFixings = store_data_int_fixing(pathSolint, intVarIdx, bShifted, b, M, numIntVars, P, B2, z1Fix, z2Fix, rows);

    // Count non-zeros in B1
    for (int k = 1; k <= rows; k++)
        for (int m = 1; m <= cols; m++)
            if (B1[k][m] != 0) NB++;

    // Count non-zeros in P (objective rows 1 and 3)
    for (int i = 1; i <= 4; i++) {
        for (int j = 1; j <= cols; j++) {
            if ((i == 1) && (P[i][j] != 0)) NP1++;
            if ((i == 3) && (P[i][j] != 0)) NP2++;
        }
        NP = NP1 + NP2;
    }

    
    // ── Step 3-4: Solve the initial BOLP via Gurobi + dichotomic search ───────
    // (replaces build_vlp_new() + bensolve() + define_list_extreme_solutions_k())
    int numBolp = 0;
    if (VERBOSE) std::cout<<"Solving initial BOLP (dichotomic search)...\n";
    t0 = Clock::now();
    std::list<std::vector<double>> newPoints =
        solveBOLPDichotomic(rows, cols, numIntVars, B1, B2, P, b, c, intVarIdx,
                             M, z1Fix, z2Fix, bolpState, BOLP_ORACLE_BUDGET_FRACTION);
    bolpSolveTime += elapsedSeconds(t0);
    numBolp++;

    // ── Steps 5-6: Build the list of non-dominated extreme points ─────────────
    extremePointsAll  = newPoints;
    extremePointsCurr = newPoints;
    
    if (VERBOSE) {
        std::cout<<"Initial list of extreme points";
        print_list_vector(extremePointsCurr);
    }

    // ── Initial tree construction ─────────────────────────────────────────────
    if (extremePointsCurr.size() > 1) {
        int type = 2;
        for (it = extremePointsCurr.begin(); it != std::prev(extremePointsCurr.end()); it++) {
            insert2(type,
                    *it->begin(), *next(it->begin()),
                    *next(it)->begin(), *next(next(it)->begin()),
                    0.0, &tree, NULL);
        }
    } else {
        it = extremePointsCurr.begin();
        insert2(1,
                *it->begin(), *next(it->begin()),
                0.0, 0.0, 0.0, &tree, NULL);
    }

    // Update z with the f2 value from the latest MILP solve
    update_z(pathSolint, z);

    int iterCount = 2;

    // ── Main iteration loop ───────────────────────────────────────────────────
    // Continue while the latest f2 value is strictly greater than the minimum
    // f2 found so far, and the last MILP was not infeasible/unbounded (f2=10000).
    while ((*std::prev(z.end()) > *z.begin()) && (*std::prev(z.end()) != 10000))
    {
        idealPoint.clear();
        update_it(pathIterIn, pathIterOut, iterCount);

        // ── Step 7: Build and solve MILPk (add no-good constraint, minimize f1) ──
        itState.k = iterCount - 1;
        
        if (VERBOSE) std::cout<<"Building and solving MILPk...\n";
        t0 = Clock::now();
        updateMPS_and_solve(inst, itState, pathSolint, pathYk);
        milpSolveTime += elapsedSeconds(t0);
        iterCount++;

        // Append the new f2 and f1 values from MILPk
        update_z(pathSolint,  z);   // f2 value of the new integer fixing
        update_z(pathYk, y);   // f1 value (first component of the ideal point, zID1k)

        // If MILPk is infeasible or unbounded, stop
        if (*std::prev(z.end()) == 10000)
            break;

        // Update integer-fixing data from the new MILPk solution
        intFixings = store_data_int_fixing(pathSolint, intVarIdx, bShifted, b, M, numIntVars, P, B2, z1Fix, z2Fix, rows);

        // ── Ideal point check ─────────────────────────────────────────────────────────
        // Only proceed if the f1 component of the current ideal point is <=
        // the stopping threshold (f1 value at x^{id,2}).
        if (*std::prev(y.end()) <= *y.begin())
        {
            // Compute the second component of the ideal point of the k-th
            // slice problem by solving an LP with the integer variables fixed.
            // itState is passed so that computeIdealPoint2 can reuse the
            // shared GRBEnv instead of creating a new one on every call.
            {
                std::vector<double> xintFixed = readXintFromSolint(pathSolint, inst.nint);
                t0 = Clock::now();
                computeIdealPoint2(inst, xintFixed, pathId2k, itState);
                idealPointSolveTime += elapsedSeconds(t0);
            }
            // Build the full ideal point: (zID1k, zID2k)
            idealPoint.push_back(*std::prev(y.end()));
            update_z(pathId2k, idealPoint);

            // ── Cut-feasibility check ─────────────────────────────────────────────
            // For each pair of consecutive extreme points, derive a weight vector
            // and check whether the current ideal point satisfies the corresponding
            // cutting plane. Count how many cuts the ideal point violates (numCutsDominating).
            int numCutsDominating = 0;

            if (extremePointsCurr.size() > 1)
            {
                double w1 = 0, w2 = 0, w1Prev = 0, w2Prev = 0, cutRhs = 0;

                for (it = extremePointsCurr.begin();
                     it != std::prev(extremePointsCurr.end()); it++)
                {
                    w1Prev = w1;
                    w2Prev = w2;
                    // w1 = f2_i - f2_{i+1}
                    w1 = *std::next(it->begin()) - *std::next(std::next(it)->begin());
                    // w2 = f1_{i+1} - f1_i
                    w2 = *std::next(it)->begin() - *it->begin();

                    // Skip if weight vector is too close to the previous one
                    if (sqrt(pow(w1 - w1Prev, 2) + pow(w2 - w2Prev, 2)) > 0.1)
                    {
                        double norm = sqrt(pow(w1, 2) + pow(w2, 2));
                        if (norm >= 0.01)
                        {
                            w1 /= norm;
                            w2 /= norm;

                            cutRhs = w1 * *it->begin()
                                     + w2 * *std::next(it->begin())
                                     - w1 * *intFixings.begin()
                                     - w2 * *std::next(intFixings.begin());

                            // If the ideal point satisfies this cut (is not dominated),
                            // the BOLP cannot be skipped: exit the cut-check loop.
                            bool idealNotCut = check_punto_ideale(cutRhs, w1, w2, idealPoint);
                            if (idealNotCut == true) {
                                if (VERBOSE) std::cout << "\nExiting cut-check: ideal point is not dominated by this cut.\n";
                                break;
                            } else {
                                numCutsDominating++;
                            }
                        }
                    }
                }
          //test edit 2026-06-18  }

            // ── Decide whether to solve the BOLP ─────────────────────────────
            // Solve if: not all cuts dominate the ideal point, OR the second
            // component of the ideal point is within the range of the current
            // extreme-point list.
            if ((numCutsDominating < (int)extremePointsCurr.size() - 1) ||
                (*std::next(idealPoint.begin()) <=
                 *std::next(std::prev(extremePointsCurr.end())->begin())))
            {
                if (VERBOSE) std::cout << "\nSolving BOLP for iteration " << iterCount - 2 << "\n";

                Clock::time_point tBolp0 = Clock::now();
                std::list<std::vector<double>> newPoints =
                    solveBOLPDichotomic(rows, cols, numIntVars, B1, B2, P, b, c, intVarIdx,
                                         M, z1Fix, z2Fix, bolpState, BOLP_ORACLE_BUDGET_FRACTION);
                bolpSolveTime += elapsedSeconds(tBolp0);
                numBolp++;

                mergeExtremePoints(extremePointsAll, newPoints);
                extremePointsCurr = newPoints;

                // Insert new segments/points into the tree
                if (extremePointsCurr.size() > 1) {
                    int type = 2;
                    for (it = extremePointsCurr.begin();
                         it != std::prev(extremePointsCurr.end()); it++) {
                        insert2(type,
                                *it->begin(), *next(it->begin()),
                                *next(it)->begin(), *next(next(it)->begin()),
                                0.0, &tree, NULL);
                    }
                } else if (extremePointsCurr.size() == 1) {
                    it = extremePointsCurr.begin();
                    insert2(1,
                            *it->begin(), *next(it->begin()),
                            0.0, 0.0, 0.0, &tree, NULL);
                }
            } else {
                if (VERBOSE) std::cout << "\nSkipping BOLP: ideal point is dominated by all active cuts.\n";
            }
        }
        else
        {
            // The f1 component of the ideal point exceeds the stopping threshold:
            // solve the BOLP unconditionally (no cut-feasibility check needed).
            if (VERBOSE) std::cout << "\nf1(ideal) > stopping threshold: solving BOLP unconditionally.\n";

            Clock::time_point tBolp1 = Clock::now();
            std::list<std::vector<double>> newPoints =
                solveBOLPDichotomic(rows, cols, numIntVars, B1, B2, P, b, c, intVarIdx,
                                     M, z1Fix, z2Fix, bolpState, BOLP_ORACLE_BUDGET_FRACTION);
            bolpSolveTime += elapsedSeconds(tBolp1);
            numBolp++;

            mergeExtremePoints(extremePointsAll, newPoints);
            extremePointsCurr = newPoints;

            if (extremePointsCurr.size() > 1) {
                int type = 2;
                for (it = extremePointsCurr.begin();
                     it != std::prev(extremePointsCurr.end()); it++) {
                    insert2(type,
                            *it->begin(), *next(it->begin()),
                            *next(it)->begin(), *next(next(it)->begin()),
                            0.0, &tree, NULL);
                }
            } else if (extremePointsCurr.size() == 1) {
                it = extremePointsCurr.begin();
                insert2(1,
                        *it->begin(), *next(it->begin()),
                        0.0, 0.0, 0.0, &tree, NULL);
            }
        }
    }
    } //test edit 2026-06-18
    // ── End main loop ─────────────────────────────────────────────────────────

    // Elapsed WALL-CLOCK time in seconds (print_time_on_txt expects seconds).
    // See the note near the top of the file on why this is steady_clock-based
    // rather than clock()/CLOCKS_PER_SEC.
    double cpuTime = elapsedSeconds(startTime);

    // ── Write results to output file ──────────────────────────────────────────
    FILE *file;
   
    std::string Sres;
    Sres=pathResults.str();
    char const *s = Sres.data();
    //char *s = pathResults.str().data();
    file = fopen(s, "w");
    print_inorder_on_txt(tree, 1, file);
    fclose(file);   // BUG FIX: must close before reopening, otherwise this
                     // stream's buffered writes can flush at exit AFTER the
                     // "a"-mode stream below, silently overwriting the
                     // summary lines it just wrote.
    file = fopen(s, "a");
    print_time_on_txt(file, cpuTime, iterCount, numBolp);
    // Total number of Gurobi solves performed inside solveBOLPDichotomic()
    // across the whole run (i.e. every weighted-sum LP query dichotomic
    // search made while building the Pareto frontier for every BOLP slice).
    fprintf(file, "%ld LPs-Dichotomic\n", bolpState.numDichotomicSolves);

    // ── CPU time breakdown by phase ──────────────────────────────────────────
    // "other" (file I/O, tree bookkeeping, the various update_z/update_it/
    // store_data_int_fixing reads, etc.) is whatever's left over from the
    // total once the three instrumented phases are subtracted out.
    double otherTime = cpuTime - milpSolveTime - bolpSolveTime - idealPointSolveTime;
    if (otherTime < 0.0) otherTime = 0.0; // guard against timer rounding
    fprintf(file, "%lf sec MILP solves\n", milpSolveTime);
    fprintf(file, "%lf sec BOLP/dichotomic solves\n", bolpSolveTime);
    fprintf(file, "%lf sec ideal-point LP solves\n", idealPointSolveTime);
    fprintf(file, "%lf sec other (I/O, bookkeeping)\n", otherTime);
    fclose(file);

    // Free memory used by the tree
    destroy_tree(tree);

    return 0;
}
