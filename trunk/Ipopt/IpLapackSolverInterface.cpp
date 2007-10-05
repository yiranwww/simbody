
#include "IpLapackSolverInterface.hpp"
#include "SimTKlapack.h"


namespace Ipopt
{
#ifdef IP_DEBUG
  static const Index dbg_verbosity = 0;
#endif

int *ipiv;
double *afact;
  LapackSolverInterface::LapackSolverInterface()
      :
      n(0),
      nz(0),
      a(NULL),
      irn_(NULL),
      jcn_(NULL),
      negevals_(-1)
  {
    DBG_START_METH("LapackSolverInterface::LapackSolverInterface()", dbg_verbosity);
  }


  LapackSolverInterface::~LapackSolverInterface()
  {
    DBG_START_METH("LapackSolverInterface::~LapackSolverInterface()", dbg_verbosity);
//    dmumps_c(&mumps_data); /* Terminate instance */

    delete [] a;
    delete [] irn_;
    delete [] jcn_;
  }

  void LapackSolverInterface::RegisterOptions(SmartPtr<RegisteredOptions> roptions)
  {}

  bool LapackSolverInterface::InitializeImpl(const OptionsList& options,
      const std::string& prefix)
  {
    return true;
  }

  ESymSolverStatus LapackSolverInterface::MultiSolve(bool new_matrix, const Index* ia, const Index* ja,
      Index nrhs, double* rhs_vals, bool check_NegEVals,
      Index numberOfNegEVals)
  {
    int i;
    double *atmp;
    DBG_START_METH("LapackSolverInterface::MultiSolve", dbg_verbosity);
    DBG_ASSERT(!check_NegEVals || ProvidesInertia());
    //DBG_ASSERT(initialized_);


    ESymSolverStatus retval = SYMSOLVER_SUCCESS;

    // check if a factorization has to be done
    // perform the factorization

    atmp = new double[n*n];
    for(i=0;i<n*n;i++) atmp[i] = a[i];
    if (new_matrix) {
      retval = Factorization(ia, ja, check_NegEVals, numberOfNegEVals);
      if (retval == SYMSOLVER_SUCCESS)  {
         isFactored = 1;
      } else {
//        DBG_PRINT((1, "FACTORIZATION FAILED!\n"));
//         printf( "MultiSolve initial FACTORIZATION FAILED! retval = %d\n",retval);
         isFactored = 0;
      }

    }
      if (isFactored)  {
        retval =  Solve(ia, ja, nrhs, rhs_vals);
      } else {
         double rcond = -1.0;
         double *s,*work;
         int ispec = 1;
         int info;
         int *iwork,rank,nlvl,smlsiz,lwork,liwork,nosmlsiz;
         const char *name = "DGELSD";
         const char opts = ' ';
         s = new double[n];
         smlsiz = ilaenv_( ispec, name, opts, n, n, n, n, 6, 0);
         if( smlsiz < 0 ) {
             printf("ilaenv arg# %d illegal value \n",smlsiz );
             return retval;
         }
         nosmlsiz = n/(smlsiz+1);
/* 
**      increased size of nlvl by adding 1 due to 64bit failures
*/
         nlvl = (int)(log10((double)nosmlsiz)/log10(2.)) + 2;
         if( nlvl < 0 ) nlvl = 0;
         liwork = 3*n*nlvl + 11*n;
         iwork = new int[liwork];
         lwork = 12*n + 2*n*smlsiz + 8*n*nlvl + n*nrhs + (smlsiz+1)*(smlsiz+1);
         work = new double[lwork];
         dgelsd_( n, n, nrhs, atmp, n, rhs_vals, n, s, rcond, rank, work, 
                  lwork, iwork, info );
         
         delete [] work;
         delete [] s;
         delete [] iwork;
         if( info > 0 ) {
//            printf( "dgelsd %d elements failed to converge to zero \n",info );
         } else if( info < 0 ) {
//            printf( "dgelsd illegal arg #%d \n",info );
         } else {
            retval = SYMSOLVER_SUCCESS;
         }
      }
      delete [] atmp;
      return retval;  

  }


