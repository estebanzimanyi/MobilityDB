-------------------------------------------------------------------------------
--
-- This MobilityDB code is provided under The PostgreSQL License.
-- Copyright (c) 2016-2025, Université libre de Bruxelles and MobilityDB
-- contributors
--
-- MobilityDB includes portions of PostGIS version 3 source code released
-- under the GNU General Public License (GPLv2 or later).
-- Copyright (c) 2001-2025, PostGIS contributors
--
-- Permission to use, copy, modify, and distribute this software and its
-- documentation for any purpose, without fee, and without a written
-- agreement is hereby granted, provided that the above copyright notice and
-- this paragraph and the following two paragraphs appear in all copies.
--
-- IN NO EVENT SHALL UNIVERSITE LIBRE DE BRUXELLES BE LIABLE TO ANY PARTY FOR
-- DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
-- LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
-- EVEN IF UNIVERSITE LIBRE DE BRUXELLES HAS BEEN ADVISED OF THE POSSIBILITY
-- OF SUCH DAMAGE.
--
-- UNIVERSITE LIBRE DE BRUXELLES SPECIFICALLY DISCLAIMS ANY WARRANTIES,
-- INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
-- AND FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS ON
-- AN "AS IS" BASIS, AND UNIVERSITE LIBRE DE BRUXELLES HAS NO OBLIGATIONS TO
-- PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
--
-------------------------------------------------------------------------------

--------------------------------------------------------------------------------
-- Send/receive functions
--------------------------------------------------------------------------------

COPY tbl_tgeompoint TO '/tmp/tbl_tgeompoint' (FORMAT BINARY);
COPY tbl_tgeogpoint TO '/tmp/tbl_tgeogpoint' (FORMAT BINARY);

DROP TABLE IF EXISTS tbl_tgeompoint_tmp;
DROP TABLE IF EXISTS tbl_tgeogpoint_tmp;

CREATE TABLE tbl_tgeompoint_tmp AS TABLE tbl_tgeompoint WITH NO DATA;
CREATE TABLE tbl_tgeogpoint_tmp AS TABLE tbl_tgeogpoint WITH NO DATA;

COPY tbl_tgeompoint_tmp FROM '/tmp/tbl_tgeompoint' (FORMAT BINARY);
COPY tbl_tgeogpoint_tmp FROM '/tmp/tbl_tgeogpoint' (FORMAT BINARY);

SELECT COUNT(*) FROM tbl_tgeompoint t1, tbl_tgeompoint_tmp t2 WHERE t1.k = t2.k AND t1.temp <> t2.temp;
SELECT COUNT(*) FROM tbl_tgeogpoint t1, tbl_tgeogpoint_tmp t2 WHERE t1.k = t2.k AND t1.temp <> t2.temp;

DROP TABLE tbl_tgeompoint_tmp;
DROP TABLE tbl_tgeogpoint_tmp;

------------------------------------------------------------------------------
-- Transformation functions
------------------------------------------------------------------------------

SELECT DISTINCT tempSubtype(tgeompointInst(inst)) FROM tbl_tgeompoint_inst;
SELECT DISTINCT tempSubtype(setInterp(inst, 'discrete')) FROM tbl_tgeompoint_inst;
SELECT DISTINCT tempSubtype(tgeompointSeq(inst)) FROM tbl_tgeompoint_inst;
SELECT DISTINCT tempSubtype(tgeompointSeqSet(inst)) FROM tbl_tgeompoint_inst;

SELECT DISTINCT tempSubtype(tgeompointInst(inst)) FROM tbl_tgeompoint3D_inst;
SELECT DISTINCT tempSubtype(setInterp(inst, 'discrete')) FROM tbl_tgeompoint3D_inst;
SELECT DISTINCT tempSubtype(tgeompointSeq(inst)) FROM tbl_tgeompoint3D_inst;
SELECT DISTINCT tempSubtype(tgeompointSeqSet(inst)) FROM tbl_tgeompoint3D_inst;

SELECT DISTINCT tempSubtype(tgeogpointInst(inst)) FROM tbl_tgeogpoint_inst;
SELECT DISTINCT tempSubtype(setInterp(inst, 'discrete')) FROM tbl_tgeogpoint_inst;
SELECT DISTINCT tempSubtype(tgeogpointSeq(inst)) FROM tbl_tgeogpoint_inst;
SELECT DISTINCT tempSubtype(tgeogpointSeqSet(inst)) FROM tbl_tgeogpoint_inst;

SELECT DISTINCT tempSubtype(tgeogpointInst(inst)) FROM tbl_tgeogpoint3D_inst;
SELECT DISTINCT tempSubtype(setInterp(inst, 'discrete')) FROM tbl_tgeogpoint3D_inst;
SELECT DISTINCT tempSubtype(tgeogpointSeq(inst)) FROM tbl_tgeogpoint3D_inst;
SELECT DISTINCT tempSubtype(tgeogpointSeqSet(inst)) FROM tbl_tgeogpoint3D_inst;

------------------------------------------------------------------------------/

