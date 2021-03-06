#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <inttypes.h>
#include <complex.h>

typedef double complex TCD;
typedef float  complex TCF;

#undef complex

#include "lapack-aux.h"

#define MACRO(B) do {B} while (0)
#define ERROR(CODE) MACRO(return CODE;)
#define REQUIRES(COND, CODE) MACRO(if(!(COND)) {ERROR(CODE);})

#define MIN(A,B) ((A)<(B)?(A):(B))
#define MAX(A,B) ((A)>(B)?(A):(B))

// #define DBGL

#ifdef DBGL
#define DEBUGMSG(M) printf("\nLAPACK "M"\n");
#else
#define DEBUGMSG(M)
#endif

#define OK return 0;

// #ifdef DBGL
// #define DEBUGMSG(M) printf("LAPACK Wrapper "M"\n: "); size_t t0 = time(NULL);
// #define OK MACRO(printf("%ld s\n",time(0)-t0); return 0;);
// #else
// #define DEBUGMSG(M)
// #define OK return 0;
// #endif


#define INFOMAT(M) printf("%dx%d %d:%d\n",M##r,M##c,M##Xr,M##Xc);

#define TRACEMAT(M) {int q; printf(" %d x %d: ",M##r,M##c); \
                     for(q=0;q<M##r*M##c;q++) printf("%.1f ",M##p[q]); printf("\n");}

#define CHECK(RES,CODE) MACRO(if(RES) return CODE;)

#define BAD_SIZE 2000
#define BAD_CODE 2001
#define MEM      2002
#define BAD_FILE 2003
#define SINGULAR 2004
#define NOCONVER 2005
#define NODEFPOS 2006
#define NOSPRTD  2007

////////////////////////////////////////////////////////////////////////////////
void asm_finit() {
#ifdef i386

//  asm("finit");

    static unsigned char buf[108];
    asm("FSAVE %0":"=m" (buf));

    #if FPUDEBUG
    if(buf[8]!=255 || buf[9]!=255) {  // print warning in red
        printf("%c[;31mWarning: FPU TAG = %x %x\%c[0m\n",0x1B,buf[8],buf[9],0x1B);
    }
    #endif

    #if NANDEBUG
    asm("FRSTOR %0":"=m" (buf));
    #endif

#endif
}

#if NANDEBUG

#define CHECKNANR(M,msg)                     \
{ int k;                                     \
for(k=0; k<(M##r * M##c); k++) {             \
    if(M##p[k] != M##p[k]) {                 \
        printf(msg);                         \
        TRACEMAT(M)                          \
        /*exit(1);*/                         \
    }                                        \
}                                            \
}

#define CHECKNANC(M,msg)                     \
{ int k;                                     \
for(k=0; k<(M##r * M##c); k++) {             \
    if(  M##p[k].r != M##p[k].r              \
      || M##p[k].i != M##p[k].i) {           \
        printf(msg);                         \
        /*exit(1);*/                         \
    }                                        \
}                                            \
}

#else
#define CHECKNANC(M,msg)
#define CHECKNANR(M,msg)
#endif


////////////////////////////////////////////////////////////////////////////////
//////////////////// real svd ///////////////////////////////////////////////////

int dgesvd_(char *jobu, char *jobvt, integer *m, integer *n,
	doublereal *a, integer *lda, doublereal *s, doublereal *u, integer *
	ldu, doublereal *vt, integer *ldvt, doublereal *work, integer *lwork,
	integer *info);

int svd_l_R(ODMAT(a),ODMAT(u), DVEC(s),ODMAT(v)) {
    integer m = ar;
    integer n = ac;
    integer q = MIN(m,n);
    REQUIRES(sn==q,BAD_SIZE);
    REQUIRES(up==NULL || (ur==m && (uc==m || uc==q)),BAD_SIZE);
    char* jobu  = "A";
    if (up==NULL) {
        jobu = "N";
    } else {
        if (uc==q) {
            jobu = "S";
        }
    }
    REQUIRES(vp==NULL || (vc==n && (vr==n || vr==q)),BAD_SIZE);
    char* jobvt  = "A";
    integer ldvt = n;
    if (vp==NULL) {
        jobvt = "N";
    } else {
        if (vr==q) {
            jobvt = "S";
            ldvt = q;
        }
    }
    DEBUGMSG("svd_l_R");
    integer lwork = -1;
    integer res;
    // ask for optimal lwork
    double ans;
    dgesvd_ (jobu,jobvt,
             &m,&n,ap,&m,
             sp,
             up,&m,
             vp,&ldvt,
             &ans, &lwork,
             &res);
    lwork = ceil(ans);
    double * work = (double*)malloc(lwork*sizeof(double));
    CHECK(!work,MEM);
    dgesvd_ (jobu,jobvt,
             &m,&n,ap,&m,
             sp,
             up,&m,
             vp,&ldvt,
             work, &lwork,
             &res);
    CHECK(res,res);
    free(work);
    OK
}

// (alternative version)

int dgesdd_(char *jobz, integer *m, integer *n, doublereal *
	a, integer *lda, doublereal *s, doublereal *u, integer *ldu,
	doublereal *vt, integer *ldvt, doublereal *work, integer *lwork,
	integer *iwork, integer *info);

int svd_l_Rdd(ODMAT(a),ODMAT(u), DVEC(s),ODMAT(v)) {
    integer m = ar;
    integer n = ac;
    integer q = MIN(m,n);
    REQUIRES(sn==q,BAD_SIZE);
    REQUIRES((up == NULL && vp == NULL)
             || (ur==m && vc==n
                &&   ((uc == q && vr == q)
                   || (uc == m && vc==n))),BAD_SIZE);
    char* jobz  = "A";
    integer ldvt = n;
    if (up==NULL) {
        jobz = "N";
    } else {
        if (uc==q && vr == q) {
            jobz = "S";
            ldvt = q;
        }
    }
    DEBUGMSG("svd_l_Rdd");
    integer* iwk = (integer*) malloc(8*q*sizeof(integer));
    CHECK(!iwk,MEM);
    integer lwk = -1;
    integer res;
    // ask for optimal lwk
    double ans;
    dgesdd_ (jobz,&m,&n,ap,&m,sp,up,&m,vp,&ldvt,&ans,&lwk,iwk,&res);
    lwk = ans;
    double * workv = (double*)malloc(lwk*sizeof(double));
    CHECK(!workv,MEM);
    dgesdd_ (jobz,&m,&n,ap,&m,sp,up,&m,vp,&ldvt,workv,&lwk,iwk,&res);
    CHECK(res,res);
    free(iwk);
    free(workv);
    OK
}

//////////////////// complex svd ////////////////////////////////////

int zgesvd_(char *jobu, char *jobvt, integer *m, integer *n,
    doublecomplex *a, integer *lda, doublereal *s, doublecomplex *u,
    integer *ldu, doublecomplex *vt, integer *ldvt, doublecomplex *work,
    integer *lwork, doublereal *rwork, integer *info);

int svd_l_C(OCMAT(a),OCMAT(u), DVEC(s),OCMAT(v)) {
    integer m = ar;
    integer n = ac;
    integer q = MIN(m,n);
    REQUIRES(sn==q,BAD_SIZE);
    REQUIRES(up==NULL || (ur==m && (uc==m || uc==q)),BAD_SIZE);
    char* jobu  = "A";
    if (up==NULL) {
        jobu = "N";
    } else {
        if (uc==q) {
            jobu = "S";
        }
    }
    REQUIRES(vp==NULL || (vc==n && (vr==n || vr==q)),BAD_SIZE);
    char* jobvt  = "A";
    integer ldvt = n;
    if (vp==NULL) {
        jobvt = "N";
    } else {
        if (vr==q) {
            jobvt = "S";
            ldvt = q;
        }
    }DEBUGMSG("svd_l_C");

    double *rwork = (double*) malloc(5*q*sizeof(double));
    CHECK(!rwork,MEM);
    integer lwork = -1;
    integer res;
    // ask for optimal lwork
    doublecomplex ans;
    zgesvd_ (jobu,jobvt,
             &m,&n,ap,&m,
             sp,
             up,&m,
             vp,&ldvt,
             &ans, &lwork,
             rwork,
             &res);
    lwork = ceil(ans.r);
    doublecomplex * work = (doublecomplex*)malloc(lwork*sizeof(doublecomplex));
    CHECK(!work,MEM);
    zgesvd_ (jobu,jobvt,
             &m,&n,ap,&m,
             sp,
             up,&m,
             vp,&ldvt,
             work, &lwork,
             rwork,
             &res);
    CHECK(res,res);
    free(work);
    free(rwork);
    OK
}

int zgesdd_ (char *jobz, integer *m, integer *n,
    doublecomplex *a, integer *lda, doublereal *s, doublecomplex *u,
    integer *ldu, doublecomplex *vt, integer *ldvt, doublecomplex *work,
    integer *lwork, doublereal *rwork, integer* iwork, integer *info);

int svd_l_Cdd(OCMAT(a),OCMAT(u), DVEC(s),OCMAT(v)) {
    integer m = ar;
    integer n = ac;
    integer q = MIN(m,n);
    REQUIRES(sn==q,BAD_SIZE);
    REQUIRES((up == NULL && vp == NULL)
             || (ur==m && vc==n
                &&   ((uc == q && vr == q)
                   || (uc == m && vc==n))),BAD_SIZE);
    char* jobz  = "A";
    integer ldvt = n;
    if (up==NULL) {
        jobz = "N";
    } else {
        if (uc==q && vr == q) {
            jobz = "S";
            ldvt = q;
        }
    }
    DEBUGMSG("svd_l_Cdd");
    integer* iwk = (integer*) malloc(8*q*sizeof(integer));
    CHECK(!iwk,MEM);
    int lrwk;
    if (0 && *jobz == 'N') {
        lrwk = 5*q; // does not work, crash at free below
    } else {
        lrwk = 5*q*q + 7*q;
    }
    double *rwk = (double*)malloc(lrwk*sizeof(double));;
    CHECK(!rwk,MEM);
    integer lwk = -1;
    integer res;
    // ask for optimal lwk
    doublecomplex ans;
    zgesdd_ (jobz,&m,&n,ap,&m,sp,up,&m,vp,&ldvt,&ans,&lwk,rwk,iwk,&res);
    lwk = ans.r;
    doublecomplex * workv = (doublecomplex*)malloc(lwk*sizeof(doublecomplex));
    CHECK(!workv,MEM);
    zgesdd_ (jobz,&m,&n,ap,&m,sp,up,&m,vp,&ldvt,workv,&lwk,rwk,iwk,&res);
    CHECK(res,res);
    free(workv);
    free(rwk);
    free(iwk);
    OK
}

//////////////////// general complex eigensystem ////////////

int zgeev_(char *jobvl, char *jobvr, integer *n,
	doublecomplex *a, integer *lda, doublecomplex *w, doublecomplex *vl,
	integer *ldvl, doublecomplex *vr, integer *ldvr, doublecomplex *work,
	integer *lwork, doublereal *rwork, integer *info);

int eig_l_C(OCMAT(a), OCMAT(u), CVEC(s),OCMAT(v)) {
    integer n = ar;
    REQUIRES(ac==n && sn==n, BAD_SIZE);
    REQUIRES(up==NULL || (ur==n && uc==n), BAD_SIZE);
    char jobvl = up==NULL?'N':'V';
    REQUIRES(vp==NULL || (vr==n && vc==n), BAD_SIZE);
    char jobvr = vp==NULL?'N':'V';
    DEBUGMSG("eig_l_C");
    double *rwork = (double*) malloc(2*n*sizeof(double));
    CHECK(!rwork,MEM);
    integer lwork = -1;
    integer res;
    // ask for optimal lwork
    doublecomplex ans;
    zgeev_  (&jobvl,&jobvr,
             &n,ap,&n,
             sp,
             up,&n,
             vp,&n,
             &ans, &lwork,
             rwork,
             &res);
    lwork = ceil(ans.r);
    doublecomplex * work = (doublecomplex*)malloc(lwork*sizeof(doublecomplex));
    CHECK(!work,MEM);
    zgeev_  (&jobvl,&jobvr,
             &n,ap,&n,
             sp,
             up,&n,
             vp,&n,
             work, &lwork,
             rwork,
             &res);
    CHECK(res,res);
    free(work);
    free(rwork);
    OK
}



//////////////////// general real eigensystem ////////////

int dgeev_(char *jobvl, char *jobvr, integer *n, doublereal *
	a, integer *lda, doublereal *wr, doublereal *wi, doublereal *vl,
	integer *ldvl, doublereal *vr, integer *ldvr, doublereal *work,
	integer *lwork, integer *info);

int eig_l_R(ODMAT(a),ODMAT(u), CVEC(s),ODMAT(v)) {
    integer n = ar;
    REQUIRES(ac==n && sn==n, BAD_SIZE);
    REQUIRES(up==NULL || (ur==n && uc==n), BAD_SIZE);
    char jobvl = up==NULL?'N':'V';
    REQUIRES(vp==NULL || (vr==n && vc==n), BAD_SIZE);
    char jobvr = vp==NULL?'N':'V';
    DEBUGMSG("eig_l_R");
    integer lwork = -1;
    integer res;
    // ask for optimal lwork
    double ans;
    dgeev_  (&jobvl,&jobvr,
             &n,ap,&n,
             (double*)sp, (double*)sp+n,
             up,&n,
             vp,&n,
             &ans, &lwork,
             &res);
    lwork = ceil(ans);
    double * work = (double*)malloc(lwork*sizeof(double));
    CHECK(!work,MEM);
    dgeev_  (&jobvl,&jobvr,
             &n,ap,&n,
             (double*)sp, (double*)sp+n,
             up,&n,
             vp,&n,
             work, &lwork,
             &res);
    CHECK(res,res);
    free(work);
    OK
}


//////////////////// symmetric real eigensystem ////////////

int dsyev_(char *jobz, char *uplo, integer *n, doublereal *a,
	integer *lda, doublereal *w, doublereal *work, integer *lwork,
	integer *info);

int eig_l_S(int wantV,DVEC(s),ODMAT(v)) {
    integer n = sn;
    REQUIRES(vr==n && vc==n, BAD_SIZE);
    char jobz = wantV?'V':'N';
    DEBUGMSG("eig_l_S");
    integer lwork = -1;
    char uplo = 'U';
    integer res;
    // ask for optimal lwork
    double ans;
    dsyev_  (&jobz,&uplo,
             &n,vp,&n,
             sp,
             &ans, &lwork,
             &res);
    lwork = ceil(ans);
    double * work = (double*)malloc(lwork*sizeof(double));
    CHECK(!work,MEM);
    dsyev_  (&jobz,&uplo,
             &n,vp,&n,
             sp,
             work, &lwork,
             &res);
    CHECK(res,res);
    free(work);
    OK
}

//////////////////// hermitian complex eigensystem ////////////

int zheev_(char *jobz, char *uplo, integer *n, doublecomplex
	*a, integer *lda, doublereal *w, doublecomplex *work, integer *lwork,
	doublereal *rwork, integer *info);

int eig_l_H(int wantV,DVEC(s),OCMAT(v)) {
    integer n = sn;
    REQUIRES(vr==n && vc==n, BAD_SIZE);
    char jobz = wantV?'V':'N';
    DEBUGMSG("eig_l_H");
    double *rwork = (double*) malloc((3*n-2)*sizeof(double));
    CHECK(!rwork,MEM);
    integer lwork = -1;
    char uplo = 'U';
    integer res;
    // ask for optimal lwork
    doublecomplex ans;
    zheev_  (&jobz,&uplo,
             &n,vp,&n,
             sp,
             &ans, &lwork,
             rwork,
             &res);
    lwork = ceil(ans.r);
    doublecomplex * work = (doublecomplex*)malloc(lwork*sizeof(doublecomplex));
    CHECK(!work,MEM);
    zheev_  (&jobz,&uplo,
             &n,vp,&n,
             sp,
             work, &lwork,
             rwork,
             &res);
    CHECK(res,res);
    free(work);
    free(rwork);
    OK
}

//////////////////// general real linear system ////////////

int dgesv_(integer *n, integer *nrhs, doublereal *a, integer
	*lda, integer *ipiv, doublereal *b, integer *ldb, integer *info);

int linearSolveR_l(ODMAT(a),ODMAT(b)) {
    integer n = ar;
    integer nhrs = bc;
    REQUIRES(n>=1 && ar==ac && ar==br,BAD_SIZE);
    DEBUGMSG("linearSolveR_l");
    integer * ipiv = (integer*)malloc(n*sizeof(integer));
    integer res;
    dgesv_  (&n,&nhrs,
             ap, &n,
             ipiv,
             bp, &n,
             &res);
    if(res>0) {
        return SINGULAR;
    }
    CHECK(res,res);
    free(ipiv);
    OK
}

//////////////////// general complex linear system ////////////

int zgesv_(integer *n, integer *nrhs, doublecomplex *a,
	integer *lda, integer *ipiv, doublecomplex *b, integer *ldb, integer *
	info);

int linearSolveC_l(OCMAT(a),OCMAT(b)) {
    integer n = ar;
    integer nhrs = bc;
    REQUIRES(n>=1 && ar==ac && ar==br,BAD_SIZE);
    DEBUGMSG("linearSolveC_l");
    integer * ipiv = (integer*)malloc(n*sizeof(integer));
    integer res;
    zgesv_  (&n,&nhrs,
             ap, &n,
             ipiv,
             bp, &n,
             &res);
    if(res>0) {
        return SINGULAR;
    }
    CHECK(res,res);
    free(ipiv);
    OK
}

//////// symmetric positive definite real linear system using Cholesky ////////////

int dpotrs_(char *uplo, integer *n, integer *nrhs,
	doublereal *a, integer *lda, doublereal *b, integer *ldb, integer *
	info);

int cholSolveR_l(KODMAT(a),ODMAT(b)) {
    integer n = ar;
    integer lda = aXc;
    integer nhrs = bc;
    REQUIRES(n>=1 && ar==ac && ar==br,BAD_SIZE);
    DEBUGMSG("cholSolveR_l");
    integer res;
    dpotrs_ ("U",
             &n,&nhrs,
             (double*)ap, &lda,
             bp, &n,
             &res);
    CHECK(res,res);
    OK
}

//////// Hermitian positive definite real linear system using Cholesky ////////////

int zpotrs_(char *uplo, integer *n, integer *nrhs,
	doublecomplex *a, integer *lda, doublecomplex *b, integer *ldb,
	integer *info);

int cholSolveC_l(KOCMAT(a),OCMAT(b)) {
    integer n = ar;
    integer lda = aXc;
    integer nhrs = bc;
    REQUIRES(n>=1 && ar==ac && ar==br,BAD_SIZE);
    DEBUGMSG("cholSolveC_l");
    integer res;
    zpotrs_  ("U",
             &n,&nhrs,
             (doublecomplex*)ap, &lda,
             bp, &n,
             &res);
    CHECK(res,res);
    OK
}

//////////////////// least squares real linear system ////////////

int dgels_(char *trans, integer *m, integer *n, integer *
	nrhs, doublereal *a, integer *lda, doublereal *b, integer *ldb,
	doublereal *work, integer *lwork, integer *info);

int linearSolveLSR_l(ODMAT(a),ODMAT(b)) {
    integer m = ar;
    integer n = ac;
    integer nrhs = bc;
    integer ldb = bXc;
    REQUIRES(m>=1 && n>=1 && br==MAX(m,n), BAD_SIZE);
    DEBUGMSG("linearSolveLSR_l");
    integer res;
    integer lwork = -1;
    double ans;
    dgels_  ("N",&m,&n,&nrhs,
             ap,&m,
             bp,&ldb,
             &ans,&lwork,
             &res);
    lwork = ceil(ans);
    double * work = (double*)malloc(lwork*sizeof(double));
    dgels_  ("N",&m,&n,&nrhs,
             ap,&m,
             bp,&ldb,
             work,&lwork,
             &res);
    if(res>0) {
        return SINGULAR;
    }
    CHECK(res,res);
    free(work);
    OK
}

//////////////////// least squares complex linear system ////////////

int zgels_(char *trans, integer *m, integer *n, integer *
	nrhs, doublecomplex *a, integer *lda, doublecomplex *b, integer *ldb,
	doublecomplex *work, integer *lwork, integer *info);

int linearSolveLSC_l(OCMAT(a),OCMAT(b)) {
    integer m = ar;
    integer n = ac;
    integer nrhs = bc;
    integer ldb = bXc;
    REQUIRES(m>=1 && n>=1 && br==MAX(m,n), BAD_SIZE);
    DEBUGMSG("linearSolveLSC_l");
    integer res;
    integer lwork = -1;
    doublecomplex ans;
    zgels_  ("N",&m,&n,&nrhs,
             ap,&m,
             bp,&ldb,
             &ans,&lwork,
             &res);
    lwork = ceil(ans.r);
    doublecomplex * work = (doublecomplex*)malloc(lwork*sizeof(doublecomplex));
    zgels_  ("N",&m,&n,&nrhs,
             ap,&m,
             bp,&ldb,
             work,&lwork,
             &res);
    if(res>0) {
        return SINGULAR;
    }
    CHECK(res,res);
    free(work);
    OK
}

//////////////////// least squares real linear system using SVD ////////////

int dgelss_(integer *m, integer *n, integer *nrhs,
	doublereal *a, integer *lda, doublereal *b, integer *ldb, doublereal *
	s, doublereal *rcond, integer *rank, doublereal *work, integer *lwork,
	integer *info);

int linearSolveSVDR_l(double rcond,ODMAT(a),ODMAT(b)) {
    integer m = ar;
    integer n = ac;
    integer nrhs = bc;
    integer ldb = bXc;
    REQUIRES(m>=1 && n>=1 && br==MAX(m,n), BAD_SIZE);
    DEBUGMSG("linearSolveSVDR_l");
    double*S = (double*)malloc(MIN(m,n)*sizeof(double));
    integer res;
    integer lwork = -1;
    integer rank;
    double ans;
    dgelss_  (&m,&n,&nrhs,
             ap,&m,
             bp,&ldb,
             S,
             &rcond,&rank,
             &ans,&lwork,
             &res);
    lwork = ceil(ans);
    double * work = (double*)malloc(lwork*sizeof(double));
    dgelss_  (&m,&n,&nrhs,
             ap,&m,
             bp,&ldb,
             S,
             &rcond,&rank,
             work,&lwork,
             &res);
    if(res>0) {
        return NOCONVER;
    }
    CHECK(res,res);
    free(work);
    free(S);
    OK
}

//////////////////// least squares complex linear system using SVD ////////////

int zgelss_(integer *m, integer *n, integer *nhrs,
    doublecomplex *a, integer *lda, doublecomplex *b, integer *ldb, doublereal *s,
    doublereal *rcond, integer* rank,
    doublecomplex *work, integer* lwork, doublereal* rwork,
    integer *info);

int linearSolveSVDC_l(double rcond, OCMAT(a),OCMAT(b)) {
    integer m = ar;
    integer n = ac;
    integer nrhs = bc;
    integer ldb = bXc;
    REQUIRES(m>=1 && n>=1 && br==MAX(m,n), BAD_SIZE);
    DEBUGMSG("linearSolveSVDC_l");
    double*S = (double*)malloc(MIN(m,n)*sizeof(double));
    double*RWORK = (double*)malloc(5*MIN(m,n)*sizeof(double));
    integer res;
    integer lwork = -1;
    integer rank;
    doublecomplex ans;
    zgelss_  (&m,&n,&nrhs,
             ap,&m,
             bp,&ldb,
             S,
             &rcond,&rank,
             &ans,&lwork,
             RWORK,
             &res);
    lwork = ceil(ans.r);
    doublecomplex * work = (doublecomplex*)malloc(lwork*sizeof(doublecomplex));
    zgelss_  (&m,&n,&nrhs,
             ap,&m,
             bp,&ldb,
             S,
             &rcond,&rank,
             work,&lwork,
             RWORK,
             &res);
    if(res>0) {
        return NOCONVER;
    }
    CHECK(res,res);
    free(work);
    free(RWORK);
    free(S);
    OK
}

//////////////////// Cholesky factorization /////////////////////////

int zpotrf_(char *uplo, integer *n, doublecomplex *a, integer *lda, integer *info);

int chol_l_H(OCMAT(l)) {
    integer n = lr;
    REQUIRES(n>=1 && lc == n,BAD_SIZE);
    DEBUGMSG("chol_l_H");
    char uplo = 'U';
    integer res;
    zpotrf_ (&uplo,&n,lp,&n,&res);
    CHECK(res>0,NODEFPOS);
    CHECK(res,res);
    doublecomplex zero = {0.,0.};
    int r,c;
    for (r=0; r<lr; r++) {
        for(c=0; c<r; c++) {
            AT(l,r,c) = zero;
        }
    }
    OK
}


int dpotrf_(char *uplo, integer *n, doublereal *a, integer * lda, integer *info);

int chol_l_S(ODMAT(l)) {
    integer n = lr;
    REQUIRES(n>=1 && lc == n,BAD_SIZE);
    DEBUGMSG("chol_l_S");
    char uplo = 'U';
    integer res;
    dpotrf_ (&uplo,&n,lp,&n,&res);
    CHECK(res>0,NODEFPOS);
    CHECK(res,res);
    int r,c;
    for (r=0; r<lr; r++) {
        for(c=0; c<r; c++) {
            AT(l,r,c) = 0.;
        }
    }
    OK
}

//////////////////// QR factorization /////////////////////////

int dgeqr2_(integer *m, integer *n, doublereal *a, integer *
	lda, doublereal *tau, doublereal *work, integer *info);

int qr_l_R(DVEC(tau), ODMAT(r)) {
    integer m = rr;
    integer n = rc;
    integer mn = MIN(m,n);
    REQUIRES(m>=1 && n >=1 && taun == mn, BAD_SIZE);
    DEBUGMSG("qr_l_R");
    double *WORK = (double*)malloc(n*sizeof(double));
    CHECK(!WORK,MEM);
    integer res;
    dgeqr2_ (&m,&n,rp,&m,taup,WORK,&res);
    CHECK(res,res);
    free(WORK);
    OK
}

int zgeqr2_(integer *m, integer *n, doublecomplex *a,
	integer *lda, doublecomplex *tau, doublecomplex *work, integer *info);

int qr_l_C(CVEC(tau), OCMAT(r)) {
    integer m = rr;
    integer n = rc;
    integer mn = MIN(m,n);
    REQUIRES(m>=1 && n >=1 && taun == mn, BAD_SIZE);
    DEBUGMSG("qr_l_C");
    doublecomplex *WORK = (doublecomplex*)malloc(n*sizeof(doublecomplex));
    CHECK(!WORK,MEM);
    integer res;
    zgeqr2_ (&m,&n,rp,&m,taup,WORK,&res);
    CHECK(res,res);
    free(WORK);
    OK
}

int dorgqr_(integer *m, integer *n, integer *k, doublereal *
	a, integer *lda, doublereal *tau, doublereal *work, integer *lwork,
	integer *info);

int c_dorgqr(KDVEC(tau), ODMAT(r)) {
    integer m = rr;
    integer n = MIN(rc,rr);
    integer k = taun;
    DEBUGMSG("c_dorgqr");
    integer lwork = 8*n; // FIXME
    double *WORK = (double*)malloc(lwork*sizeof(double));
    CHECK(!WORK,MEM);
    integer res;
    dorgqr_ (&m,&n,&k,rp,&m,(double*)taup,WORK,&lwork,&res);
    CHECK(res,res);
    free(WORK);
    OK
}

int zungqr_(integer *m, integer *n, integer *k,
	doublecomplex *a, integer *lda, doublecomplex *tau, doublecomplex *
	work, integer *lwork, integer *info);

int c_zungqr(KCVEC(tau), OCMAT(r)) {
    integer m = rr;
    integer n = MIN(rc,rr);
    integer k = taun;
    DEBUGMSG("z_ungqr");
    integer lwork = 8*n; // FIXME
    doublecomplex *WORK = (doublecomplex*)malloc(lwork*sizeof(doublecomplex));
    CHECK(!WORK,MEM);
    integer res;
    zungqr_ (&m,&n,&k,rp,&m,(doublecomplex*)taup,WORK,&lwork,&res);
    CHECK(res,res);
    free(WORK);
    OK
}


//////////////////// Hessenberg factorization /////////////////////////

int dgehrd_(integer *n, integer *ilo, integer *ihi,
	doublereal *a, integer *lda, doublereal *tau, doublereal *work,
	integer *lwork, integer *info);

int hess_l_R(DVEC(tau), ODMAT(r)) {
    integer m = rr;
    integer n = rc;
    integer mn = MIN(m,n);
    REQUIRES(m>=1 && n == m && taun == mn-1, BAD_SIZE);
    DEBUGMSG("hess_l_R");
    integer lwork = 5*n; // FIXME
    double *WORK = (double*)malloc(lwork*sizeof(double));
    CHECK(!WORK,MEM);
    integer res;
    integer one = 1;
    dgehrd_ (&n,&one,&n,rp,&n,taup,WORK,&lwork,&res);
    CHECK(res,res);
    free(WORK);
    OK
}


int zgehrd_(integer *n, integer *ilo, integer *ihi,
	doublecomplex *a, integer *lda, doublecomplex *tau, doublecomplex *
	work, integer *lwork, integer *info);

int hess_l_C(CVEC(tau), OCMAT(r)) {
    integer m = rr;
    integer n = rc;
    integer mn = MIN(m,n);
    REQUIRES(m>=1 && n == m && taun == mn-1, BAD_SIZE);
    DEBUGMSG("hess_l_C");
    integer lwork = 5*n; // FIXME
    doublecomplex *WORK = (doublecomplex*)malloc(lwork*sizeof(doublecomplex));
    CHECK(!WORK,MEM);
    integer res;
    integer one = 1;
    zgehrd_ (&n,&one,&n,rp,&n,taup,WORK,&lwork,&res);
    CHECK(res,res);
    free(WORK);
    OK
}

//////////////////// Schur factorization /////////////////////////

int dgees_(char *jobvs, char *sort, L_fp select, integer *n,
	doublereal *a, integer *lda, integer *sdim, doublereal *wr,
	doublereal *wi, doublereal *vs, integer *ldvs, doublereal *work,
	integer *lwork, logical *bwork, integer *info);

int schur_l_R(ODMAT(u), ODMAT(s)) {
    integer m = sr;
    integer n = sc;
    REQUIRES(m>=1 && n==m && ur==n && uc==n, BAD_SIZE);
    DEBUGMSG("schur_l_R");
    integer lwork = 6*n; // FIXME
    double *WORK = (double*)malloc(lwork*sizeof(double));
    double *WR = (double*)malloc(n*sizeof(double));
    double *WI = (double*)malloc(n*sizeof(double));
    // WR and WI not really required in this call
    logical *BWORK = (logical*)malloc(n*sizeof(logical));
    integer res;
    integer sdim;
    dgees_ ("V","N",NULL,&n,sp,&n,&sdim,WR,WI,up,&n,WORK,&lwork,BWORK,&res);
    if(res>0) {
        return NOCONVER;
    }
    CHECK(res,res);
    free(WR);
    free(WI);
    free(BWORK);
    free(WORK);
    OK
}


int zgees_(char *jobvs, char *sort, L_fp select, integer *n,
	doublecomplex *a, integer *lda, integer *sdim, doublecomplex *w,
	doublecomplex *vs, integer *ldvs, doublecomplex *work, integer *lwork,
	doublereal *rwork, logical *bwork, integer *info);

int schur_l_C(OCMAT(u), OCMAT(s)) {
    integer m = sr;
    integer n = sc;
    REQUIRES(m>=1 && n==m && ur==n && uc==n, BAD_SIZE);
    DEBUGMSG("schur_l_C");
    integer lwork = 6*n; // FIXME
    doublecomplex *WORK = (doublecomplex*)malloc(lwork*sizeof(doublecomplex));
    doublecomplex *W = (doublecomplex*)malloc(n*sizeof(doublecomplex));
    // W not really required in this call
    logical *BWORK = (logical*)malloc(n*sizeof(logical));
    double *RWORK = (double*)malloc(n*sizeof(double));
    integer res;
    integer sdim;
    zgees_ ("V","N",NULL,&n,sp,&n,&sdim,W,
                            up,&n,
                            WORK,&lwork,RWORK,BWORK,&res);
    if(res>0) {
        return NOCONVER;
    }
    CHECK(res,res);
    free(W);
    free(BWORK);
    free(WORK);
    OK
}

//////////////////// LU factorization /////////////////////////

int dgetrf_(integer *m, integer *n, doublereal *a, integer *
	lda, integer *ipiv, integer *info);

int lu_l_R(DVEC(ipiv), ODMAT(r)) {
    integer m = rr;
    integer n = rc;
    integer mn = MIN(m,n);
    REQUIRES(m>=1 && n >=1 && ipivn == mn, BAD_SIZE);
    DEBUGMSG("lu_l_R");
    integer* auxipiv = (integer*)malloc(mn*sizeof(integer));
    integer res;
    dgetrf_ (&m,&n,rp,&m,auxipiv,&res);
    if(res>0) {
        res = 0; // FIXME
    }
    CHECK(res,res);
    int k;
    for (k=0; k<mn; k++) {
        ipivp[k] = auxipiv[k];
    }
    free(auxipiv);
    OK
}


int zgetrf_(integer *m, integer *n, doublecomplex *a,
	integer *lda, integer *ipiv, integer *info);

int lu_l_C(DVEC(ipiv), OCMAT(r)) {
    integer m = rr;
    integer n = rc;
    integer mn = MIN(m,n);
    REQUIRES(m>=1 && n >=1 && ipivn == mn, BAD_SIZE);
    DEBUGMSG("lu_l_C");
    integer* auxipiv = (integer*)malloc(mn*sizeof(integer));
    integer res;
    zgetrf_ (&m,&n,rp,&m,auxipiv,&res);
    if(res>0) {
        res = 0; // FIXME
    }
    CHECK(res,res);
    int k;
    for (k=0; k<mn; k++) {
        ipivp[k] = auxipiv[k];
    }
    free(auxipiv);
    OK
}


//////////////////// LU substitution /////////////////////////

int dgetrs_(char *trans, integer *n, integer *nrhs,
	doublereal *a, integer *lda, integer *ipiv, doublereal *b, integer *
	ldb, integer *info);

int luS_l_R(KODMAT(a), KDVEC(ipiv), ODMAT(b)) {
  integer m = ar;
  integer n = ac;
  integer lda = aXc;
  integer mrhs = br;
  integer nrhs = bc;

  REQUIRES(m==n && m==mrhs && m==ipivn,BAD_SIZE);
  integer* auxipiv = (integer*)malloc(n*sizeof(integer));
  int k;
  for (k=0; k<n; k++) {
    auxipiv[k] = (integer)ipivp[k];
  }
  integer res;
  dgetrs_ ("N",&n,&nrhs,(/*no const (!?)*/ double*)ap,&lda,auxipiv,bp,&mrhs,&res);
  CHECK(res,res);
  free(auxipiv);
  OK
}


int zgetrs_(char *trans, integer *n, integer *nrhs,
	doublecomplex *a, integer *lda, integer *ipiv, doublecomplex *b,
	integer *ldb, integer *info);

int luS_l_C(KOCMAT(a), KDVEC(ipiv), OCMAT(b)) {
    integer m = ar;
    integer n = ac;
    integer lda = aXc;
    integer mrhs = br;
    integer nrhs = bc;

    REQUIRES(m==n && m==mrhs && m==ipivn,BAD_SIZE);
    integer* auxipiv = (integer*)malloc(n*sizeof(integer));
    int k;
    for (k=0; k<n; k++) {
        auxipiv[k] = (integer)ipivp[k];
    }
    integer res;
    zgetrs_ ("N",&n,&nrhs,(doublecomplex*)ap,&lda,auxipiv,bp,&mrhs,&res);
    CHECK(res,res);
    free(auxipiv);
    OK
}


//////////////////// LDL factorization /////////////////////////

int dsytrf_(char *uplo, integer *n, doublereal *a, integer *lda, integer *ipiv,
            doublereal *work, integer *lwork, integer *info);

int ldl_R(DVEC(ipiv), ODMAT(r)) {
    integer n = rr;
    REQUIRES(n>=1 && rc==n && ipivn == n, BAD_SIZE);
    DEBUGMSG("ldl_R");
    integer* auxipiv = (integer*)malloc(n*sizeof(integer));
    integer res;
    integer lda = rXc;
    integer lwork = -1;
    doublereal ans;
    dsytrf_ ("L",&n,rp,&lda,auxipiv,&ans,&lwork,&res);
    lwork = ceil(ans);
    doublereal* work = (doublereal*)malloc(lwork*sizeof(doublereal));
    dsytrf_ ("L",&n,rp,&lda,auxipiv,work,&lwork,&res);
    CHECK(res,res);
    int k;
    for (k=0; k<n; k++) {
        ipivp[k] = auxipiv[k];
    }
    free(auxipiv);
    free(work);
    OK
}


int zhetrf_(char *uplo, integer *n, doublecomplex *a, integer *lda, integer *ipiv,
            doublecomplex *work, integer *lwork, integer *info);

int ldl_C(DVEC(ipiv), OCMAT(r)) {
    integer n = rr;
    REQUIRES(n>=1 && rc==n && ipivn == n, BAD_SIZE);
    DEBUGMSG("ldl_R");
    integer* auxipiv = (integer*)malloc(n*sizeof(integer));
    integer res;
    integer lda = rXc;
    integer lwork = -1;
    doublecomplex ans;
    zhetrf_ ("L",&n,rp,&lda,auxipiv,&ans,&lwork,&res);
    lwork = ceil(ans.r);
    doublecomplex* work = (doublecomplex*)malloc(lwork*sizeof(doublecomplex));
    zhetrf_ ("L",&n,rp,&lda,auxipiv,work,&lwork,&res);
    CHECK(res,res);
    int k;
    for (k=0; k<n; k++) {
        ipivp[k] = auxipiv[k];
    }
    free(auxipiv);
    free(work);
    OK

}

//////////////////// LDL solve /////////////////////////

int dsytrs_(char *uplo, integer *n, integer *nrhs, doublereal *a, integer *lda,
            integer *ipiv, doublereal *b, integer *ldb, integer *info);

int ldl_S_R(KODMAT(a), KDVEC(ipiv), ODMAT(b)) {
  integer m = ar;
  integer n = ac;
  integer lda = aXc;
  integer mrhs = br;
  integer nrhs = bc;

  REQUIRES(m==n && m==mrhs && m==ipivn,BAD_SIZE);
  integer* auxipiv = (integer*)malloc(n*sizeof(integer));
  int k;
  for (k=0; k<n; k++) {
    auxipiv[k] = (integer)ipivp[k];
  }
  integer res;
  dsytrs_ ("L",&n,&nrhs,(/*no const (!?)*/ double*)ap,&lda,auxipiv,bp,&mrhs,&res);
  CHECK(res,res);
  free(auxipiv);
  OK
}


int zhetrs_(char *uplo, integer *n, integer *nrhs, doublecomplex *a, integer *lda,
            integer *ipiv, doublecomplex *b, integer *ldb, integer *info);

int ldl_S_C(KOCMAT(a), KDVEC(ipiv), OCMAT(b)) {
    integer m = ar;
    integer n = ac;
    integer lda = aXc;
    integer mrhs = br;
    integer nrhs = bc;

    REQUIRES(m==n && m==mrhs && m==ipivn,BAD_SIZE);
    integer* auxipiv = (integer*)malloc(n*sizeof(integer));
    int k;
    for (k=0; k<n; k++) {
        auxipiv[k] = (integer)ipivp[k];
    }
    integer res;
    zhetrs_ ("L",&n,&nrhs,(doublecomplex*)ap,&lda,auxipiv,bp,&mrhs,&res);
    CHECK(res,res);
    free(auxipiv);
    OK
}


//////////////////// Matrix Product /////////////////////////

void dgemm_(char *, char *, integer *, integer *, integer *,
           double *, const double *, integer *, const double *,
           integer *, double *, double *, integer *);

int multiplyR(int ta, int tb, KODMAT(a),KODMAT(b),ODMAT(r)) {
    DEBUGMSG("dgemm_");
    CHECKNANR(a,"NaN multR Input\n")
    CHECKNANR(b,"NaN multR Input\n")
    integer m = ta?ac:ar;
    integer n = tb?br:bc;
    integer k = ta?ar:ac;
    integer lda = aXc;
    integer ldb = bXc;
    integer ldc = rXc;
    double alpha = 1;
    double beta = 0;
    dgemm_(ta?"T":"N",tb?"T":"N",&m,&n,&k,&alpha,ap,&lda,bp,&ldb,&beta,rp,&ldc);
    CHECKNANR(r,"NaN multR Output\n")
    OK
}

void zgemm_(char *, char *, integer *, integer *, integer *,
           doublecomplex *, const doublecomplex *, integer *, const doublecomplex *,
           integer *, doublecomplex *, doublecomplex *, integer *);

int multiplyC(int ta, int tb, KOCMAT(a),KOCMAT(b),OCMAT(r)) {
    DEBUGMSG("zgemm_");
    CHECKNANC(a,"NaN multC Input\n")
    CHECKNANC(b,"NaN multC Input\n")
    integer m = ta?ac:ar;
    integer n = tb?br:bc;
    integer k = ta?ar:ac;
    integer lda = aXc;
    integer ldb = bXc;
    integer ldc = rXc;
    doublecomplex alpha = {1,0};
    doublecomplex beta = {0,0};
    zgemm_(ta?"T":"N",tb?"T":"N",&m,&n,&k,&alpha,
           ap,&lda,
           bp,&ldb,&beta,
           rp,&ldc);
    CHECKNANC(r,"NaN multC Output\n")
    OK
}

void sgemm_(char *, char *, integer *, integer *, integer *,
            float *, const float *, integer *, const float *,
           integer *, float *, float *, integer *);

int multiplyF(int ta, int tb, KOFMAT(a),KOFMAT(b),OFMAT(r)) {
    DEBUGMSG("sgemm_");
    integer m = ta?ac:ar;
    integer n = tb?br:bc;
    integer k = ta?ar:ac;
    integer lda = aXc;
    integer ldb = bXc;
    integer ldc = rXc;
    float alpha = 1;
    float beta = 0;
    sgemm_(ta?"T":"N",tb?"T":"N",&m,&n,&k,&alpha,ap,&lda,bp,&ldb,&beta,rp,&ldc);
    OK
}

void cgemm_(char *, char *, integer *, integer *, integer *,
           complex *, const complex *, integer *, const complex *,
           integer *, complex *, complex *, integer *);

int multiplyQ(int ta, int tb, KOQMAT(a),KOQMAT(b),OQMAT(r)) {
    DEBUGMSG("cgemm_");
    integer m = ta?ac:ar;
    integer n = tb?br:bc;
    integer k = ta?ar:ac;
    integer lda = aXc;
    integer ldb = bXc;
    integer ldc = rXc;
    complex alpha = {1,0};
    complex beta = {0,0};
    cgemm_(ta?"T":"N",tb?"T":"N",&m,&n,&k,&alpha,
           ap,&lda,
           bp,&ldb,&beta,
           rp,&ldc);
    OK
}


#define MULT_IMP_VER(OP)        \
    { TRAV(r,i,j) {             \
        int k;                  \
        AT(r,i,j) = 0;          \
        for (k=0;k<ac;k++) {    \
            OP                  \
        }                       \
      }                         \
    }

#define MULT_IMP(M) {                                                                \
    if (m==1) {                                                                      \
        MULT_IMP_VER( AT(r,i,j) += AT(a,i,k) * AT(b,k,j); )                          \
    } else {                                                                         \
        MULT_IMP_VER( AT(r,i,j) = M(AT(r,i,j) + M(AT(a,i,k) * AT(b,k,j), m) , m) ; ) \
    } OK }

int multiplyI(int     m, KOIMAT(a), KOIMAT(b), OIMAT(r)) MULT_IMP(mod)
int multiplyL(int64_t m, KOLMAT(a), KOLMAT(b), OLMAT(r)) MULT_IMP(mod_l)

/////////////////////////////// inplace row ops ////////////////////////////////

#define AXPY_IMP {                    \
    int j;                            \
    for(j=j1; j<=j2; j++) {           \
        AT(r,i2,j) += a*AT(r,i1,j);   \
    } OK }

#define AXPY_MOD_IMP(M) {                                      \
    int j;                                                     \
    for(j=j1; j<=j2; j++) {                                    \
        AT(r,i2,j) = M(AT(r,i2,j) + M(a*AT(r,i1,j), m) , m);   \
    } OK }


