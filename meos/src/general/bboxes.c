/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2024, Université libre de Bruxelles and MobilityDB
 * contributors
 *
 * MobilityDB includes portions of PostGIS version 3 source code released
 * under the GNU General Public License (GPLv2 or later).
 * Copyright (c) 2001-2024, PostGIS contributors
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without a written
 * agreement is hereby granted, provided that the above copyright notice and
 * this paragraph and the following two paragraphs appear in all copies.
 *
 * IN NO EVENT SHALL UNIVERSITE LIBRE DE BRUXELLES BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
 * LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
 * EVEN IF UNIVERSITE LIBRE DE BRUXELLES HAS BEEN ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * UNIVERSITE LIBRE DE BRUXELLES SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS ON
 * AN "AS IS" BASIS, AND UNIVERSITE LIBRE DE BRUXELLES HAS NO OBLIGATIONS TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 *****************************************************************************/

/**
 * @brief STBox expandable array type.
 */

#include "general/bboxarray.h"

/* C */
#include <assert.h>
#include <limits.h>
/* PostgreSQL */
#include <postgres.h>
#if MEOS
  #define MaxAllocSize   ((Size) 0x3fffffff) /* 1 gigabyte - 1 */
#else
  #include <utils/memutils.h>
#endif /* MEOS */
/* MEOS */
#include <meos.h>
#include <meos_internal.h>
#include <general/temporal_boxops.h>
#include <general/temporal_tile.h>

/*****************************************************************************
 * Spans functions for temporal values
 * These functions can be used for defining Multi-Entry Search Trees (a.k.a.
 * VODKA) indexes
 * https://www.pgcon.org/2014/schedule/events/696.en.html
 * https://github.com/MobilityDB/mest
 *****************************************************************************/

/**
 * @brief Set the span in the last argument from a temporal instant
 * @param[in] inst Temporal value
 * @param[out] result Span
 */
static void
tinstant_set_span(const TInstant *inst, Span *result)
{
  assert(inst); assert(result);
  span_set(TimestampTzGetDatum(inst->t), TimestampTzGetDatum(inst->t), 
    true, true, T_TIMESTAMPTZ, T_TSTZSPAN, result);
  return;
}

/**
 * @ingroup meos_internal_temporal_bbox
 * @brief Return an array of maximum n spans from a temporal instant
 * @param[in] inst Temporal value
 * @param[out] count Number of elements in the output array
 */
Span *
tinstant_spans_duration(const TInstant *inst, int *count)
{
  assert(inst);
  Span *result = palloc(sizeof(Span));
  tinstant_set_span(inst, result);
  *count = 1;
  return result;
}

/**
 * @brief Return an array of maximum n spans from the instants of a
 * temporal sequence with discrete interpolation (iterator function)
 * @param[in] seq Temporal value
 * @param[in] interv Maximum duration of spans in the output array
 * @param[out] result Temporal span
 * @return Number of elements in the array
 */
static int
tdiscseq_spans_duration_iter(const TSequence *seq, const Interval *duration,
  Span *result)
{
  assert(MEOS_FLAGS_GET_INTERP(seq->flags) == DISCRETE); 
  assert(seq->count > 1);
  /* Temporal sequence has at least 2 instants */
  Interval *seqdur = minus_timestamptz_timestamptz(
    DatumGetTimestampTz(seq->period.upper), 
    DatumGetTimestampTz(seq->period.lower));
  int64 size = interval_units(duration);
  int64 seqsize = interval_units(seqdur);
  if (seqsize <= size)
  {
    /* Copy the sequence interval */
    memcpy(result, &seq->period, sizeof(Span));
    return 1;    
  }
  else /* seqsize > size */
  {
    /* Split the interval */
    int64 count = seqsize / size;
    int64 remainder = seqsize % size;
    if (remainder) count++;
    TimestampTz lower = seq->period.lower;
    for (int i = 0; i < count; i++)
    {
      span_bucket_set(lower, size, T_TIMESTAMPTZ, &result[i]);
      lower += size;
    }
    return count;
  }
}

