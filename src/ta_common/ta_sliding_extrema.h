/*
** ta_sliding_extrema.h - Monotonic deque for O(n) sliding window min/max
**
** Algorithm: Maintain a deque of (index, value) pairs in monotonic order.
** For MIN: increasing values (front = min), for MAX: decreasing values (front = max).
**
** Invariants:
** 1. Front element is always the current extremum
** 2. All elements in deque are within the window [trailingIdx, today]
** 3. Values are strictly monotonic (with special handling for ties)
**
** Complexity: O(n) amortized - each element enters/exits deque at most once
*/

#ifndef TA_SLIDING_EXTREMA_H
#define TA_SLIDING_EXTREMA_H

#include <stddef.h>
#include <stdlib.h>

/* Stack-allocated deque for small periods, heap for large */
#define TA_EXTREMA_STACK_THRESHOLD 128

/* Element in the deque: (index, value) pair */
typedef struct {
    int idx;
    double val;
} TA_DequeElement;

/* Monotonic deque for sliding window extrema */
typedef struct {
    TA_DequeElement *elements;  /* Circular buffer */
    int capacity;               /* Total capacity */
    int front;                  /* Index of front element */
    int back;                   /* Index one past back element */
    int isMinDeque;             /* 1 for min (increasing), 0 for max (decreasing) */
    TA_DequeElement stackBuf[TA_EXTREMA_STACK_THRESHOLD];  /* Stack allocation */
} TA_ExtremaDeque;

/* Initialize deque for given period */
static inline void TA_ExtremaDeque_Init(TA_ExtremaDeque *dq, int period, int isMin)
{
    dq->capacity = period + 1;  /* +1 for circular buffer logic */
    dq->front = 0;
    dq->back = 0;
    dq->isMinDeque = isMin;
    
    if( dq->capacity <= TA_EXTREMA_STACK_THRESHOLD )
    {
        dq->elements = dq->stackBuf;
    }
    else
    {
        dq->elements = (TA_DequeElement*)malloc(dq->capacity * sizeof(TA_DequeElement));
    }
}

/* Free deque resources */
static inline void TA_ExtremaDeque_Free(TA_ExtremaDeque *dq)
{
    if( dq->elements != dq->stackBuf && dq->elements != NULL )
    {
        free(dq->elements);
        dq->elements = NULL;
    }
}

/* Check if deque is empty */
static inline int TA_ExtremaDeque_IsEmpty(const TA_ExtremaDeque *dq)
{
    return dq->front == dq->back;
}

/* Get front element (current extremum) */
static inline TA_DequeElement TA_ExtremaDeque_Front(const TA_ExtremaDeque *dq)
{
    return dq->elements[dq->front];
}

/* Get back element */
static inline TA_DequeElement TA_ExtremaDeque_Back(const TA_ExtremaDeque *dq)
{
    int backIdx = (dq->back - 1 + dq->capacity) % dq->capacity;
    return dq->elements[backIdx];
}

/* Remove element from front */
static inline void TA_ExtremaDeque_PopFront(TA_ExtremaDeque *dq)
{
    if( !TA_ExtremaDeque_IsEmpty(dq) )
    {
        dq->front = (dq->front + 1) % dq->capacity;
    }
}

/* Remove element from back */
static inline void TA_ExtremaDeque_PopBack(TA_ExtremaDeque *dq)
{
    if( !TA_ExtremaDeque_IsEmpty(dq) )
    {
        dq->back = (dq->back - 1 + dq->capacity) % dq->capacity;
    }
}

/* Add element to back */
static inline void TA_ExtremaDeque_PushBack(TA_ExtremaDeque *dq, int idx, double val)
{
    dq->elements[dq->back].idx = idx;
    dq->elements[dq->back].val = val;
    dq->back = (dq->back + 1) % dq->capacity;
}

/* Compare values based on deque type (min or max) */
static inline int TA_ExtremaDeque_ShouldReplace(const TA_ExtremaDeque *dq, 
                                                  double newVal, double oldVal)
{
    if( dq->isMinDeque )
    {
        /* For MIN: replace if new value is smaller or equal (prefer newer on tie) */
        return newVal <= oldVal;
    }
    else
    {
        /* For MAX: replace if new value is larger or equal (prefer newer on tie) */
        return newVal >= oldVal;
    }
}

/* Main operation: push new element and maintain monotonic property */
static inline void TA_ExtremaDeque_Push(TA_ExtremaDeque *dq, int idx, double val, 
                                         int trailingIdx)
{
    /* Remove elements outside the window from front */
    while( !TA_ExtremaDeque_IsEmpty(dq) && 
           TA_ExtremaDeque_Front(dq).idx < trailingIdx )
    {
        TA_ExtremaDeque_PopFront(dq);
    }
    
    /* Maintain monotonic property: remove elements from back that violate it */
    while( !TA_ExtremaDeque_IsEmpty(dq) && 
           TA_ExtremaDeque_ShouldReplace(dq, val, TA_ExtremaDeque_Back(dq).val) )
    {
        TA_ExtremaDeque_PopBack(dq);
    }
    
    /* Add new element to back */
    TA_ExtremaDeque_PushBack(dq, idx, val);
}

/* Get current extremum value */
static inline double TA_ExtremaDeque_GetValue(const TA_ExtremaDeque *dq)
{
    if( TA_ExtremaDeque_IsEmpty(dq) )
        return 0.0;
    return TA_ExtremaDeque_Front(dq).val;
}

/* Get current extremum index */
static inline int TA_ExtremaDeque_GetIndex(const TA_ExtremaDeque *dq)
{
    if( TA_ExtremaDeque_IsEmpty(dq) )
        return -1;
    return TA_ExtremaDeque_Front(dq).idx;
}

#endif /* TA_SLIDING_EXTREMA_H */
