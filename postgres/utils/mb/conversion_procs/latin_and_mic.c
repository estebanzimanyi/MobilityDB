/*-------------------------------------------------------------------------
 *
 *    LATINn and MULE_INTERNAL
 *
 * Portions Copyright (c) 1996-2025, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *    src/backend/utils/mb/conversion_procs/latin_and_mic/latin_and_mic.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "mb/pg_wchar.h"

/* ----------
 * conv_proc(
 *    INTEGER,  -- source encoding id
 *    INTEGER,  -- destination encoding id
 *    CSTRING,  -- source string (null terminated C string)
 *    CSTRING,  -- destination string (null terminated C string)
 *    INTEGER,  -- source string length
 *    BOOL    -- if true, don't throw an error if conversion fails
 * ) returns INTEGER;
 *
 * Returns the number of bytes successfully converted.
 * ----------
 */


Datum
latin1_to_mic(unsigned char *src, unsigned char *dest, int len, bool noError)
{
  CHECK_ENCODING_CONVERSION_ARGS(PG_LATIN1, PG_MULE_INTERNAL);
  return latin2mic(src, dest, len, LC_ISO8859_1, PG_LATIN1, noError);
}

Datum
mic_to_latin1(unsigned char *src, unsigned char *dest, int len, bool noError)
{
  CHECK_ENCODING_CONVERSION_ARGS(PG_MULE_INTERNAL, PG_LATIN1);
  return mic2latin(src, dest, len, LC_ISO8859_1, PG_LATIN1, noError);
}

Datum
latin3_to_mic(unsigned char *src, unsigned char *dest, int len, bool noError)
{
  CHECK_ENCODING_CONVERSION_ARGS(PG_LATIN3, PG_MULE_INTERNAL);
  return latin2mic(src, dest, len, LC_ISO8859_3, PG_LATIN3, noError);
}

Datum
mic_to_latin3(unsigned char *src, unsigned char *dest, int len, bool noError)
{
  CHECK_ENCODING_CONVERSION_ARGS(PG_MULE_INTERNAL, PG_LATIN3);
  return mic2latin(src, dest, len, LC_ISO8859_3, PG_LATIN3, noError);
}

Datum
latin4_to_mic(unsigned char *src, unsigned char *dest, int len, bool noError)
{
  CHECK_ENCODING_CONVERSION_ARGS(PG_LATIN4, PG_MULE_INTERNAL);
  return latin2mic(src, dest, len, LC_ISO8859_4, PG_LATIN4, noError);
}

Datum
mic_to_latin4(unsigned char *src, unsigned char *dest, int len, bool noError)
{
  CHECK_ENCODING_CONVERSION_ARGS(PG_MULE_INTERNAL, PG_LATIN4);
  return mic2latin(src, dest, len, LC_ISO8859_4, PG_LATIN4, noError);
}
