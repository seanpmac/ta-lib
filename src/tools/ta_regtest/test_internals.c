/* TA-LIB Copyright (c) 1999-2025, Mario Fortier
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither name of author nor the names of its contributors
 *   may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* List of contributors:
 *
 *  Initial  Name/description
 *  -------------------------------------------------------------------
 *  MF       Mario Fortier
 *
 *
 * Change history:
 *
 *  MMDDYY BY   Description
 *  -------------------------------------------------------------------
 *  070401 MF   First version.
 *  050104 MF   Add TA_RegressionTest calls.
 *  080605 MF   Add tests for pseudo-random generator.
 *  091705 MF   Add tests for TA_AddTimeToTimestamp (Fix#1293953).
 *  110906 MF   Remove pseudo-random to eliminate dependencies.
 */

/* Description:
 *         Regression testing of some internal utility like:
 *            - collections: List/Stack/Circular buffer.
 *            - Memory allocation mechanism.
 *            etc...
 */

/**** Headers ****/
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ta_test_priv.h"
#include "ta_memory.h"
#include "ta_defs.h"
#include "ta_common.h"
#include "ta_batch.h"
#include "ta_vec_math.h"


/**** External functions declarations. ****/
/* None */

/**** External variables declarations. ****/
/* None */

/**** Global variables definitions.    ****/
/* None */

/**** Local declarations.              ****/
/* None */

/**** Local functions declarations.    ****/
static ErrorNumber testCircularBuffer( void );
static ErrorNumber testParallelVecOps( void );

static TA_RetCode circBufferFillFrom0ToSize( int size, int *buffer );


/**** Local variables definitions.     ****/
/* None */

/**** Global functions definitions.   ****/
/* None */

/**** Local functions definitions.     ****/
ErrorNumber test_internals( void )
{
   ErrorNumber retValue;

   printf( "Testing utility functions\n" );

   retValue = testCircularBuffer();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Circular buffer tests (%d)\n", retValue );
      return retValue;
   }

   retValue = testParallelVecOps();
    if( retValue != TA_TEST_PASS )
    {
       printf( "\nFailed: Parallel vector tests (%d)\n", retValue );
       return retValue;
    }

   return TA_TEST_PASS; /* Success. */
}

static ErrorNumber testCircularBuffer( void )
{
   TA_RetCode retCode;
   int i;
   int buffer[20];
   ErrorNumber retValue;

   /* Initialize the library. */
   retValue = allocLib();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Can't initialize the library\n" );
      return retValue;
   }

   /* The following function is supose to fill
    * the buffer with the value 0 to 8 sequentialy,
    * if somehow it is not 0 to 8, there is a bug!
    */
   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 1, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (1+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_1;
      }
   }

   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 2, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (2+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_2;
      }
   }

   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 3, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (3+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_3;
      }
   }

   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 4, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (4+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_4;
      }
   }

   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 5, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (5+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_5;
      }
   }

   memset( buffer, 0xFF, sizeof(buffer) );
   retCode = circBufferFillFrom0ToSize( 6, buffer );
   if( retCode != TA_SUCCESS )
   {
      printf( "\nFailed circular buffer test RetCode = %d\n", retCode );
      return TA_INTERNAL_CIRC_BUFF_FAIL_0;
   }
   for( i=0; i < (6+3); i++ )
   {
      if( buffer[i] != i )
      {
         printf( "\nFailed circular buffer test (%d != %d)\n", buffer[i], i );
         return TA_INTERNAL_CIRC_BUFF_FAIL_6;
      }
   }

   retValue = freeLib();
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS; /* Success. */
}

/* This function is suppose to fill the buffer
 * with values going from 0 to 'size'.
 * The filling is done using the CIRCBUF macros.
 */
static TA_RetCode circBufferFillFrom0ToSize( int size, int *buffer )
{
   CIRCBUF_PROLOG(MyBuf,int,4);
   int i, value;
   int outIdx;

   CIRCBUF_INIT(MyBuf,int,size);

   outIdx = 0;

   // 1st Loop: Fill MyBuf with initial values
   //           (must be done).
   value = 0;
   for( i=0; i < size; i++ )
   {
      MyBuf[MyBuf_Idx] = value++;
      CIRCBUF_NEXT(MyBuf);
   }

   // 2nd Loop: Get and Add subsequent values
   //           in MyBuf (optional)
   for( i=0; i < 3; i++ )
   {
      buffer[outIdx++] = MyBuf[MyBuf_Idx];
      MyBuf[MyBuf_Idx] = value++;
      CIRCBUF_NEXT(MyBuf);
   }

   // 3rd Loop: Empty MyBuf (optional)
   for( i=0; i < size; i++ )
   {
      buffer[outIdx++] = MyBuf[MyBuf_Idx];
      CIRCBUF_NEXT(MyBuf);
   }

   CIRCBUF_DESTROY(MyBuf);

   return TA_SUCCESS;
}

static int almostEqual(double a, double b)
{
   const double diff = fabs(a - b);
   const double scale = fmax(fabs(a), fabs(b));
   const double tol = 1e-12 * (scale > 1.0 ? scale : 1.0);
   return diff <= tol;
}