#define SCAL_IMP {                    \
    int i,j;                          \
    for(i=i1; i<=i2; i++) {           \
        for(j=j1; j<=j2; j++) {       \
            AT(r,i,j) = a*AT(r,i,j);  \
            }                         \
    } OK }

#define SCAL_MOD_IMP(M) {                   \
    int i,j;                                \
    for(i=i1; i<=i2; i++) {                 \
        for(j=j1; j<=j2; j++) {             \
            AT(r,i,j) = M(a*AT(r,i,j) , m); \
            }                               \
    } OK }


#define SWAP_IMP(T)   {               \
    T aux;                            \
    int k;                            \
    if (i1 != i2) {                   \
        for (k=j1; k<=j2; k++) {      \
            aux = AT(r,i1,k);         \
            AT(r,i1,k) = AT(r,i2,k);  \
            AT(r,i2,k) = aux;         \
        }                             \
    } OK }


#define ROWOP_IMP(T) {                \
    T a = *pa;                        \
    switch(code) {                    \
        case 0:  AXPY_IMP             \
        case 1:  SCAL_IMP             \
        case 2:  SWAP_IMP(T)          \
        default: ERROR(BAD_CODE);     \
    }                                 \
}

#define ROWOP_MOD_IMP(T,M) {          \
    T a = *pa;                        \
    switch(code) {                    \
        case 0:  AXPY_MOD_IMP(M)      \
        case 1:  SCAL_MOD_IMP(M)      \
        case 2:  SWAP_IMP(T)          \
        default: ERROR(BAD_CODE);     \
    }                                 \
}


