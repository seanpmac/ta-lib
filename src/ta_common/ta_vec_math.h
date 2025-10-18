/*
** ta_vec_math.h - Shared SIMD-friendly helpers for TA-Lib vector math
*/

#ifndef TA_VEC_MATH_H
#define TA_VEC_MATH_H

#include <stddef.h>
#include <math.h>

static inline double TA_FmaScalar(double lhs, double rhs, double addend)
{
#if defined(_MSC_VER) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L)
   return fma(lhs, rhs, addend);
#else
   return (lhs * rhs) + addend;
#endif
}

/* Ensure count is non-negative and bail fast when empty */
static inline void TA_VEC_AddD_Range(const double *lhs, const double *rhs,
                                     double *dest, int start, int length)
{
   if( length <= 0 || start < 0 || !lhs || !rhs || !dest )
      return;

   const int end = start + length;
   int i;
#if defined(_OPENMP)
   #pragma omp simd
#endif
   for( i = start; i < end; ++i )
   {
      dest[i] = lhs[i] + rhs[i];
   }
}

static inline void TA_VEC_AddD(const double *lhs, const double *rhs,
                               double *dest, int count)
{
   if( count <= 0 || !lhs || !rhs || !dest )
      return;

   TA_VEC_AddD_Range(lhs, rhs, dest, 0, count);
}

static inline void TA_VEC_SubD_Range(const double *lhs, const double *rhs,
                                     double *dest, int start, int length)
{
   if( length <= 0 || start < 0 || !lhs || !rhs || !dest )
      return;

   const int end = start + length;
   int i;
#if defined(_OPENMP)
   #pragma omp simd
#endif
   for( i = start; i < end; ++i )
   {
      dest[i] = lhs[i] - rhs[i];
   }
}

static inline void TA_VEC_SubD(const double *lhs, const double *rhs,
                               double *dest, int count)
{
   if( count <= 0 || !lhs || !rhs || !dest )
      return;

   TA_VEC_SubD_Range(lhs, rhs, dest, 0, count);
}

static inline void TA_VEC_MulD_Range(const double *lhs, const double *rhs,
                                     double *dest, int start, int length)
{
   if( length <= 0 || start < 0 || !lhs || !rhs || !dest )
      return;

   const int end = start + length;
   int i;
#if defined(_OPENMP)
   #pragma omp simd
#endif
   for( i = start; i < end; ++i )
   {
      dest[i] = lhs[i] * rhs[i];
   }
}

static inline void TA_VEC_MulD(const double *lhs, const double *rhs,
                               double *dest, int count)
{
   if( count <= 0 || !lhs || !rhs || !dest )
      return;

   TA_VEC_MulD_Range(lhs, rhs, dest, 0, count);
}

static inline void TA_VEC_DivD_Range(const double *lhs, const double *rhs,
                                     double *dest, int start, int length)
{
   if( length <= 0 || start < 0 || !lhs || !rhs || !dest )
      return;

   const int end = start + length;
   int i;
#if defined(_OPENMP)
   #pragma omp simd
#endif
   for( i = start; i < end; ++i )
   {
      dest[i] = lhs[i] / rhs[i];
   }
}

static inline void TA_VEC_DivD(const double *lhs, const double *rhs,
                               double *dest, int count)
{
   if( count <= 0 || !lhs || !rhs || !dest )
      return;

   TA_VEC_DivD_Range(lhs, rhs, dest, 0, count);
}

static inline void TA_VEC_FmaD_Range(const double *lhs, const double *rhs,
                                     const double *addend, double *dest,
                                     int start, int length)
{
   if( length <= 0 || start < 0 || !lhs || !rhs || !addend || !dest )
      return;

   const int end = start + length;
   int i;
#if defined(_OPENMP)
   #pragma omp simd
#endif
   for( i = start; i < end; ++i )
   {
      dest[i] = TA_FmaScalar(lhs[i], rhs[i], addend[i]);
   }
}

static inline void TA_VEC_FmaD(const double *lhs, const double *rhs,
                               const double *addend, double *dest, int count)
{
   if( count <= 0 || !lhs || !rhs || !addend || !dest )
      return;

   TA_VEC_FmaD_Range(lhs, rhs, addend, dest, 0, count);
}

static inline void TA_VEC_AddF_Range(const float *lhs, const float *rhs,
                                     double *dest, int start, int length)
{
   if( length <= 0 || start < 0 || !lhs || !rhs || !dest )
      return;

   const int end = start + length;
   int i;
#if defined(_OPENMP)
   #pragma omp simd
#endif
   for( i = start; i < end; ++i )
   {
      const double lhsVal = (double)lhs[i];
      const double rhsVal = (double)rhs[i];
      dest[i] = lhsVal + rhsVal;
   }
}

static inline void TA_VEC_AddF(const float *lhs, const float *rhs,
                               double *dest, int count)
{
   if( count <= 0 || !lhs || !rhs || !dest )
      return;

   TA_VEC_AddF_Range(lhs, rhs, dest, 0, count);
}

static inline void TA_VEC_SubF_Range(const float *lhs, const float *rhs,
                                     double *dest, int start, int length)
{
   if( length <= 0 || start < 0 || !lhs || !rhs || !dest )
      return;

   const int end = start + length;
   int i;
#if defined(_OPENMP)
   #pragma omp simd
#endif
   for( i = start; i < end; ++i )
   {
      const double lhsVal = (double)lhs[i];
      const double rhsVal = (double)rhs[i];
      dest[i] = lhsVal - rhsVal;
   }
}

static inline void TA_VEC_SubF(const float *lhs, const float *rhs,
                               double *dest, int count)
{
   if( count <= 0 || !lhs || !rhs || !dest )
      return;

   TA_VEC_SubF_Range(lhs, rhs, dest, 0, count);
}

static inline void TA_VEC_MulF_Range(const float *lhs, const float *rhs,
                                     double *dest, int start, int length)
{
   if( length <= 0 || start < 0 || !lhs || !rhs || !dest )
      return;

   const int end = start + length;
   int i;
#if defined(_OPENMP)
   #pragma omp simd
#endif
   for( i = start; i < end; ++i )
   {
      const double lhsVal = (double)lhs[i];
      const double rhsVal = (double)rhs[i];
      dest[i] = lhsVal * rhsVal;
   }
}

static inline void TA_VEC_MulF(const float *lhs, const float *rhs,
                               double *dest, int count)
{
   if( count <= 0 || !lhs || !rhs || !dest )
      return;

   TA_VEC_MulF_Range(lhs, rhs, dest, 0, count);
}

static inline void TA_VEC_DivF_Range(const float *lhs, const float *rhs,
                                     double *dest, int start, int length)
{
   if( length <= 0 || start < 0 || !lhs || !rhs || !dest )
      return;

   const int end = start + length;
   int i;
#if defined(_OPENMP)
   #pragma omp simd
#endif
   for( i = start; i < end; ++i )
   {
      const double lhsVal = (double)lhs[i];
      const double rhsVal = (double)rhs[i];
      dest[i] = lhsVal / rhsVal;
   }
}

static inline void TA_VEC_DivF(const float *lhs, const float *rhs,
                               double *dest, int count)
{
   if( count <= 0 || !lhs || !rhs || !dest )
      return;

   TA_VEC_DivF_Range(lhs, rhs, dest, 0, count);
}

#endif /* TA_VEC_MATH_H */