  double* LapackSolverInterface::GetValuesArrayPtr()
  {
    return a;
  }

  /** Initialize the local copy of the positions of the nonzero
      elements */
  ESymSolverStatus LapackSolverInterface::InitializeStructure(Index dim, Index nonzeros,
      const Index* ia, const Index* ja)
  {
    ESymSolverStatus retval = SYMSOLVER_SUCCESS;
    DBG_START_METH("LapackSolverInterface::InitializeStructure", dbg_verbosity);
    n = dim;
    nz = nonzeros;
    delete [] a;
    delete [] irn_;
    delete [] jcn_;
    a = new double[dim*dim];
    irn_ = new int[nz];
    jcn_ = new int[nz];
    for (Index i=0; i<nz; i++) {
      irn_[i] = ia[i];
      jcn_[i] = ja[i];
    }

    return retval;
  }


  ESymSolverStatus LapackSolverInterface::Factorization(const Index* ia, const Index* ja,
      bool check_NegEVals, Index numberOfNegEVals)
  {
    DBG_START_METH("LapackSolverInterface::Factorization", dbg_verbosity);
    ESymSolverStatus retval = SYMSOLVER_SUCCESS;
    int info,lwork;
    double *w, *work,*atmp;
    char jobz = 'N';
    char uplo = 'L';
    int *tmp_ipiv; 
    int i;

    
    ipiv = new int[n];
    tmp_ipiv = (int *)malloc( sizeof(int)*n );

    /* compute negative eigenvalues */

    negevals_ = 0;
    w = new double[n];
    lwork = 3*n;   // TODO get optimial value 
    work = new double[lwork];
    atmp = new double[n*n];
    for(i=0;i<n*n;i++) atmp[i] = a[i];
    dsyev_(jobz, uplo, n, atmp, n, w, work, lwork, info,  1, 1);
    if( info != 0 ) {
         return(SYMSOLVER_FATAL_ERROR);
    }
    for(i=0;i<n;i++){
   //     printf(" eigenvlaue #%d = %f \n",i,w[i] );
        if( w[i] < 0.0 ) negevals_++;
    }
    delete [] w;
    delete [] work;
    if (check_NegEVals && (numberOfNegEVals!=negevals_)) {
       return SYMSOLVER_WRONG_INERTIA;
    }


    dgetrf_( n, n, a, n, ipiv, info); 
    delete [] atmp;

    if( info > 0  ) {
        retval = SYMSOLVER_SINGULAR;
    }

    return retval;
  }

  ESymSolverStatus LapackSolverInterface::Solve(const Index* ia, const Index* ja, Index nrhs, double *b)
  {
    DBG_START_METH("LapackSolverInterface::Solve", dbg_verbosity);
    ESymSolverStatus retval = SYMSOLVER_SUCCESS;
    int info;
    char transpose = 'N';

    dgetrs_( transpose, n, nrhs, a, n, ipiv, b, n, info, 1); 
    if( info != 0 ) {
        if( info > 0 && info <= n  ) {
            retval = SYMSOLVER_SINGULAR;
        } else {
            retval = SYMSOLVER_FATAL_ERROR;
        }
    } else {
        retval = SYMSOLVER_SUCCESS;
    }


    return retval;
  }

  Index LapackSolverInterface::NumberOfNegEVals() const
  {
//    DBG_START_METH("LapackSolverInterface::NumberOfNegEVals", dbg_verbosity);
//printf("LapackSolverInterface::NumberOfNegEVals = %d\n",negevals_);
    DBG_ASSERT(negevals >= 0);
    return negevals_;
  }

  bool LapackSolverInterface::IncreaseQuality()
  {
    return false;
  }

}//end Ipopt namespace