#define ROWOP(T) int rowop_##T(int code, T* pa, int i1, int i2, int j1, int j2, MATG(T,r)) ROWOP_IMP(T)

#define ROWOP_MOD(T,M) int rowop_mod_##T(T m, int code, T* pa, int i1, int i2, int j1, int j2, MATG(T,r)) ROWOP_MOD_IMP(T,M)

ROWOP(double)
ROWOP(float)
ROWOP(TCD)
ROWOP(TCF)
ROWOP(int32_t)
ROWOP(int64_t)
ROWOP_MOD(int32_t,mod)
ROWOP_MOD(int64_t,mod_l)

/////////////////////////////// inplace GEMM ////////////////////////////////

#define GEMM(T) int gemm_##T(VECG(T,c),MATG(T,a),MATG(T,b),MATG(T,r)) {  \
    T a = cp[0], b = cp[1];             \
    T t;                                \
    int k;                              \
    { TRAV(r,i,j) {                     \
      t = 0;                            \
      for(k=0; k<ac; k++) {             \
          t += AT(a,i,k) * AT(b,k,j);   \
      }                                 \
      AT(r,i,j) = b*AT(r,i,j) + a*t;    \
      }                                 \
    } OK }


GEMM(double)
GEMM(float)
GEMM(TCD)
GEMM(TCF)
GEMM(int32_t)
GEMM(int64_t)