SELECT DISTINCT tempSubtype(tgeompointInst(ti)) FROM tbl_tgeompoint_discseq WHERE numInstants(ti) = 1;
SELECT DISTINCT tempSubtype(setInterp(ti, 'discrete')) FROM tbl_tgeompoint_discseq;
SELECT DISTINCT tempSubtype(tgeompointSeq(ti)) FROM tbl_tgeompoint_discseq WHERE numInstants(ti) = 1;
SELECT DISTINCT tempSubtype(tgeompointSeqSet(ti)) FROM tbl_tgeompoint_discseq;

SELECT DISTINCT tempSubtype(tgeompointInst(ti)) FROM tbl_tgeompoint3D_discseq WHERE numInstants(ti) = 1;
SELECT DISTINCT tempSubtype(setInterp(ti, 'discrete')) FROM tbl_tgeompoint3D_discseq;
SELECT DISTINCT tempSubtype(tgeompointSeq(ti)) FROM tbl_tgeompoint3D_discseq WHERE numInstants(ti) = 1;
SELECT DISTINCT tempSubtype(tgeompointSeqSet(ti)) FROM tbl_tgeompoint3D_discseq;

SELECT DISTINCT tempSubtype(tgeogpointInst(ti)) FROM tbl_tgeogpoint_discseq WHERE numInstants(ti) = 1;
SELECT DISTINCT tempSubtype(setInterp(ti, 'discrete')) FROM tbl_tgeogpoint_discseq;
SELECT DISTINCT tempSubtype(tgeogpointSeq(ti)) FROM tbl_tgeogpoint_discseq WHERE numInstants(ti) = 1;
SELECT DISTINCT tempSubtype(tgeogpointSeqSet(ti)) FROM tbl_tgeogpoint_discseq;

SELECT DISTINCT tempSubtype(tgeogpointInst(ti)) FROM tbl_tgeogpoint3D_discseq WHERE numInstants(ti) = 1;
SELECT DISTINCT tempSubtype(setInterp(ti, 'discrete')) FROM tbl_tgeogpoint3D_discseq;
SELECT DISTINCT tempSubtype(tgeogpointSeq(ti)) FROM tbl_tgeogpoint3D_discseq WHERE numInstants(ti) = 1;
SELECT DISTINCT tempSubtype(tgeogpointSeqSet(ti)) FROM tbl_tgeogpoint3D_discseq;

------------------------------------------------------------------------------/

SELECT DISTINCT tempSubtype(tgeompointInst(seq)) FROM tbl_tgeompoint_seq WHERE numInstants(seq) = 1;
SELECT DISTINCT tempSubtype(setInterp(seq, 'discrete')) FROM tbl_tgeompoint_seq WHERE numInstants(seq) = 1;
SELECT DISTINCT tempSubtype(tgeompointSeq(seq)) FROM tbl_tgeompoint_seq;
SELECT DISTINCT tempSubtype(tgeompointSeqSet(seq)) FROM tbl_tgeompoint_seq;

SELECT DISTINCT tempSubtype(tgeompointInst(seq)) FROM tbl_tgeompoint3D_seq WHERE numInstants(seq) = 1;
SELECT DISTINCT tempSubtype(setInterp(seq, 'discrete')) FROM tbl_tgeompoint3D_seq WHERE numInstants(seq) = 1;
SELECT DISTINCT tempSubtype(tgeompointSeq(seq)) FROM tbl_tgeompoint3D_seq;
SELECT DISTINCT tempSubtype(tgeompointSeqSet(seq)) FROM tbl_tgeompoint3D_seq;

SELECT DISTINCT tempSubtype(tgeogpointInst(seq)) FROM tbl_tgeogpoint_seq WHERE numInstants(seq) = 1;
SELECT DISTINCT tempSubtype(setInterp(seq, 'discrete')) FROM tbl_tgeogpoint_seq WHERE numInstants(seq) = 1;
SELECT DISTINCT tempSubtype(tgeogpointSeq(seq)) FROM tbl_tgeogpoint_seq;
SELECT DISTINCT tempSubtype(tgeogpointSeqSet(seq)) FROM tbl_tgeogpoint_seq;

SELECT DISTINCT tempSubtype(tgeogpointInst(seq)) FROM tbl_tgeogpoint3D_seq WHERE numInstants(seq) = 1;
SELECT DISTINCT tempSubtype(setInterp(seq, 'discrete')) FROM tbl_tgeogpoint3D_seq WHERE numInstants(seq) = 1;
SELECT DISTINCT tempSubtype(tgeogpointSeq(seq)) FROM tbl_tgeogpoint3D_seq;
SELECT DISTINCT tempSubtype(tgeogpointSeqSet(seq)) FROM tbl_tgeogpoint3D_seq;

------------------------------------------------------------------------------/

SELECT DISTINCT tempSubtype(tgeompointInst(ss)) FROM tbl_tgeompoint_seqset WHERE numInstants(ss) = 1;
SELECT DISTINCT tempSubtype(setInterp(ss, 'discrete')) FROM tbl_tgeompoint_seqset WHERE duration(ss) = '00:00:00';
SELECT DISTINCT tempSubtype(tgeompointSeq(ss)) FROM tbl_tgeompoint_seqset WHERE numSequences(ss) = 1;
SELECT DISTINCT tempSubtype(tgeompointSeqSet(ss)) FROM tbl_tgeompoint_seqset;