static ErrorNumber testParallelVecOps( void )
{
   static const int counts[] = { 5, 10000, 35000 };
   const size_t numCounts = sizeof(counts) / sizeof(counts[0]);
   ErrorNumber retValue;

   retValue = allocLib();
   if( retValue != TA_TEST_PASS )
   {
      printf( "\nFailed: Can't initialize the library\n" );
      return retValue;
   }

   for( size_t idx = 0; idx < numCounts; ++idx )
   {
      const int count = counts[idx];
      double *a = (double *)TA_Malloc(sizeof(double) * (size_t)count);
      double *b = (double *)TA_Malloc(sizeof(double) * (size_t)count);
      double *c = (double *)TA_Malloc(sizeof(double) * (size_t)count);
      double *destSerial = (double *)TA_Malloc(sizeof(double) * (size_t)count);
      double *destParallel = (double *)TA_Malloc(sizeof(double) * (size_t)count);

      if( !a || !b || !c || !destSerial || !destParallel )
      {
         if( a ) TA_Free(a);
         if( b ) TA_Free(b);
         if( c ) TA_Free(c);
         if( destSerial ) TA_Free(destSerial);
         if( destParallel ) TA_Free(destParallel);
         freeLib();
         return TA_INTERNAL_PARALLEL_VEC_ALLOC_FAILED;
      }

      for( int i = 0; i < count; ++i )
      {
         a[i] = (double)(i + 1) * 1.5;
         b[i] = (double)((i % 17) + 1) * 0.75;
         c[i] = (double)((i % 13) - 6) * 0.5;
      }

      TA_BatchConfig config;
      TA_BatchConfig_Init(&config);
      config.numThreads = 0; /* auto */
      config.schedulePolicy = (idx == 1) ? 1 : ((idx == 2) ? 2 : 0);
      config.chunkSize = (idx == 0) ? 0 : (idx == 1 ? 257 : 0);

      /* Addition */
      TA_VEC_AddD(a, b, destSerial, count);
      memset(destParallel, 0, sizeof(double) * (size_t)count);
      TA_ParallelVecAdd(a, b, destParallel, count, &config);
      for( int i = 0; i < count; ++i )
      {
         if( !almostEqual(destSerial[i], destParallel[i]) )
         {
            TA_Free(a);
            TA_Free(b);
            TA_Free(destSerial);
            TA_Free(destParallel);
            freeLib();
            return TA_INTERNAL_PARALLEL_VEC_ADD_MISMATCH;
         }
      }

      /* Subtraction */
      TA_VEC_SubD(a, b, destSerial, count);
      memset(destParallel, 0, sizeof(double) * (size_t)count);
      TA_ParallelVecSub(a, b, destParallel, count, &config);
      for( int i = 0; i < count; ++i )
      {
         if( !almostEqual(destSerial[i], destParallel[i]) )
         {
            TA_Free(a);
            TA_Free(b);
            TA_Free(destSerial);
            TA_Free(destParallel);
            freeLib();
            return TA_INTERNAL_PARALLEL_VEC_SUB_MISMATCH;
         }
      }

      /* Multiplication */
      TA_VEC_MulD(a, b, destSerial, count);
      memset(destParallel, 0, sizeof(double) * (size_t)count);
      TA_ParallelVecMul(a, b, destParallel, count, &config);
      for( int i = 0; i < count; ++i )
      {
         if( !almostEqual(destSerial[i], destParallel[i]) )
         {
            TA_Free(a);
            TA_Free(b);
            TA_Free(destSerial);
            TA_Free(destParallel);
            freeLib();
            return TA_INTERNAL_PARALLEL_VEC_MUL_MISMATCH;
         }
      }

      /* Division */
      TA_VEC_DivD(a, b, destSerial, count);
      memset(destParallel, 0, sizeof(double) * (size_t)count);
      TA_ParallelVecDiv(a, b, destParallel, count, &config);
      for( int i = 0; i < count; ++i )
      {
         if( !almostEqual(destSerial[i], destParallel[i]) )
         {
            TA_Free(a);
            TA_Free(b);
            TA_Free(c);
            TA_Free(destSerial);
            TA_Free(destParallel);
            freeLib();
            return TA_INTERNAL_PARALLEL_VEC_DIV_MISMATCH;
         }
      }

      /* Fused multiply-add */
      TA_VEC_FmaD(a, b, c, destSerial, count);
      memset(destParallel, 0, sizeof(double) * (size_t)count);
      TA_ParallelVecFma(a, b, c, destParallel, count, &config);
      for( int i = 0; i < count; ++i )
      {
         if( !almostEqual(destSerial[i], destParallel[i]) )
         {
            TA_Free(a);
            TA_Free(b);
            TA_Free(c);
            TA_Free(destSerial);
            TA_Free(destParallel);
            freeLib();
            return TA_INTERNAL_PARALLEL_VEC_FMA_MISMATCH;
         }
      }

      TA_Free(a);
      TA_Free(b);
      TA_Free(c);
      TA_Free(destSerial);
      TA_Free(destParallel);
   }

   retValue = freeLib();
   if( retValue != TA_TEST_PASS )
      return retValue;

   return TA_TEST_PASS;
}