/**
 * @brief Return an array of maximum n spans from the segments of a
 * temporal sequence with continuous interpolation (iterator function)
 * @param[in] seq Temporal value
 * @param[in] interv Maximum duration of spans in the output array 
 * @param[out] result Temporal span
 * @return Number of elements in the array
 */
// static int
// tcontseq_spans_duration_iter(const TSequence *seq, const Interval *duration,
  // Span *result)
// {
  // assert(MEOS_FLAGS_GET_INTERP(seq->flags) != DISCRETE); 
  // assert(seq->count > 1);
  // /* Temporal sequence has at least 2 instants */
  // int nsegs = seq->count - 1;
  // if (interv < 1 || nsegs <= interv)
  // {
    // /* One bounding span per segment */
    // const TInstant *inst1 = TSEQUENCE_INST_N(seq, 0);
    // for (int i = 0; i < seq->count - 1; i++)
    // {
      // tinstant_set_span(inst1, &result[i]);
      // const TInstant *inst2 = TSEQUENCE_INST_N(seq, i + 1);
      // Span span;
      // tinstant_set_span(inst2, &span);
      // span_expand(&span, &result[i]);
      // inst1 = inst2;
    // }
    // return nsegs;
  // }
  // else
  // {
    // /* One bounding span per several consecutive segments */
    // /* Minimum number of input segments merged together in an output span */
    // int size = nsegs / interv;
    // /* Number of output boxes that result from merging (size + 1) segments */
    // int remainder = nsegs % interv;
    // int i = 0; /* Loop variable for input segments */
    // int k = 0; /* Loop variable for output boxes */
    // while (k < interv)
    // {
      // int j = i + size;
      // if (k < remainder)
        // j++;
      // assert(i < j);
      // tinstant_set_span(TSEQUENCE_INST_N(seq, i), &result[k]);
      // for (int l = i + 1; l <= j; l++)
      // {
        // const TInstant *inst = TSEQUENCE_INST_N(seq, l);
        // Span span;
        // tinstant_set_span(inst, &span);
        // span_expand(&span, &result[k]);
      // }
      // k++;
      // i = j;
    // }
    // return interv;
  // }
// }

/**
 * @brief Return an array of maximum n spans from the instants or
 * segments of a temporal sequence (iterator function)
 * @param[in] seq Temporal value
 * @param[in] interv Maximum duration of spans in the output array 
 * @param[out] result Temporal span
 * @return Number of elements in the array
 */
// static int
// tsequence_spans_duration_iter(const TSequence *seq, const Interval *duration,
  // Span *result)
// {
  // /* Instantaneous sequence */
  // if (seq->count == 1)
  // {
    // tinstant_set_span(TSEQUENCE_INST_N(seq, 0), &result[0]);
    // return 1;
  // }
  // return (MEOS_FLAGS_GET_INTERP(seq->flags) == DISCRETE) ?
    // tdiscseq_spans_duration_iter(seq, interv, result) :
    // tcontseq_spans_duration_iter(seq, interv, result);
// }

// /**
 // * @ingroup meos_internal_temporal_bbox
 // * @brief Return an array of maximum n spans from the segments
 // * of a temporal number sequence
 // * @param[in] seq Temporal sequence
 // * @param[in] interv Maximum duration of spans in the output array 
 // * @param[out] count Number of elements in the output array
 // */
// Span *
// tsequence_spans_duration(const TSequence *seq, const Interval *duration, int *count)
// {
  // assert(seq); assert(count);
  // int nboxes = (interv < 1) ?
    // ( seq->count == 1 ? 1 : seq->count - 1 ) : interv;
  // Span *result = palloc(sizeof(Span) * nboxes);
  // *count = tsequence_spans_duration_iter(seq, interv, result);
  // return result;
// }