SELECT DISTINCT tempSubtype(tgeompointInst(ss)) FROM tbl_tgeompoint3D_seqset WHERE numInstants(ss) = 1;
SELECT DISTINCT tempSubtype(setInterp(ss, 'discrete')) FROM tbl_tgeompoint3D_seqset WHERE duration(ss) = '00:00:00';
SELECT DISTINCT tempSubtype(tgeompointSeq(ss)) FROM tbl_tgeompoint3D_seqset WHERE numSequences(ss) = 1;
SELECT DISTINCT tempSubtype(tgeompointSeqSet(ss)) FROM tbl_tgeompoint3D_seqset;

SELECT DISTINCT tempSubtype(tgeogpointInst(ss)) FROM tbl_tgeogpoint_seqset WHERE numInstants(ss) = 1;
SELECT DISTINCT tempSubtype(setInterp(ss, 'discrete')) FROM tbl_tgeogpoint_seqset WHERE duration(ss) = '00:00:00';
SELECT DISTINCT tempSubtype(tgeogpointSeq(ss)) FROM tbl_tgeogpoint_seqset WHERE numSequences(ss) = 1;
SELECT DISTINCT tempSubtype(tgeogpointSeqSet(ss)) FROM tbl_tgeogpoint_seqset;

SELECT DISTINCT tempSubtype(tgeogpointInst(ss)) FROM tbl_tgeogpoint3D_seqset WHERE numInstants(ss) = 1;
SELECT DISTINCT tempSubtype(setInterp(ss, 'discrete')) FROM tbl_tgeogpoint3D_seqset WHERE duration(ss) = '00:00:00';
SELECT DISTINCT tempSubtype(tgeogpointSeq(ss)) FROM tbl_tgeogpoint3D_seqset WHERE numSequences(ss) = 1;
SELECT DISTINCT tempSubtype(tgeogpointSeqSet(ss)) FROM tbl_tgeogpoint3D_seqset;

------------------------------------------------------------------------------

SELECT MAX(numInstants(appendInstant(temp, shiftTime(endInstant(temp), '5 min')))) FROM tbl_tgeompoint;
SELECT MAX(numInstants(appendInstant(temp, shiftTime(endInstant(temp), '5 min')))) FROM tbl_tgeogpoint;

------------------------------------------------------------------------------
-- Accessor functions
------------------------------------------------------------------------------

SELECT DISTINCT tempSubtype(temp) FROM tbl_tgeompoint ORDER BY 1;
SELECT DISTINCT tempSubtype(temp) FROM tbl_tgeogpoint ORDER BY 1;
SELECT DISTINCT tempSubtype(temp) FROM tbl_tgeompoint3D ORDER BY 1;
SELECT DISTINCT tempSubtype(temp) FROM tbl_tgeogpoint3D ORDER BY 1;

-- The size of geometries increased a few bytes in PostGIS 3
SELECT COUNT(*) FROM tbl_tgeompoint WHERE memSize(temp) > 0;
SELECT COUNT(*) FROM tbl_tgeogpoint WHERE memSize(temp) > 0;
SELECT COUNT(*) FROM tbl_tgeompoint3D WHERE memSize(temp) > 0;
SELECT COUNT(*) FROM tbl_tgeogpoint3D WHERE memSize(temp) > 0;

SELECT MAX(Xmin(round(stbox(temp), 6))) FROM tbl_tgeompoint;
SELECT MAX(Xmin(round(stbox(temp), 6))) FROM tbl_tgeogpoint;
SELECT MAX(Xmin(round(stbox(temp), 6))) FROM tbl_tgeompoint3D;
SELECT MAX(Xmin(round(stbox(temp), 6))) FROM tbl_tgeogpoint3D;

/* There is no ST_MemSize neither MAX for geography. */
SELECT MAX(ST_MemSize(getValue(inst))) FROM tbl_tgeompoint_inst;
SELECT MAX(ST_MemSize(getValue(inst)::geometry)) FROM tbl_tgeogpoint_inst;
SELECT MAX(ST_MemSize(getValue(inst))) FROM tbl_tgeompoint3D_inst;
SELECT MAX(ST_MemSize(getValue(inst)::geometry)) FROM tbl_tgeogpoint3D_inst;

SELECT MAX(memSize(valueSet(temp))) FROM tbl_tgeompoint;
SELECT MAX(memSize(valueSet(temp))) FROM tbl_tgeogpoint;
SELECT MAX(memSize(valueSet(temp))) FROM tbl_tgeompoint3D;
SELECT MAX(memSize(valueSet(temp))) FROM tbl_tgeogpoint3D;

SELECT MAX(ST_MemSize(startValue(temp))) FROM tbl_tgeompoint;
SELECT MAX(ST_MemSize(startValue(temp)::geometry)) FROM tbl_tgeogpoint;
SELECT MAX(ST_MemSize(startValue(temp))) FROM tbl_tgeompoint3D;
SELECT MAX(ST_MemSize(startValue(temp)::geometry)) FROM tbl_tgeogpoint3D;