#define GEMM_MOD(T,M) int gemm_mod_##T(T m, VECG(T,c),MATG(T,a),MATG(T,b),MATG(T,r)) {  \
    T a = cp[0], b = cp[1];                     \
    int k;                                      \
    T t;                                        \
    { TRAV(r,i,j) {                             \
      t = 0;                                    \
      for(k=0; k<ac; k++) {                     \
          t = M(t+M(AT(a,i,k) * AT(b,k,j)));    \
      }                                         \
      AT(r,i,j) = M(M(b*AT(r,i,j)) + M(a*t));   \
      }                                         \
    } OK }


#define MOD32(X) mod(X,m)
#define MOD64(X) mod_l(X,m)

GEMM_MOD(int32_t,MOD32)
GEMM_MOD(int64_t,MOD64)

////////////////// sparse matrix-product ///////////////////////////////////////


int smXv(KDVEC(vals),KIVEC(cols),KIVEC(rows),KDVEC(x),DVEC(r)) {
    int r, c;
    for (r = 0; r < rowsn - 1; r++) {
        rp[r] = 0;
        for (c = rowsp[r]; c < rowsp[r+1]; c++) {
            rp[r] += valsp[c-1] * xp[colsp[c-1]-1];
        }
    }
    OK
}

int smTXv(KDVEC(vals),KIVEC(cols),KIVEC(rows),KDVEC(x),DVEC(r)) {
    int r,c;
    for (c = 0; c < rn; c++) {
        rp[c] = 0;
    }
    for (r = 0; r < rowsn - 1; r++) {
        for (c = rowsp[r]; c < rowsp[r+1]; c++) {
            rp[colsp[c-1]-1] += valsp[c-1] * xp[r];
        }
    }
    OK
}