// /**
 // * @ingroup meos_internal_temporal_bbox
 // * @brief Return an array of spans from the segments of a temporal sequence set
 // * @param[in] ss Temporal sequence set
 // * @param[in] interv Maximum duration of spans in the output array 
 // * @param[out] count Number of elements in the output array
 // */
// Span *
// tsequenceset_spans_duration(const TSequenceSet *ss, const Interval *duration, int *count)
// {
  // assert(ss); assert(count);
  // int nboxes = (interv < 1) ? ss->totalcount : interv;
  // Span *result = palloc(sizeof(Span) * nboxes);
  // int nboxes1;
  // if (interv < 1 || ss->totalcount <= interv)
  // {
    // /* One bounding span per segment */
    // nboxes1 = 0;
    // for (int i = 0; i < ss->count; i++)
      // nboxes1 += tsequence_spans_duration_iter(TSEQUENCESET_SEQ_N(ss, i),
        // interv, &result[nboxes1]);
    // *count = nboxes1;
    // return result;
  // }
  // else if (ss->count <= interv)
  // {
    // /* Amount of bounding boxes per composing sequence determined from the
     // * proportion of seq->count and ss->totalcount */
    // nboxes1 = 0;
    // for (int i = 0; i < ss->count; i++)
    // {
      // const TSequence *seq = TSEQUENCESET_SEQ_N(ss, i);
      // int nboxes_seq = (int) (interv * seq->count * 1.0 / ss->totalcount);
      // if (! nboxes_seq)
        // nboxes_seq = 1;
      // nboxes1 += tsequence_spans_duration_iter(seq, nboxes_seq,
        // &result[nboxes1]);
    // }
    // *count = nboxes1;
    // return result;
  // }
  // else
  // {
    // /* Merge consecutive sequences to reach the maximum number of boxes */
    // /* Minimum number of sequences merged together in an output span */
    // int size = ss->count / interv;
    // /* Number of output boxes that result from merging (size + 1) sequences */
    // int remainder = ss->count % interv;
    // int i = 0; /* Loop variable for input sequences */
    // int k = 0; /* Loop variable for output boxes */
    // while (k < interv)
    // {
      // int j = i + size - 1;
      // if (k < remainder)
        // j++;
      // if (i < j)
      // {
        // tsequence_spans_duration_iter(TSEQUENCESET_SEQ_N(ss, i), 1,
          // &result[k]);
        // for (int l = i + 1; l <= j; l++)
        // {
          // Span span;
          // tsequence_spans_duration_iter(TSEQUENCESET_SEQ_N(ss, l), 1, &span);
          // span_expand(&span, &result[k]);
        // }
        // i = j + 1;
        // k++;
      // }
      // else
        // tsequence_spans_duration_iter(TSEQUENCESET_SEQ_N(ss, i++), 1,
          // &result[k++]);
    // }
    // *count = interv;
    // return result;
  // }
// }

/**
 * @ingroup meos_temporal_bbox
 * @brief Return an array of spans from the segments of a temporal value
 * @param[in] temp Temporal value
 * @param[in] interv Maximum number of elements in the output array.
 * @param[out] count Number of values of the output array
 * @return On error return @p NULL
 * @csqlfn #Temporal_spans_duration()
 */
// Span *
// temporal_spans_duration(const Temporal *temp, const Interval *duration, int *count)
// {
  // /* Ensure validity of the arguments */
  // if (! ensure_not_null((void *) temp) || ! ensure_not_null((void *) count))
    // return NULL;

  // assert(temptype_subtype(temp->subtype));
  // if (temp->subtype == TINSTANT)
    // return tinstant_spans_duration((TInstant *) temp, count);
  // else if (temp->subtype == TSEQUENCE)
    // return tsequence_spans_duration((TSequence *) temp, interv, count);
  // else /* TSEQUENCESET */
    // return tsequenceset_spans_duration((TSequenceSet *) temp, interv, count);
// }