SELECT MAX(ST_MemSize(endValue(temp))) FROM tbl_tgeompoint;
SELECT MAX(ST_MemSize(endValue(temp)::geometry)) FROM tbl_tgeogpoint;
SELECT MAX(ST_MemSize(endValue(temp))) FROM tbl_tgeompoint3D;
SELECT MAX(ST_MemSize(endValue(temp)::geometry)) FROM tbl_tgeogpoint3D;

SELECT MAX(ST_MemSize(valueN(temp, 1))) FROM tbl_tgeompoint;
SELECT MAX(ST_MemSize(valueN(temp, 1)::geometry)) FROM tbl_tgeogpoint;
SELECT MAX(ST_MemSize(valueN(temp, 1))) FROM tbl_tgeompoint3D;
SELECT MAX(ST_MemSize(valueN(temp, 1)::geometry)) FROM tbl_tgeogpoint3D;

SELECT MAX(getTimestamp(inst)) FROM tbl_tgeompoint_inst;
SELECT MAX(getTimestamp(inst)) FROM tbl_tgeogpoint_inst;
SELECT MAX(getTimestamp(inst)) FROM tbl_tgeompoint3D_inst;
SELECT MAX(getTimestamp(inst)) FROM tbl_tgeogpoint3D_inst;

SELECT MAX(duration(getTime(temp))) FROM tbl_tgeompoint;
SELECT MAX(duration(getTime(temp))) FROM tbl_tgeogpoint;
SELECT MAX(duration(getTime(temp))) FROM tbl_tgeompoint3D;
SELECT MAX(duration(getTime(temp))) FROM tbl_tgeogpoint3D;

SELECT MAX(duration(timeSpan(temp))) FROM tbl_tgeompoint;
SELECT MAX(duration(timeSpan(temp))) FROM tbl_tgeogpoint;
SELECT MAX(duration(timeSpan(temp))) FROM tbl_tgeompoint3D;
SELECT MAX(duration(timeSpan(temp))) FROM tbl_tgeogpoint3D;

SELECT MAX(duration(temp)) FROM tbl_tgeompoint;
SELECT MAX(duration(temp)) FROM tbl_tgeogpoint;
SELECT MAX(duration(temp)) FROM tbl_tgeompoint3D;
SELECT MAX(duration(temp)) FROM tbl_tgeogpoint3D;

SELECT MAX(numSequences(seq)) FROM tbl_tgeompoint_seq;
SELECT MAX(numSequences(seq)) FROM tbl_tgeogpoint_seq;
SELECT MAX(numSequences(seq)) FROM tbl_tgeompoint3D_seq;
SELECT MAX(numSequences(seq)) FROM tbl_tgeogpoint3D_seq;

SELECT MAX(duration(startSequence(seq))) FROM tbl_tgeompoint_seq;
SELECT MAX(duration(startSequence(seq))) FROM tbl_tgeogpoint_seq;
SELECT MAX(duration(startSequence(seq))) FROM tbl_tgeompoint3D_seq;
SELECT MAX(duration(startSequence(seq))) FROM tbl_tgeogpoint3D_seq;

SELECT MAX(duration(endSequence(seq))) FROM tbl_tgeompoint_seq;
SELECT MAX(duration(endSequence(seq))) FROM tbl_tgeogpoint_seq;
SELECT MAX(duration(endSequence(seq))) FROM tbl_tgeompoint3D_seq;
SELECT MAX(duration(endSequence(seq))) FROM tbl_tgeogpoint3D_seq;

SELECT MAX(duration(sequenceN(seq, numSequences(seq)))) FROM tbl_tgeompoint_seq;
SELECT MAX(duration(sequenceN(seq, numSequences(seq)))) FROM tbl_tgeogpoint_seq;
SELECT MAX(duration(sequenceN(seq, numSequences(seq)))) FROM tbl_tgeompoint3D_seq;
SELECT MAX(duration(sequenceN(seq, numSequences(seq)))) FROM tbl_tgeogpoint3D_seq;

SELECT MAX(array_length(sequences(seq),1)) FROM tbl_tgeompoint_seq;
SELECT MAX(array_length(sequences(seq),1)) FROM tbl_tgeogpoint_seq;
SELECT MAX(array_length(sequences(seq),1)) FROM tbl_tgeompoint3D_seq;
SELECT MAX(array_length(sequences(seq),1)) FROM tbl_tgeogpoint3D_seq;

SELECT MAX(numSequences(ss)) FROM tbl_tgeompoint_seqset;
SELECT MAX(numSequences(ss)) FROM tbl_tgeogpoint_seqset;
SELECT MAX(numSequences(ss)) FROM tbl_tgeompoint3D_seqset;
SELECT MAX(numSequences(ss)) FROM tbl_tgeogpoint3D_seqset;

SELECT MAX(duration(startSequence(ss))) FROM tbl_tgeompoint_seqset;
SELECT MAX(duration(startSequence(ss))) FROM tbl_tgeogpoint_seqset;
SELECT MAX(duration(startSequence(ss))) FROM tbl_tgeompoint3D_seqset;
SELECT MAX(duration(startSequence(ss))) FROM tbl_tgeogpoint3D_seqset;