//////////////////////// extract /////////////////////////////////

#define EXTRACT_IMP {                      \
    int i,j,si,sj,ni,nj;                   \
    ni = modei ? in : ip[1]-ip[0]+1;       \
    nj = modej ? jn : jp[1]-jp[0]+1;       \
                                           \
    for (i=0; i<ni; i++) {                 \
        si = modei ? ip[i] : i+ip[0];      \
                                           \
        for (j=0; j<nj; j++) {             \
            sj = modej ? jp[j] : j+jp[0];  \
                                           \
            AT(r,i,j) = AT(m,si,sj);       \
        }                                  \
    } OK }

#define EXTRACT(T) int extract##T(int modei, int modej, KIVEC(i), KIVEC(j), KO##T##MAT(m), O##T##MAT(r)) EXTRACT_IMP

EXTRACT(D)
EXTRACT(F)
EXTRACT(C)
EXTRACT(Q)
EXTRACT(I)
EXTRACT(L)

//////////////////////// setRect /////////////////////////////////

#define SETRECT(T)                                            \
int setRect##T(int i, int j, KO##T##MAT(m), O##T##MAT(r)) {   \
    { TRAV(m,a,b) {                                           \
        int x = a+i, y = b+j;                                 \
        if(x>=0 && x<rr && y>=0 && y<rc) {                    \
            AT(r,x,y) = AT(m,a,b);                            \
        }                                                     \
      }                                                       \
    } OK }

