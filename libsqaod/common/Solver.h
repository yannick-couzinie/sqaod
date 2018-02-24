#pragma once

#include <common/Matrix.h>
#include <common/Preference.h>

namespace sqaod {


enum OptimizeMethod {
    /* These values should sync with python bind. */
    optNone = -1,
    optMinimize = 0,
    optMaximize = 1,
};

enum AnnealerState {
    annNone = 0,
    annRandSeedGiven = 1,
    annProblemSet = 2,
    annInitialized = 4,
    annQSet = 8,
    
    annQSetReady = (annProblemSet | annRandSeedGiven | annInitialized),

    annNTrottersGiven = 32,
};


template<class real>
struct Solver {
    virtual ~Solver() { }

    virtual Algorithm selectAlgorithm(Algorithm algo) = 0;
    
    virtual Algorithm getAlgorithm() const = 0;

    virtual Preferences getPreferences() const = 0;

    virtual void setPreference(const Preference &pref) = 0;

    void setPreferences(const Preferences &prefs);

    virtual const VectorType<real> &get_E() const = 0;
    
protected:
    Solver() : om_(optNone) { }

    OptimizeMethod om_;
};


template<class real>
struct BFSearcher : Solver<real> {
    virtual ~BFSearcher() { }

    virtual Algorithm selectAlgorithm(Algorithm algo);
    
    virtual Algorithm getAlgorithm() const;
    
    virtual void initSearch() = 0;

    virtual void finSearch() = 0;

    virtual void search() = 0;

protected:
    BFSearcher() { }
};


template<class real>
struct Annealer : Solver<real> {
    virtual ~Annealer() { }

    virtual Preferences getPreferences() const;

    virtual void setPreference(const Preference &pref);

    virtual void seed(unsigned int seed) = 0;

    virtual void randomize_q() = 0;

    virtual void initAnneal() = 0;

    virtual void finAnneal() = 0;

    virtual void calculate_E() = 0;
    
    virtual void annealOneStep(real G, real kT) = 0;

protected:
    Annealer() : annState_(annNone), m_(0) { }

    int annState_;
    SizeType m_;
};


template<class real>
struct DenseGraphSolver {
    virtual ~DenseGraphSolver() { }

    void getProblemSize(SizeType *N) const;

    virtual void setProblem(const MatrixType<real> &W,
                            OptimizeMethod om = sqaod::optMinimize) = 0;

    virtual const BitsArray &get_x() const = 0;

protected:
    DenseGraphSolver() : N_(0) { }

    SizeType N_;
};



template<class real>
struct BipartiteGraphSolver {
    virtual ~BipartiteGraphSolver() { }

    void getProblemSize(SizeType *N0, SizeType *N1) const;
    
    virtual void setProblem(const VectorType<real> &b0, const VectorType<real> &b1,
                            const MatrixType<real> &W, OptimizeMethod om = optMinimize) = 0;

    virtual const BitsPairArray &get_x() const = 0;

protected:
    BipartiteGraphSolver() : N0_(0), N1_(0) { }

    SizeType N0_, N1_;
};



template<class real>
struct DenseGraphBFSearcher
        : BFSearcher<real>, DenseGraphSolver<real> {
    virtual ~DenseGraphBFSearcher() { }

    virtual Preferences getPreferences() const;

    virtual void setPreference(const Preference &pref);

    virtual void searchRange(sqaod::PackedBits xBegin, sqaod::PackedBits xEnd) = 0;

    virtual void search();

protected:
    DenseGraphBFSearcher() :xMax_(0), tileSize_(0) { }
    
    PackedBits xMax_;
    SizeType tileSize_;
};


template<class real>
struct DenseGraphAnnealer
        : Annealer<real>, DenseGraphSolver<real> {
    virtual ~DenseGraphAnnealer() { }

    virtual void get_hJc(VectorType<real> *h, MatrixType<real> *J, real *c) const = 0;

    virtual void set_x(const Bits &x) = 0;

    virtual const BitsArray &get_q() const = 0;

protected:
    DenseGraphAnnealer() { }
};
    

template<class real>
struct BipartiteGraphBFSearcher
        : BFSearcher<real>, BipartiteGraphSolver<real> {
    virtual ~BipartiteGraphBFSearcher() { }

    virtual Preferences getPreferences() const;

    virtual void setPreference(const Preference &pref);

    virtual void searchRange(sqaod::PackedBits x0Begin, sqaod::PackedBits x0End,
                             sqaod::PackedBits x1Begin, sqaod::PackedBits x1End) = 0;


    virtual void search();

protected:
    BipartiteGraphBFSearcher()
            : x0max_(0), x1max_(0), tileSize0_(0), tileSize1_(0) { }

    PackedBits x0max_, x1max_;
    SizeType tileSize0_, tileSize1_;
};


template<class real>
struct BipartiteGraphAnnealer
        : Annealer<real>, BipartiteGraphSolver<real> {
    virtual ~BipartiteGraphAnnealer() { }

    virtual void get_hJc(VectorType<real> *h0, VectorType<real> *h1,
                         MatrixType<real> *J, real *c) const = 0;

    virtual void set_x(const Bits &x0, const Bits &x1) = 0;

    virtual const BitsPairArray &get_q() const = 0;

protected:
    BipartiteGraphAnnealer() { }
};


}