SELECT MAX(duration(endSequence(ss))) FROM tbl_tgeompoint_seqset;
SELECT MAX(duration(endSequence(ss))) FROM tbl_tgeogpoint_seqset;
SELECT MAX(duration(endSequence(ss))) FROM tbl_tgeompoint3D_seqset;
SELECT MAX(duration(endSequence(ss))) FROM tbl_tgeogpoint3D_seqset;

SELECT MAX(duration(sequenceN(ss, numSequences(ss)))) FROM tbl_tgeompoint_seqset;
SELECT MAX(duration(sequenceN(ss, numSequences(ss)))) FROM tbl_tgeogpoint_seqset;
SELECT MAX(duration(sequenceN(ss, numSequences(ss)))) FROM tbl_tgeompoint3D_seqset;
SELECT MAX(duration(sequenceN(ss, numSequences(ss)))) FROM tbl_tgeogpoint3D_seqset;

SELECT MAX(array_length(sequences(ss),1)) FROM tbl_tgeompoint_seqset;
SELECT MAX(array_length(sequences(ss),1)) FROM tbl_tgeogpoint_seqset;
SELECT MAX(array_length(sequences(ss),1)) FROM tbl_tgeompoint3D_seqset;
SELECT MAX(array_length(sequences(ss),1)) FROM tbl_tgeogpoint3D_seqset;

SELECT MAX(numInstants(temp)) FROM tbl_tgeompoint;
SELECT MAX(numInstants(temp)) FROM tbl_tgeogpoint;
SELECT MAX(numInstants(temp)) FROM tbl_tgeompoint3D;
SELECT MAX(numInstants(temp)) FROM tbl_tgeogpoint3D;

SELECT COUNT(startInstant(temp)) FROM tbl_tgeompoint;
SELECT COUNT(startInstant(temp)) FROM tbl_tgeogpoint;
SELECT COUNT(startInstant(temp)) FROM tbl_tgeompoint3D;
SELECT COUNT(startInstant(temp)) FROM tbl_tgeogpoint3D;

SELECT COUNT(endInstant(temp)) FROM tbl_tgeompoint;
SELECT COUNT(endInstant(temp)) FROM tbl_tgeogpoint;
SELECT COUNT(endInstant(temp)) FROM tbl_tgeompoint3D;
SELECT COUNT(endInstant(temp)) FROM tbl_tgeogpoint3D;

SELECT COUNT(instantN(temp, numInstants(temp))) FROM tbl_tgeompoint;
SELECT COUNT(instantN(temp, numInstants(temp))) FROM tbl_tgeogpoint;
SELECT COUNT(instantN(temp, numInstants(temp))) FROM tbl_tgeompoint3D;
SELECT COUNT(instantN(temp, numInstants(temp))) FROM tbl_tgeogpoint3D;

SELECT MAX(array_length(instants(temp),1)) FROM tbl_tgeompoint;
SELECT MAX(array_length(instants(temp),1)) FROM tbl_tgeogpoint;
SELECT MAX(array_length(instants(temp),1)) FROM tbl_tgeompoint3D;
SELECT MAX(array_length(instants(temp),1)) FROM tbl_tgeogpoint3D;

SELECT MAX(numTimestamps(temp)) FROM tbl_tgeompoint;
SELECT MAX(numTimestamps(temp)) FROM tbl_tgeogpoint;
SELECT MAX(numTimestamps(temp)) FROM tbl_tgeompoint3D;
SELECT MAX(numTimestamps(temp)) FROM tbl_tgeogpoint3D;

SELECT MAX(startTimestamp(temp)) FROM tbl_tgeompoint;
SELECT MAX(startTimestamp(temp)) FROM tbl_tgeogpoint;
SELECT MAX(startTimestamp(temp)) FROM tbl_tgeompoint3D;
SELECT MAX(startTimestamp(temp)) FROM tbl_tgeogpoint3D;

SELECT MAX(endTimestamp(temp)) FROM tbl_tgeompoint;
SELECT MAX(endTimestamp(temp)) FROM tbl_tgeogpoint;
SELECT MAX(endTimestamp(temp)) FROM tbl_tgeompoint3D;
SELECT MAX(endTimestamp(temp)) FROM tbl_tgeogpoint3D;

SELECT MAX(timestampN(temp, numTimestamps(temp))) FROM tbl_tgeompoint;
SELECT MAX(timestampN(temp, numTimestamps(temp))) FROM tbl_tgeogpoint;
SELECT MAX(timestampN(temp, numTimestamps(temp))) FROM tbl_tgeompoint3D;
SELECT MAX(timestampN(temp, numTimestamps(temp))) FROM tbl_tgeogpoint3D;

SELECT MAX(array_length(timestamps(temp),1)) FROM tbl_tgeompoint;
SELECT MAX(array_length(timestamps(temp),1)) FROM tbl_tgeogpoint;
SELECT MAX(array_length(timestamps(temp),1)) FROM tbl_tgeompoint3D;
SELECT MAX(array_length(timestamps(temp),1)) FROM tbl_tgeogpoint3D;