SETRECT(D)
SETRECT(F)
SETRECT(C)
SETRECT(Q)
SETRECT(I)
SETRECT(L)

//////////////////////// remap /////////////////////////////////

#define REMAP_IMP                                               \
    REQUIRES(ir==jr && ic==jc && ir==rr && ic==rc ,BAD_SIZE);   \
    { TRAV(r,a,b) { AT(r,a,b) = AT(m,AT(i,a,b),AT(j,a,b)); }    \
    }                                                           \
    OK

int remapD(KOIMAT(i), KOIMAT(j), KODMAT(m), ODMAT(r)) {
    REMAP_IMP
}

int remapF(KOIMAT(i), KOIMAT(j), KOFMAT(m), OFMAT(r)) {
    REMAP_IMP
}

int remapI(KOIMAT(i), KOIMAT(j), KOIMAT(m), OIMAT(r)) {
    REMAP_IMP
}

int remapL(KOIMAT(i), KOIMAT(j), KOLMAT(m), OLMAT(r)) {
    REMAP_IMP
}

int remapC(KOIMAT(i), KOIMAT(j), KOCMAT(m), OCMAT(r)) {
    REMAP_IMP
}

int remapQ(KOIMAT(i), KOIMAT(j), KOQMAT(m), OQMAT(r)) {
    REMAP_IMP
}

////////////////////////////////////////////////////////////////////////////////

int saveMatrix(char * file, char * format, KODMAT(a)){
    FILE * fp;
    fp = fopen (file, "w");
    int r, c;
    for (r=0;r<ar; r++) {
        for (c=0; c<ac; c++) {
            fprintf(fp,format,AT(a,r,c));
            if (c<ac-1) {
                fprintf(fp," ");
            } else {
                fprintf(fp,"\n");
            }
        }
    }
    fclose(fp);
    OK
}

