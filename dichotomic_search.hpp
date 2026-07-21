// dichotomic_search.hpp
//
// Solver-agnostic dichotomic search for bi-objective (minimization)
// problems. See dichotomic_search.cpp (toy brute-force oracle) and
// dichotomic_search_gurobi.cpp (Gurobi oracle) for usage examples.

#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <queue>
#include <vector>

// A point in objective space (f1, f2), both to be minimized.
struct Point {
    double f1;
    double f2;

    bool approxEqual(const Point& other, double tol) const {
        return std::fabs(f1 - other.f1) < tol && std::fabs(f2 - other.f2) < tol;
    }
};

// A normalized weight vector (w1, w2), w1 + w2 = 1, w1,w2 >= 0.
struct Weight {
    double w1;
    double w2;
};

// The oracle solves:  min w1*f1(x) + w2*f2(x)  over the feasible set,
// and returns the resulting point in objective space.
using Oracle = std::function<Point(const Weight&)>;

class DichotomicSearch {
public:
    // maxOracleCalls <= 0 means "unlimited": find the exact supported
    // frontier (identical result to the original unbounded algorithm --
    // only the exploration ORDER changes, not the final point set).
    //
    // A positive maxOracleCalls caps the total number of oracle calls
    // (including the 2 calls used to find the two extreme points),
    // trading exactness for speed: once the budget is used up, the search
    // stops and returns whatever supported extreme points it has found so
    // far. Those points are still genuinely non-dominated -- just possibly
    // an incomplete subset of the true frontier. Segments are always
    // explored largest/most-promising gap first, so a budget-limited run
    // stays evenly spread across the whole frontier rather than exhausting
    // the budget on one corner of it.
    explicit DichotomicSearch(Oracle oracle, double tol = 1e-7,
                               int maxOracleCalls = -1)
        : oracle_(std::move(oracle)), tol_(tol), maxOracleCalls_(maxOracleCalls) {}

    std::vector<Point> run() {
        oracleCalls_ = 0;
        results_.clear();

        // Step 1: find the two extreme points by optimizing each
        // objective alone. Always done regardless of budget -- these are
        // the two cheapest, most informative points available.
        Point a = callOracle(Weight{1.0, 0.0});   // minimizes f1
        Point b = callOracle(Weight{0.0, 1.0});   // minimizes f2
        addPoint(a);
        addPoint(b);

        // Normalization ranges so segment "size" is compared on a common
        // scale regardless of how f1 and f2 differ in raw magnitude --
        // otherwise whichever objective happens to have larger values
        // would dominate the priority ordering.
        range1_ = std::fabs(b.f1 - a.f1);
        range2_ = std::fabs(a.f2 - b.f2);
        if (range1_ < 1e-12) range1_ = 1.0;
        if (range2_ < 1e-12) range2_ = 1.0;

        // Step 2: explore the gap between a and b, largest/most-promising
        // segment first, until either the frontier is fully resolved or
        // the oracle-call budget runs out.
        std::priority_queue<Segment, std::vector<Segment>, SegmentLess> pq;
        pushSegment(pq, a, b);

        while (!pq.empty()) {
            if (maxOracleCalls_ > 0 && oracleCalls_ >= maxOracleCalls_) break;

            Segment seg = pq.top();
            pq.pop();
            if (seg.a.approxEqual(seg.b, tol_)) continue;

            Weight w = segmentWeight(seg.a, seg.b);
            Point c = callOracle(w);

            double valueOnSegment = weightedValue(w, seg.a); // == weightedValue(w, seg.b)
            double valueAtC = weightedValue(w, c);

            // If c is (numerically) on the segment, or worse, there is no
            // supported point strictly between a and b for this weight:
            // this segment is fully resolved, discard it.
            if (valueAtC > valueOnSegment - tol_) continue;

            // c is a genuinely new efficient point strictly improving the
            // segment: record it and queue both new sub-segments.
            addPoint(c);
            pushSegment(pq, seg.a, c);
            pushSegment(pq, c, seg.b);
        }

        std::sort(results_.begin(), results_.end(),
                  [](const Point& p, const Point& q) { return p.f1 < q.f1; });
        return results_;
    }

    // Number of oracle (LP) calls made during the most recent run().
    int oracleCallCount() const { return oracleCalls_; }

private:
    struct Segment {
        Point a, b;
        double priority; // larger = more promising to explore next
    };
    struct SegmentLess {
        // std::priority_queue is a max-heap over this comparator's
        // ordering: returning true means "lhs is LOWER priority than
        // rhs", i.e. rhs should come out of the queue first.
        bool operator()(const Segment& lhs, const Segment& rhs) const {
            return lhs.priority < rhs.priority;
        }
    };

    Oracle oracle_;
    double tol_;
    int maxOracleCalls_;
    int oracleCalls_ = 0;
    double range1_ = 1.0, range2_ = 1.0;
    std::vector<Point> results_;

    Point callOracle(const Weight& w) {
        oracleCalls_++;
        return oracle_(w);
    }

    void pushSegment(std::priority_queue<Segment, std::vector<Segment>, SegmentLess>& pq,
                      const Point& a, const Point& b) {
        double dx = (a.f1 - b.f1) / range1_;
        double dy = (a.f2 - b.f2) / range2_;
        pq.push(Segment{a, b, dx * dx + dy * dy});
    }

    // Weight vector orthogonal to segment (a,b), oriented so that the
    // weighted-sum objective is minimized *below* the segment for any point
    // that dominates the segment. Valid when f1(a) <= f1(b) and
    // f2(a) >= f2(b) (the usual trade-off ordering along the frontier).
    static Weight segmentWeight(const Point& a, const Point& b) {
        double w1 = a.f2 - b.f2;   // >= 0
        double w2 = b.f1 - a.f1;   // >= 0
        double sum = w1 + w2;
        if (sum < 1e-12) {
            // Degenerate: a and b coincide (shouldn't normally happen).
            return Weight{0.5, 0.5};
        }
        return Weight{w1 / sum, w2 / sum};
    }

    static double weightedValue(const Weight& w, const Point& p) {
        return w.w1 * p.f1 + w.w2 * p.f2;
    }

    void addPoint(const Point& p) {
        for (const auto& existing : results_) {
            if (existing.approxEqual(p, tol_)) return;
        }
        results_.push_back(p);
    }
};