-------------------------------------------------------------------------------
-- Shift and scaleTime functions
-------------------------------------------------------------------------------

SELECT COUNT(shiftTime(temp, i)) FROM tbl_tgeompoint, tbl_interval;
SELECT COUNT(shiftTime(temp, i)) FROM tbl_tgeogpoint, tbl_interval;
SELECT COUNT(shiftTime(temp, i)) FROM tbl_tgeompoint3D, tbl_interval;
SELECT COUNT(shiftTime(temp, i)) FROM tbl_tgeogpoint3D, tbl_interval;

SELECT COUNT(scaleTime(temp, i)) FROM tbl_tgeompoint, tbl_interval;
SELECT COUNT(scaleTime(temp, i)) FROM tbl_tgeogpoint, tbl_interval;
SELECT COUNT(scaleTime(temp, i)) FROM tbl_tgeompoint3D, tbl_interval;
SELECT COUNT(scaleTime(temp, i)) FROM tbl_tgeogpoint3D, tbl_interval;

SELECT COUNT(shiftScaleTime(temp, i, i)) FROM tbl_tgeompoint, tbl_interval;
SELECT COUNT(shiftScaleTime(temp, i, i)) FROM tbl_tgeogpoint, tbl_interval;
SELECT COUNT(shiftScaleTime(temp, i, i)) FROM tbl_tgeompoint3D, tbl_interval;
SELECT COUNT(shiftScaleTime(temp, i, i)) FROM tbl_tgeogpoint3D, tbl_interval;

-------------------------------------------------------------------------------
-- Granularity modification with tprecision and tsample

SELECT MAX(startTimestamp(tprecision(inst, '15 minutes'))) FROM tbl_tgeompoint_inst;
SELECT MAX(startTimestamp(tprecision(ti, '15 minutes'))) FROM tbl_tgeompoint_discseq;
SELECT MAX(startTimestamp(tprecision(seq, '15 minutes'))) FROM tbl_tgeompoint_seq;
SELECT MAX(startTimestamp(tprecision(ss, '15 minutes'))) FROM tbl_tgeompoint_seqset;

SELECT MAX(startTimestamp(tsample(inst, '15 minutes'))) FROM tbl_tgeompoint_inst;
SELECT MAX(startTimestamp(tsample(ti, '15 minutes'))) FROM tbl_tgeompoint_discseq;
SELECT MAX(startTimestamp(tsample(seq, '15 minutes'))) FROM tbl_tgeompoint_seq;
SELECT MAX(startTimestamp(tsample(ss, '15 minutes'))) FROM tbl_tgeompoint_seqset;

SELECT MAX(numInstants(tsample(inst, '15 minutes', interp := 'step'))) FROM tbl_tgeompoint_inst;
SELECT MAX(numInstants(tsample(ti, '15 minutes', interp := 'step'))) FROM tbl_tgeompoint_discseq;
SELECT MAX(numInstants(tsample(seq, '15 minutes', interp := 'step'))) FROM tbl_tgeompoint_seq;
SELECT MAX(numInstants(tsample(ss, '15 minutes', interp := 'step'))) FROM tbl_tgeompoint_seqset;

SELECT MAX(numInstants(tsample(inst, '15 minutes', interp := 'linear'))) FROM tbl_tgeompoint_inst;
SELECT MAX(numInstants(tsample(ti, '15 minutes', interp := 'linear'))) FROM tbl_tgeompoint_discseq;
SELECT MAX(numInstants(tsample(seq, '15 minutes', interp := 'linear'))) FROM tbl_tgeompoint_seq;
SELECT MAX(numInstants(tsample(ss, '15 minutes', interp := 'linear'))) FROM tbl_tgeompoint_seqset;

-------------------------------------------------------------------------------
-- Stop function

SELECT MAX(numInstants(stops(seq, 50.0))) FROM tbl_tgeompoint_seq;
SELECT MAX(numInstants(stops(seq, 50.0))) FROM tbl_tgeogpoint_seq;

SELECT MAX(numInstants(stops(seq, 50.0, '1 min'))) FROM tbl_tgeompoint_seq;
SELECT MAX(numInstants(stops(seq, 50.0, '1 min'))) FROM tbl_tgeogpoint_seq;

SELECT MAX(numInstants(stops(ss, 10.0, '1 min'))) FROM tbl_tgeompoint_seqset;
SELECT MAX(numInstants(stops(ss, 10.0, '1 min'))) FROM tbl_tgeogpoint_seqset;

-------------------------------------------------------------------------------
-- Ever/always comparison functions
-------------------------------------------------------------------------------

SELECT COUNT(*) FROM tbl_tgeompoint WHERE temp ?= startValue(temp);
SELECT COUNT(*) FROM tbl_tgeogpoint WHERE temp ?= startValue(temp);
SELECT COUNT(*) FROM tbl_tgeompoint3D WHERE temp ?= startValue(temp);
SELECT COUNT(*) FROM tbl_tgeogpoint3D WHERE temp ?= startValue(temp);

SELECT COUNT(*) FROM tbl_tgeompoint WHERE temp %= startValue(temp);
SELECT COUNT(*) FROM tbl_tgeogpoint WHERE temp %= startValue(temp);
SELECT COUNT(*) FROM tbl_tgeompoint3D WHERE temp %= startValue(temp);
SELECT COUNT(*) FROM tbl_tgeogpoint3D WHERE temp %= startValue(temp);

------------------------------------------------------------------------------
-- Restriction functions
------------------------------------------------------------------------------

SELECT COUNT(*) FROM tbl_tgeompoint, tbl_geom_point WHERE temp != merge(atValues(temp, g), minusValues(temp, g));
SELECT COUNT(*) FROM tbl_tgeogpoint, tbl_geog_point WHERE temp != merge(atValues(temp, g), minusValues(temp, g));
SELECT COUNT(*) FROM tbl_tgeompoint3D, tbl_geom_point3D WHERE temp != merge(atValues(temp, g), minusValues(temp, g));
SELECT COUNT(*) FROM tbl_tgeogpoint3D, tbl_geog_point3D WHERE temp != merge(atValues(temp, g), minusValues(temp, g));

SELECT COUNT(*) FROM tbl_tgeompoint, (
  SELECT set(array_agg(g)) AS s FROM tbl_geom_point WHERE g IS NOT NULL AND NOT ST_IsEmpty(g)) tmp
WHERE temp != merge(atValues(temp, s), minusValues(temp, s));
SELECT COUNT(*) FROM tbl_tgeogpoint, (
  SELECT set(array_agg(g)) AS s FROM tbl_geog_point WHERE g IS NOT NULL AND NOT ST_IsEmpty(g::geometry)) tmp
WHERE temp != merge(atValues(temp, s), minusValues(temp, s));
SELECT COUNT(*) FROM tbl_tgeompoint3D, (
  SELECT set(array_agg(g)) AS s FROM tbl_geom_point3D WHERE g IS NOT NULL AND NOT ST_IsEmpty(g)) tmp
WHERE temp != merge(atValues(temp, s), minusValues(temp, s));
SELECT COUNT(*) FROM tbl_tgeogpoint3D, (
  SELECT set(array_agg(g)) AS s FROM tbl_geog_point3D WHERE g IS NOT NULL AND NOT ST_IsEmpty(g::geometry)) tmp
WHERE temp != merge(atValues(temp, s), minusValues(temp, s));

SELECT COUNT(*) FROM tbl_tgeompoint, tbl_timestamptz WHERE valueAtTimestamp(temp, t) IS NOT NULL;
SELECT COUNT(*) FROM tbl_tgeogpoint, tbl_timestamptz WHERE valueAtTimestamp(temp, t) IS NOT NULL;
SELECT COUNT(*) FROM tbl_tgeompoint3D, tbl_timestamptz WHERE valueAtTimestamp(temp, t) IS NOT NULL;
SELECT COUNT(*) FROM tbl_tgeogpoint3D, tbl_timestamptz WHERE valueAtTimestamp(temp, t) IS NOT NULL;

SELECT COUNT(*) FROM tbl_tgeompoint, tbl_timestamptz WHERE temp != merge(atTime(temp, t), minusTime(temp, t));
SELECT COUNT(*) FROM tbl_tgeogpoint, tbl_timestamptz WHERE temp != merge(atTime(temp, t), minusTime(temp, t));
SELECT COUNT(*) FROM tbl_tgeompoint3D, tbl_timestamptz WHERE temp != merge(atTime(temp, t), minusTime(temp, t));
SELECT COUNT(*) FROM tbl_tgeogpoint3D, tbl_timestamptz WHERE temp != merge(atTime(temp, t), minusTime(temp, t));

SELECT COUNT(*) FROM tbl_tgeompoint, tbl_tstzset WHERE temp != merge(atTime(temp, t), minusTime(temp, t));
SELECT COUNT(*) FROM tbl_tgeogpoint, tbl_tstzset WHERE temp != merge(atTime(temp, t), minusTime(temp, t));
SELECT COUNT(*) FROM tbl_tgeompoint3D, tbl_tstzset WHERE temp != merge(atTime(temp, t), minusTime(temp, t));
SELECT COUNT(*) FROM tbl_tgeogpoint3D, tbl_tstzset WHERE temp != merge(atTime(temp, t), minusTime(temp, t));

SELECT COUNT(*) FROM tbl_tgeompoint, tbl_tstzspan WHERE temp != merge(atTime(temp, t), minusTime(temp, t));
SELECT COUNT(*) FROM tbl_tgeogpoint, tbl_tstzspan WHERE temp != merge(atTime(temp, t), minusTime(temp, t));
SELECT COUNT(*) FROM tbl_tgeompoint3D, tbl_tstzspan WHERE temp != merge(atTime(temp, t), minusTime(temp, t));
SELECT COUNT(*) FROM tbl_tgeogpoint3D, tbl_tstzspan WHERE temp != merge(atTime(temp, t), minusTime(temp, t));

SELECT COUNT(*) FROM tbl_tgeompoint, tbl_tstzspanset WHERE temp != merge(atTime(temp, t), minusTime(temp, t));
SELECT COUNT(*) FROM tbl_tgeogpoint, tbl_tstzspanset WHERE temp != merge(atTime(temp, t), minusTime(temp, t));
SELECT COUNT(*) FROM tbl_tgeompoint3D, tbl_tstzspanset WHERE temp != merge(atTime(temp, t), minusTime(temp, t));
SELECT COUNT(*) FROM tbl_tgeogpoint3D, tbl_tstzspanset WHERE temp != merge(atTime(temp, t), minusTime(temp, t));

-------------------------------------------------------------------------------
-- Modification functions
-------------------------------------------------------------------------------

-- Update calls the insert function after calling the minusTime function
SELECT SUM(numInstants(update(t1.temp, t2.temp))) FROM tbl_tgeompoint t1, tbl_tgeompoint t2 WHERE t1.k < t2.k;
SELECT SUM(numInstants(update(t1.temp, t2.temp))) FROM tbl_tgeogpoint t1, tbl_tgeogpoint t2 WHERE t1.k < t2.k;

------------------------------------------------------------------------------
-- Local aggregate functions
------------------------------------------------------------------------------

SELECT MAX(ST_memsize(twCentroid(temp))) FROM tbl_tgeompoint;
SELECT MAX(ST_memsize(twCentroid(temp))) FROM tbl_tgeompoint3D;

------------------------------------------------------------------------------
-- Comparison functions and B-tree indexing
------------------------------------------------------------------------------

SELECT COUNT(*) FROM tbl_tgeompoint t1, tbl_tgeompoint t2
WHERE t1.temp = t2.temp;
SELECT COUNT(*) FROM tbl_tgeompoint t1, tbl_tgeompoint t2
WHERE t1.temp <> t2.temp;
SELECT COUNT(*) FROM tbl_tgeompoint t1, tbl_tgeompoint t2
WHERE t1.temp < t2.temp;
SELECT COUNT(*) FROM tbl_tgeompoint t1, tbl_tgeompoint t2
WHERE t1.temp <= t2.temp;
SELECT COUNT(*) FROM tbl_tgeompoint t1, tbl_tgeompoint t2
WHERE t1.temp > t2.temp;
SELECT COUNT(*) FROM tbl_tgeompoint t1, tbl_tgeompoint t2
WHERE t1.temp >= t2.temp;

SELECT COUNT(*) FROM tbl_tgeogpoint t1, tbl_tgeogpoint t2
WHERE t1.temp = t2.temp;
SELECT COUNT(*) FROM tbl_tgeogpoint t1, tbl_tgeogpoint t2
WHERE t1.temp <> t2.temp;
SELECT COUNT(*) FROM tbl_tgeogpoint t1, tbl_tgeogpoint t2
WHERE t1.temp < t2.temp;
SELECT COUNT(*) FROM tbl_tgeogpoint t1, tbl_tgeogpoint t2
WHERE t1.temp <= t2.temp;
SELECT COUNT(*) FROM tbl_tgeogpoint t1, tbl_tgeogpoint t2
WHERE t1.temp > t2.temp;
SELECT COUNT(*) FROM tbl_tgeogpoint t1, tbl_tgeogpoint t2
WHERE t1.temp >= t2.temp;

SELECT COUNT(*) FROM tbl_tgeompoint3D t1, tbl_tgeompoint3D t2
WHERE t1.temp = t2.temp;
SELECT COUNT(*) FROM tbl_tgeompoint3D t1, tbl_tgeompoint3D t2
WHERE t1.temp <> t2.temp;
SELECT COUNT(*) FROM tbl_tgeompoint3D t1, tbl_tgeompoint3D t2
WHERE t1.temp < t2.temp;
SELECT COUNT(*) FROM tbl_tgeompoint3D t1, tbl_tgeompoint3D t2
WHERE t1.temp <= t2.temp;
SELECT COUNT(*) FROM tbl_tgeompoint3D t1, tbl_tgeompoint3D t2
WHERE t1.temp > t2.temp;
SELECT COUNT(*) FROM tbl_tgeompoint3D t1, tbl_tgeompoint3D t2
WHERE t1.temp >= t2.temp;

SELECT COUNT(*) FROM tbl_tgeogpoint3D t1, tbl_tgeogpoint3D t2
WHERE t1.temp = t2.temp;
SELECT COUNT(*) FROM tbl_tgeogpoint3D t1, tbl_tgeogpoint3D t2
WHERE t1.temp <> t2.temp;
SELECT COUNT(*) FROM tbl_tgeogpoint3D t1, tbl_tgeogpoint3D t2
WHERE t1.temp < t2.temp;
SELECT COUNT(*) FROM tbl_tgeogpoint3D t1, tbl_tgeogpoint3D t2
WHERE t1.temp <= t2.temp;
SELECT COUNT(*) FROM tbl_tgeogpoint3D t1, tbl_tgeogpoint3D t2
WHERE t1.temp > t2.temp;
SELECT COUNT(*) FROM tbl_tgeogpoint3D t1, tbl_tgeogpoint3D t2
WHERE t1.temp >= t2.temp;

-------------------------------------------------------------------------------
