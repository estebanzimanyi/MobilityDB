/*-------------------------------------------------------------------------
 *
 * json.c
 *    JSON data type support.
 *
 * Portions Copyright (c) 1996-2025, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *    src/backend/utils/adt/json.c
 *
 *-------------------------------------------------------------------------
 */

/* C */
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
/* PostgreSQL */
#include <postgres.h>
#include <miscadmin.h>
#include "catalog/pg_type.h"
#include <common/hashfn.h>
#include <common/int.h>
#include <common/jsonapi.h>
#include <port/simd.h>
#include <utils/date.h>
#include <utils/datetime.h>
#include <utils/timestamp.h>
#include <utils/hsearch.h>
#include <utils/json.h>
#include <utils/jsonb.h>
#include <utils/jsonfuncs.h>
#include <utils/varlena.h> /* For DatumGetTextP */

#include "utils/jsonb.h"
#include <postgres_types.h>

// #include "postgres.h"
// #include "catalog/pg_proc.h"
// #include "catalog/pg_type.h"
// #include "common/hashfn.h"
// #include "funcapi.h"
// #include "libpq/pqformat.h"
// #include "miscadmin.h"
// #include "port/simd.h"
// #include "utils/array.h"
// #include "utils/builtins.h"
// #include "utils/date.h"
// #include "utils/datetime.h"
// #include "utils/fmgroids.h"
// #include "utils/json.h"
// #include "utils/jsonfuncs.h"
// #include "utils/lsyscache.h"
// #include "utils/typcache.h"

/*
 * Support for fast key uniqueness checking.
 *
 * We maintain a hash table of used keys in JSON objects for fast detection
 * of duplicates.
 */

/* Hash entry for JsonUniqueCheckState */
typedef struct JsonUniqueHashEntry
{
  const char *key;
  int      key_len;
  int      object_id;
} JsonUniqueHashEntry;

/* Common context for key uniqueness check */
typedef struct HTAB *JsonUniqueCheckState;  /* hash table for key names */

/* Stack element for key uniqueness check during JSON parsing */
typedef struct JsonUniqueStackEntry
{
  struct JsonUniqueStackEntry *parent;
  int object_id;
} JsonUniqueStackEntry;

/* Context struct for key uniqueness check during JSON parsing */
typedef struct JsonUniqueParsingState
{
  JsonLexContext *lex;
  JsonUniqueCheckState check;
  JsonUniqueStackEntry *stack;
  int id_counter;
  bool unique;
} JsonUniqueParsingState;

/* Context struct for key uniqueness check during JSON building */
typedef struct JsonUniqueBuilderState
{
  JsonUniqueCheckState check; /* unique check */
  StringInfoData skipped_keys;  /* skipped keys with NULL values */
  // MemoryContext mcxt;      /* context for saving skipped keys */
} JsonUniqueBuilderState;

/* State struct for JSON aggregation */
typedef struct JsonAggState
{
  StringInfo str;
  JsonTypeCategory key_category;
  Oid key_output_func;
  JsonTypeCategory val_category;
  Oid val_output_func;
  JsonUniqueBuilderState unique_check;
} JsonAggState;

static void composite_to_json(Datum composite, StringInfo result,
  bool use_line_feeds);
static void array_dim_to_json(StringInfo result, int dim, int ndims, int *dims,
  Datum *vals, bool *nulls, int *valcount, JsonTypeCategory tcategory,
  Oid outfuncoid, bool use_line_feeds);
static void array_to_json_internal(Datum array, StringInfo result,
  bool use_line_feeds);
// static void datum_to_json_internal(Datum val, bool is_null, StringInfo result,
  // JsonTypeCategory tcategory, Oid outfuncoid, bool key_scalar);
static void add_json(Datum val, bool is_null, StringInfo result,
  Oid val_type, bool key_scalar);
static text *catenate_stringinfo_string(StringInfo buffer, const char *addon);

/**
 * @ingroup meos_base_json
 * @brief Return a JSON value from its string representation
 * @param[in] str String
 * @note Derived from PostgreSQL function @p json_in()
 */
text *
pg_json_in(const char *str)
{
  text *result = cstring_to_text(str);

  /* validate it */
  JsonLexContext lex;
  makeJsonLexContext(&lex, result, false);
  if (! pg_parse_json_or_errsave(&lex, &nullSemAction, NULL))
    return NULL;

  /* Internal representation is the same as text */
  return result;
}

/**
 * @ingroup meos_base_json
 * @brief Return the string representation of a JSON value
 * @param[in] json JSON value
 * @note Derived from PostgreSQL function @p json_out()
 */
char *
pg_json_out(const text *json)
{
  return text_to_cstring(json);
}

#if 0 /* NOT USED */

/*
 * Turn a Datum into JSON text, appending the string to "result".
 *
 * tcategory and outfuncoid are from a previous call to json_categorize_type,
 * except that if is_null is true then they can be invalid.
 *
 * If key_scalar is true, the value is being printed as a key, so insist
 * it's of an acceptable type, and force it to be quoted.
 */
static void
datum_to_json_internal(Datum val, bool is_null, StringInfo result,
  JsonTypeCategory tcategory, Oid outfuncoid, bool key_scalar)
{
  char *outputstr;
  text *jsontext;

  /* callers are expected to ensure that null keys are not passed in */
  Assert(!(key_scalar && is_null));

  if (is_null)
  {
    appendBinaryStringInfo(result, "null", strlen("null"));
    return;
  }

  if (key_scalar &&
    (tcategory == JSONTYPE_ARRAY || tcategory == JSONTYPE_COMPOSITE ||
     tcategory == JSONTYPE_JSON || tcategory == JSONTYPE_CAST))
  {
    elog(ERROR, "key value must be scalar, not array, composite, or json");
  }

  switch (tcategory)
  {
    case JSONTYPE_ARRAY:
      array_to_json_internal(val, result, false);
      break;
    case JSONTYPE_COMPOSITE:
      composite_to_json(val, result, false);
      break;
    case JSONTYPE_BOOL:
      if (key_scalar)
        appendStringInfoChar(result, '"');
      if (DatumGetBool(val))
        appendBinaryStringInfo(result, "true", strlen("true"));
      else
        appendBinaryStringInfo(result, "false", strlen("false"));
      if (key_scalar)
        appendStringInfoChar(result, '"');
      break;
    case JSONTYPE_NUMERIC:
      outputstr = OidOutputFunctionCall(outfuncoid, val);

      /*
       * Don't quote a non-key if it's a valid JSON number (i.e., not
       * "Infinity", "-Infinity", or "NaN").  Since we know this is a
       * numeric data type's output, we simplify and open-code the
       * validation for better performance.
       */
      if (!key_scalar &&
        ((*outputstr >= '0' && *outputstr <= '9') ||
         (*outputstr == '-' && (outputstr[1] >= '0' && outputstr[1] <= '9'))))
        appendStringInfoString(result, outputstr);
      else
      {
        appendStringInfoChar(result, '"');
        appendStringInfoString(result, outputstr);
        appendStringInfoChar(result, '"');
      }
      pfree(outputstr);
      break;
    case JSONTYPE_DATE:
      {
        char buf[MAXDATELEN + 1];
        JsonEncodeDateTime(buf, val, DATEOID, NULL);
        appendStringInfoChar(result, '"');
        appendStringInfoString(result, buf);
        appendStringInfoChar(result, '"');
      }
      break;
    case JSONTYPE_TIMESTAMP:
      {
        char buf[MAXDATELEN + 1];
        JsonEncodeDateTime(buf, val, TIMESTAMPOID, NULL);
        appendStringInfoChar(result, '"');
        appendStringInfoString(result, buf);
        appendStringInfoChar(result, '"');
      }
      break;
    case JSONTYPE_TIMESTAMPTZ:
      {
        char buf[MAXDATELEN + 1];
        JsonEncodeDateTime(buf, val, TIMESTAMPTZOID, NULL);
        appendStringInfoChar(result, '"');
        appendStringInfoString(result, buf);
        appendStringInfoChar(result, '"');
      }
      break;
    case JSONTYPE_JSON:
      /* JSON and JSONB output will already be escaped */
      outputstr = OidOutputFunctionCall(outfuncoid, val);
      appendStringInfoString(result, outputstr);
      pfree(outputstr);
      break;
    case JSONTYPE_CAST:
      /* outfuncoid refers to a cast function, not an output function */
      jsontext = DatumGetTextP(OidFunctionCall1(outfuncoid, val));
      appendBinaryStringInfo(result, VARDATA_ANY(jsontext),
        VARSIZE_ANY_EXHDR(jsontext));
      pfree(jsontext);
      break;
    default:
      /* special-case text types to save useless palloc/memcpy cycles */
      if (outfuncoid == F_TEXTOUT || outfuncoid == F_VARCHAROUT ||
          outfuncoid == F_BPCHAROUT)
        escape_json_text(result, (text *) DatumGetPointer(val));
      else
      {
        outputstr = OidOutputFunctionCall(outfuncoid, val);
        escape_json(result, outputstr);
        pfree(outputstr);
      }
      break;
  }
}

#endif /* NOT USED */

/*
 * Encode 'value' of datetime type 'typid' into JSON string in ISO format using
 * optionally preallocated buffer 'buf'.  Optional 'tzp' determines time-zone
 * offset (in seconds) in which we want to show timestamptz.
 */
char *
JsonEncodeDateTime(char *buf, Datum value, Oid typid, const int *tzp)
{
  if (!buf)
    buf = palloc(MAXDATELEN + 1);

  switch (typid)
  {
    case DATEOID:
      {
        struct pg_tm tm;
        DateADT date = DatumGetDateADT(value);
        /* Same as date_out(), but forcing DateStyle */
        if (DATE_NOT_FINITE(date))
          EncodeSpecialDate(date, buf);
        else
        {
          j2date(date + POSTGRES_EPOCH_JDATE, &(tm.tm_year), &(tm.tm_mon),
            &(tm.tm_mday));
          EncodeDateOnly(&tm, USE_XSD_DATES, buf);
        }
      }
      break;
    case TIMEOID:
      {
        TimeADT time = DatumGetTimeADT(value);
        struct pg_tm tt, *tm = &tt;
        fsec_t fsec;
        /* Same as time_out(), but forcing DateStyle */
        time2tm(time, tm, &fsec);
        EncodeTimeOnly(tm, fsec, false, 0, USE_XSD_DATES, buf);
      }
      break;
    case TIMETZOID:
      {
        TimeTzADT  *time = DatumGetTimeTzADTP(value);
        struct pg_tm tt, *tm = &tt;
        fsec_t fsec;
        int tz;
        /* Same as timetz_out(), but forcing DateStyle */
        timetz2tm(time, tm, &fsec, &tz);
        EncodeTimeOnly(tm, fsec, true, tz, USE_XSD_DATES, buf);
      }
      break;
    case TIMESTAMPOID:
      {
        struct pg_tm tm;
        fsec_t fsec;
        Timestamp timestamp = DatumGetTimestamp(value);
        /* Same as timestamp_out(), but forcing DateStyle */
        if (TIMESTAMP_NOT_FINITE(timestamp))
          EncodeSpecialTimestamp(timestamp, buf);
        else if (timestamp2tm(timestamp, NULL, &tm, &fsec, NULL, NULL) == 0)
          EncodeDateTime(&tm, fsec, false, 0, NULL, USE_XSD_DATES, buf);
        else
        {
          elog(ERROR, "timestamp out of range");
          return NULL;
        }
      }
      break;
    case TIMESTAMPTZOID:
      {
        struct pg_tm tm;
        int tz;
        fsec_t fsec;
        const char *tzn = NULL;
        TimestampTz timestamp = DatumGetTimestampTz(value);

        /*
         * If a time zone is specified, we apply the time-zone shift,
         * convert timestamptz to pg_tm as if it were without a time
         * zone, and then use the specified time zone for converting
         * the timestamp into a string.
         */
        if (tzp)
        {
          tz = *tzp;
          timestamp -= (TimestampTz) tz * USECS_PER_SEC;
        }

        /* Same as timestamptz_out(), but forcing DateStyle */
        if (TIMESTAMP_NOT_FINITE(timestamp))
          EncodeSpecialTimestamp(timestamp, buf);
        else if (timestamp2tm(timestamp, tzp ? NULL : &tz, &tm, &fsec,
          tzp ? NULL : &tzn, NULL) == 0)
        {
          if (tzp)
            tm.tm_isdst = 1;  /* set time-zone presence flag */
          EncodeDateTime(&tm, fsec, true, tz, tzn, USE_XSD_DATES, buf);
        }
        else
        {
          elog(ERROR, "timestamp out of range");
          return NULL;
        }
      }
      break;
    default:
      elog(ERROR, "unknown jsonb value datetime type oid %u", typid);
      return NULL;
  }

  return buf;
}

#if 0 /* NOT USED */

/*
 * Process a single dimension of an array.
 * If it's the innermost dimension, output the values, otherwise call
 * ourselves recursively to process the next dimension.
 */
static void
array_dim_to_json(StringInfo result, int dim, int ndims, int *dims,
  Datum *vals, bool *nulls, int *valcount, JsonTypeCategory tcategory,
  Oid outfuncoid, bool use_line_feeds)
{
  Assert(dim < ndims);
  const char *sep = use_line_feeds ? ",\n " : ",";
  appendStringInfoChar(result, '[');
  for (int i = 1; i <= dims[dim]; i++)
  {
    if (i > 1)
      appendStringInfoString(result, sep);
    if (dim + 1 == ndims)
    {
      datum_to_json_internal(vals[*valcount], nulls[*valcount], result,
        tcategory, outfuncoid, false);
      (*valcount)++;
    }
    else
    {
      /*
       * Do we want line feeds on inner dimensions of arrays? For now
       * we'll say no.
       */
      array_dim_to_json(result, dim + 1, ndims, dims, vals, nulls, valcount,
        tcategory, outfuncoid, false);
    }
  }

  appendStringInfoChar(result, ']');
}

/*
 * Turn an array into JSON.
 */
static void
array_to_json_internal(Datum array, StringInfo result, bool use_line_feeds)
{
  ArrayType  *v = DatumGetArrayTypeP(array);
  Oid      element_type = ARR_ELEMTYPE(v);
  int       *dim;
  int      ndim;
  int      nitems;
  int      count = 0;
  Datum     *elements;
  bool     *nulls;
  int16    typlen;
  bool    typbyval;
  char    typalign;
  JsonTypeCategory tcategory;
  Oid      outfuncoid;

  ndim = ARR_NDIM(v);
  dim = ARR_DIMS(v);
  nitems = ArrayGetNItems(ndim, dim);

  if (nitems <= 0)
  {
    appendStringInfoString(result, "[]");
    return;
  }

  get_typlenbyvalalign(element_type, &typlen, &typbyval, &typalign);

  json_categorize_type(element_type, false, &tcategory, &outfuncoid);

  deconstruct_array(v, element_type, typlen, typbyval, typalign, &elements,
    &nulls, &nitems);

  array_dim_to_json(result, 0, ndim, dim, elements, nulls, &count, tcategory,
    outfuncoid, use_line_feeds);

  pfree(elements);
  pfree(nulls);
}

/*
 * Turn a composite / record into JSON.
 */
static void
composite_to_json(Datum composite, StringInfo result, bool use_line_feeds)
{
  Oid tupType;
  int32 tupTypmod;
  TupleDesc tupdesc;
  HeapTupleData tmptup, *tuple;
  /*
   * We can avoid expensive strlen() calls by precalculating the separator
   * length.
   */
  const char *sep = use_line_feeds ? ",\n " : ",";
  int seplen = use_line_feeds ? strlen(",\n ") : strlen(",");
  HeapTupleHeader td = DatumGetHeapTupleHeader(composite);

  /* Extract rowtype info and find a tupdesc */
  tupType = HeapTupleHeaderGetTypeId(td);
  tupTypmod = HeapTupleHeaderGetTypMod(td);
  tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);

  /* Build a temporary HeapTuple control structure */
  tmptup.t_len = HeapTupleHeaderGetDatumLength(td);
  tmptup.t_data = td;
  tuple = &tmptup;
  appendStringInfoChar(result, '{');
  bool needsep = false;
  for (int i = 0; i < tupdesc->natts; i++)
  {
    Datum val;
    bool isnull;
    char *attname;
    JsonTypeCategory tcategory;
    Oid outfuncoid;
    Form_pg_attribute att = TupleDescAttr(tupdesc, i);

    if (att->attisdropped)
      continue;

    if (needsep)
      appendBinaryStringInfo(result, sep, seplen);
    needsep = true;

    attname = NameStr(att->attname);
    escape_json(result, attname);
    appendStringInfoChar(result, ':');

    val = heap_getattr(tuple, i + 1, tupdesc, &isnull);

    if (isnull)
    {
      tcategory = JSONTYPE_NULL;
      outfuncoid = InvalidOid;
    }
    else
      json_categorize_type(att->atttypid, false, &tcategory, &outfuncoid);

    datum_to_json_internal(val, isnull, result, tcategory, outfuncoid, false);
  }

  appendStringInfoChar(result, '}');
  ReleaseTupleDesc(tupdesc);
}

/*
 * Append JSON text for "val" to "result".
 *
 * This is just a thin wrapper around datum_to_json.  If the same type will be
 * printed many times, avoid using this; better to do the json_categorize_type
 * lookups only once.
 */
static void
add_json(Datum val, bool is_null, StringInfo result, Oid val_type,
  bool key_scalar)
{
  if (val_type == InvalidOid)
  {
    elog(ERROR, "could not determine input data type");
    return;
  }

  JsonTypeCategory tcategory;
  Oid outfuncoid;
  if (is_null)
  {
    tcategory = JSONTYPE_NULL;
    outfuncoid = InvalidOid;
  }
  else
    json_categorize_type(val_type, false, &tcategory, &outfuncoid);

  datum_to_json_internal(val, is_null, result, tcategory, outfuncoid,
    key_scalar);
}

/*
 * SQL function array_to_json(row)
 */
text *
array_to_json_internal(Datum array)
{
  StringInfo result = makeStringInfo();
  array_to_json_internal(array, result, false);
  return cstring_to_text_with_len(result->data, result->len);
}

/*
 * SQL function array_to_json(row, prettybool)
 */
text *
array_to_json_pretty_internal(Datum array)
{
  bool use_line_feeds = PG_GETARG_BOOL(1);
  StringInfo result = makeStringInfo();
  array_to_json_internal(array, result, use_line_feeds);
  return cstring_to_text_with_len(result->data, result->len);
}

/*
 * SQL function row_to_json(row)
 */
Datum
row_to_json_internal(Datum array)
{
  StringInfo result = makeStringInfo();
  composite_to_json(array, result, false);
  return cstring_to_text_with_len(result->data, result->len);
}

/*
 * SQL function row_to_json(row, prettybool)
 */
Datum
row_to_json_pretty_internal(Datum array, bool use_line_feeds)
{
  StringInfo result = makeStringInfo();
  composite_to_json(array, result, use_line_feeds);
  return cstring_to_text_with_len(result->data, result->len);
}

/*
 * Is the given type immutable when coming out of a JSON context?
 *
 * At present, datetimes are all considered mutable, because they
 * depend on timezone.  XXX we should also drill down into objects
 * and arrays, but do not.
 */
bool
to_json_is_immutable(Oid typoid)
{
  JsonTypeCategory tcategory;
  Oid outfuncoid;
  json_categorize_type(typoid, false, &tcategory, &outfuncoid);

  switch (tcategory)
  {
    case JSONTYPE_BOOL:
    case JSONTYPE_JSON:
    case JSONTYPE_JSONB:
    case JSONTYPE_NULL:
      return true;

    case JSONTYPE_DATE:
    case JSONTYPE_TIMESTAMP:
    case JSONTYPE_TIMESTAMPTZ:
      return false;

    case JSONTYPE_ARRAY:
      return false;    /* TODO recurse into elements */

    case JSONTYPE_COMPOSITE:
      return false;    /* TODO recurse into fields */

    case JSONTYPE_NUMERIC:
    case JSONTYPE_CAST:
    case JSONTYPE_OTHER:
      return func_volatile(outfuncoid) == PROVOLATILE_IMMUTABLE;
  }

  return false;        /* not reached */
}

/*
 * SQL function to_json(anyvalue)
 */
text *
to_json_internal(Datum val, Oid val_type)
{
  JsonTypeCategory tcategory;
  Oid outfuncoid;

  if (val_type == InvalidOid)
  {
    elog(ERROR, "could not determine input data type");
    return NULL;
  }

  json_categorize_type(val_type, false, &tcategory, &outfuncoid);
  return datum_to_json(val, tcategory, outfuncoid);
}

/*
 * Turn a Datum into JSON text.
 *
 * tcategory and outfuncoid are from a previous call to json_categorize_type.
 */
text *
datum_to_json(Datum val, JsonTypeCategory tcategory, Oid outfuncoid)
{
  StringInfo result = makeStringInfo();
  datum_to_json_internal(val, false, result, tcategory, outfuncoid, false);
  return cstring_to_text_with_len(result->data, result->len);
}

/*
 * json_agg transition function
 *
 * aggregate input column as a json array value.
 */
static JsonAggState *
json_agg_transfn_worker(JsonAggState *state, Datum val, Oid arg_type,
  bool arg_null, bool absent_on_null)
{
  if (! state)
  {
    if (arg_type == InvalidOid)
    {
      elog(ERROR, "could not determine input data type");
      return NULL;
    }

    state = (JsonAggState *) palloc(sizeof(JsonAggState));
    state->str = makeStringInfo();
    appendStringInfoChar(state->str, '[');
    json_categorize_type(arg_type, false, &state->val_category,
      &state->val_output_func);
  }

  if (absent_on_null && arg_null)
    return state;

  if (state->str->len > 1)
    appendStringInfoString(state->str, ", ");

  /* fast path for NULLs */
  if (arg_null)
  {
    datum_to_json_internal((Datum) 0, true, state->str, JSONTYPE_NULL,
      InvalidOid, false);
    return state;
  }

  /* add some whitespace if structured type and not first item */
  if (!PG_ARGISNULL(0) && state->str->len > 1 &&
      (state->val_category == JSONTYPE_ARRAY ||
       state->val_category == JSONTYPE_COMPOSITE))
  {
    appendStringInfoString(state->str, "\n ");
  }

  datum_to_json_internal(val, false, state->str, state->val_category,
    state->val_output_func, false);

  /*
   * The transition type for json_agg() is declared to be "internal", which
   * is a pass-by-value type the same size as a pointer.  So we can safely
   * pass the JsonAggState pointer through nodeAgg.c's machinations.
   */
  return state;
}

/*
 * json_agg aggregate function
 */
JsonAggState *
json_agg_transfn_internal(JsonAggState *state, Datum val, Oid arg_type,
  bool arg_null)
{
  return json_agg_transfn_worker(state, val, arg_type, arg_null, false);
}

/*
 * json_agg_strict aggregate function
 */
JsonAggState *
json_agg_strict_transfn_internal(JsonAggState *state, Datum val, Oid arg_type,
  bool arg_null)
{
  return json_agg_transfn_worker(state, val, arg_type, arg_null, true);
}

/*
 * json_agg final function
 */
text *
json_agg_finalfn_internal(JsonAggState *state)
{
  /* NULL result for no rows in, as is standard with aggregates */
  if (state == NULL)
    return NULL;

  /* Else return state with appropriate array terminator added */
  return catenate_stringinfo_string(state->str, "]");
}

#endif /* NOT USED */

/* Functions implementing hash table for key uniqueness check */
static uint32
json_unique_hash(const void *key, Size keysize)
{
  const JsonUniqueHashEntry *entry = (JsonUniqueHashEntry *) key;
  uint32 hash = hash_bytes_uint32(entry->object_id);
  hash ^= hash_bytes((const unsigned char *) entry->key, entry->key_len);
  return DatumGetUInt32(hash);
}

static int
json_unique_hash_match(const void *key1, const void *key2, Size keysize)
{
  const JsonUniqueHashEntry *entry1 = (const JsonUniqueHashEntry *) key1;
  const JsonUniqueHashEntry *entry2 = (const JsonUniqueHashEntry *) key2;

  if (entry1->object_id != entry2->object_id)
    return entry1->object_id > entry2->object_id ? 1 : -1;

  if (entry1->key_len != entry2->key_len)
    return entry1->key_len > entry2->key_len ? 1 : -1;

  return strncmp(entry1->key, entry2->key, entry1->key_len);
}

/*
 * Uniqueness detection support.
 *
 * In order to detect uniqueness during building or parsing of a JSON
 * object, we maintain a hash table of key names already seen.
 */
static void
json_unique_check_init(JsonUniqueCheckState *cxt)
{
  HASHCTL ctl;
  memset(&ctl, 0, sizeof(ctl));
  ctl.keysize = sizeof(JsonUniqueHashEntry);
  ctl.entrysize = sizeof(JsonUniqueHashEntry);
  ctl.hash = json_unique_hash;
  ctl.match = json_unique_hash_match;
  *cxt = hash_create("json object hashtable", 32, &ctl,
    HASH_ELEM | HASH_FUNCTION | HASH_COMPARE);
  return;
}

static void
json_unique_builder_init(JsonUniqueBuilderState *cxt)
{
  json_unique_check_init(&cxt->check);
  cxt->skipped_keys.data = NULL;
  return;
}

static bool
json_unique_check_key(JsonUniqueCheckState *cxt, const char *key,
  int object_id)
{
  JsonUniqueHashEntry entry;
  entry.key = key;
  entry.key_len = strlen(key);
  entry.object_id = object_id;
  bool found;
  (void) hash_search(*cxt, &entry, HASH_ENTER, &found);
  return !found;
}

/*
 * On-demand initialization of a throwaway StringInfo.  This is used to
 * read a key name that we don't need to store in the output object, for
 * duplicate key detection when the value is NULL.
 */
static StringInfo
json_unique_builder_get_throwawaybuf(JsonUniqueBuilderState *cxt)
{
  StringInfo out = &cxt->skipped_keys;
  if (!out->data)
    initStringInfo(out);
  else
    /* Just reset the string to empty */
    out->len = 0;
  return out;
}

#if 0 /* NOT USED */

/*
 * json_object_agg transition function.
 *
 * aggregate two input columns as a single json object value.
 */
static JsonAggState *
json_object_agg_transfn_worker(JsonAggState *state, Datum arg1, Oid arg_type1,
  bool arg1_null, Datum arg2, Oid arg_type2, bool arg2_null,
  bool absent_on_null, bool unique_keys)
{
  StringInfo out;
  bool skip;
  int key_offset;

  if (! state)
  {
    /*
     * Make the StringInfo in a context where it will persist for the
     * duration of the aggregate call. Switching context is only needed
     * for this initial step, as the StringInfo and dynahash routines make
     * sure they use the right context to enlarge the object if necessary.
     */
    state = (JsonAggState *) palloc(sizeof(JsonAggState));
    state->str = makeStringInfo();
    if (unique_keys)
      json_unique_builder_init(&state->unique_check);
    else
      memset(&state->unique_check, 0, sizeof(state->unique_check));

    if (arg_type1 == InvalidOid)
    {
      elog(ERROR, "could not determine data type for argument %d", 1);
      return NULL;
    }

    json_categorize_type(arg_type1, false, &state->key_category,
      &state->key_output_func);

    if (arg_type2 == InvalidOid)
    {
      elog(ERROR, "could not determine data type for argument %d", 2);
      return NULL;
    }

    json_categorize_type(arg_type2, false, &state->val_category,
      &state->val_output_func);
    appendStringInfoString(state->str, "{ ");
  }

  /*
   * Note: since json_object_agg() is declared as taking type "any", the
   * parser will not do any type conversion on unknown-type literals (that
   * is, undecorated strings or NULLs).  Such values will arrive here as
   * type UNKNOWN, which fortunately does not matter to us, since
   * unknownout() works fine.
   */

  if (arg1_null)
  {
    elog(ERROR, "null value not allowed for object key");
    return NULL;
  }

  /* Skip null values if absent_on_null */
  skip = absent_on_null && arg2_null;

  if (skip)
  {
    /*
     * We got a NULL value and we're not storing those; if we're not
     * testing key uniqueness, we're done.  If we are, use the throwaway
     * buffer to store the key name so that we can check it.
     */
    if (!unique_keys)
      return state;

    out = json_unique_builder_get_throwawaybuf(&state->unique_check);
  }
  else
  {
    out = state->str;

    /*
     * Append comma delimiter only if we have already output some fields
     * after the initial string "{ ".
     */
    if (out->len > 2)
      appendStringInfoString(out, ", ");
  }

  key_offset = out->len;
  datum_to_json_internal(arg1, false, out, state->key_category,
    state->key_output_func, true);

  if (unique_keys)
  {
    /*
     * Copy the key first, instead of pointing into the buffer. It will be
     * added to the hash table, but the buffer may get reallocated as
     * we're appending more data to it. That would invalidate pointers to
     * keys in the current buffer.
     */
    const char *key = strdup(&out->data[key_offset]);

    if (!json_unique_check_key(&state->unique_check.check, key, 0))
    {
      elog(ERROR, "duplicate JSON object key value: %s", key);
      return NULL;
    }

    if (skip)
      return state;
  }

  appendStringInfoString(state->str, " : ");

  if (arg2_null)
    arg2 = (Datum) 0;
  datum_to_json_internal(arg2, arg2_null, state->str,
    state->val_category, state->val_output_func, false);

  return state;
}

/*
 * json_object_agg aggregate function
 */
JsonAggState *
json_object_agg_transfn_internal(JsonAggState *state, Datum arg1,
  Oid arg_type1, bool arg1_null, Datum arg2, Oid arg_type2, bool arg2_null)
{
  return json_object_agg_transfn_worker(state, arg1, arg_type1, arg1_null,
    arg2, arg_type2, arg2_null, false, false);
}

/*
 * json_object_agg_strict aggregate function
 */
JsonAggState *
json_object_agg_strict_transfn_internal(JsonAggState *state, Datum arg1,
  Oid arg_type1, bool arg1_null, Datum arg2, Oid arg_type2, bool arg2_null)
{
  return json_object_agg_transfn_worker(state, arg1, arg_type1, arg1_null,
    arg2, arg_type2, arg2_null, true, false);
}

/*
 * json_object_agg_unique aggregate function
 */
JsonAggState *
json_object_agg_unique_transfn_internal(JsonAggState *state, Datum arg1,
  Oid arg_type1, bool arg1_null, Datum arg2, Oid arg_type2, bool arg2_null)
{
  return json_object_agg_transfn_worker(state, arg1, arg_type1, arg1_null,
    arg2, arg_type2, arg2_null, false, true);
}

/*
 * json_object_agg_unique_strict aggregate function
 */
JsonAggState *
json_object_agg_unique_strict_transfn_internal(JsonAggState *state, Datum arg1,
  Oid arg_type1, bool arg1_null, Datum arg2, Oid arg_type2, bool arg2_null)
{
  return json_object_agg_transfn_worker(state, arg1, arg_type1, arg1_null,
    arg2, arg_type2, arg2_null, true, true);
}

/*
 * json_object_agg final function.
 */
text *
json_object_agg_finalfn_internal(JsonAggState *state)
{
  /* NULL result for no rows in, as is standard with aggregates */
  if (state == NULL)
    return NULL;

  /* Else return state with appropriate object terminator added */
  return catenate_stringinfo_string(state->str, " }");
}

/*
 * Helper function for aggregates: return given StringInfo's contents plus
 * specified trailing string, as a text datum.  We need this because aggregate
 * final functions are not allowed to modify the aggregate state.
 */
static text *
catenate_stringinfo_string(StringInfo buffer, const char *addon)
{
  /* custom version of cstring_to_text_with_len */
  int buflen = buffer->len;
  int addlen = strlen(addon);
  text *result = (text *) palloc(buflen + addlen + VARHDRSZ);

  SET_VARSIZE(result, buflen + addlen + VARHDRSZ);
  memcpy(VARDATA(result), buffer->data, buflen);
  memcpy(VARDATA(result) + buflen, addon, addlen);

  return result;
}

text *
json_build_object_worker(int nargs, const Datum *args, const bool *nulls,
  const Oid *types, bool absent_on_null, bool unique_keys)
{
  if (nargs % 2 != 0)
  {
    elog(ERROR, "argument list must have even number of elements");
    return NULL;
  }

  JsonUniqueBuilderState unique_check;
  StringInfo result = makeStringInfo();
  appendStringInfoChar(result, '{');
  if (unique_keys)
    json_unique_builder_init(&unique_check);

  const char *sep = "";
  for (int i = 0; i < nargs; i += 2)
  {
    StringInfo out;

    /* Skip null values if absent_on_null */
    bool skip = absent_on_null && nulls[i + 1];
    if (skip)
    {
      /* If key uniqueness check is needed we must save skipped keys */
      if (!unique_keys)
        continue;
      out = json_unique_builder_get_throwawaybuf(&unique_check);
    }
    else
    {
      appendStringInfoString(result, sep);
      sep = ", ";
      out = result;
    }

    /* process key */
    if (nulls[i])
    {
      elog(ERROR, "null value not allowed for object key");
      return NULL;
    }

    /* save key offset before appending it */
    int key_offset = out->len;
    add_json(args[i], false, out, types[i], true);
    if (unique_keys)
    {
      /*
       * check key uniqueness after key appending
       *
       * Copy the key first, instead of pointing into the buffer. It
       * will be added to the hash table, but the buffer may get
       * reallocated as we're appending more data to it. That would
       * invalidate pointers to keys in the current buffer.
       */
      const char *key = pstrdup(&out->data[key_offset]);
      if (!json_unique_check_key(&unique_check.check, key, 0))
      {
        elog(ERROR, "duplicate JSON object key value: %s", key);
        return NULL;
      }

      if (skip)
        continue;
    }

    appendStringInfoString(result, " : ");

    /* process value */
    add_json(args[i + 1], nulls[i + 1], result, types[i + 1], false);
  }

  appendStringInfoChar(result, '}');
  return cstring_to_text_with_len(result->data, result->len);
}

/*
 * SQL function json_build_object(variadic "any")
 */
text *
json_build_object_internal(int nargs, const Datum *args, const bool *nulls,
  const Oid *types)
{
  if (nargs < 0)
    return NULL;
  return json_build_object_worker(nargs, args, nulls, types, false, false);
}

/*
 * degenerate case of json_build_object where it gets 0 arguments.
 */
text *
json_build_object_noargs_internal(XX)
{
  return cstring_to_text_with_len("{}", 2);
}

text *
json_build_array_worker(int nargs, const Datum *args, const bool *nulls,
  const Oid *types, bool absent_on_null)
{
  const char *sep = "";
  StringInfo result = makeStringInfo();
  appendStringInfoChar(result, '[');

  for (int i = 0; i < nargs; i++)
  {
    if (absent_on_null && nulls[i])
      continue;

    appendStringInfoString(result, sep);
    sep = ", ";
    add_json(args[i], nulls[i], result, types[i], false);
  }

  appendStringInfoChar(result, ']');

  return cstring_to_text_with_len(result->data, result->len);
}

/*
 * SQL function json_build_array(variadic "any")
 */
text *
json_build_array_internal(int nargs, const Datum *args, const bool *nulls,
  const Oid *types)
{
  if (nargs < 0)
    return NULL;
  return json_build_array_worker(nargs, args, nulls, types, false);
}

/*
 * degenerate case of json_build_array where it gets 0 arguments.
 */
text *
json_build_array_noargs_internal(XX)
{
  return cstring_to_text_with_len("[]", 2);
}

#endif /* NOT USED */

/**
 * @ingroup meos_base_json
 * @brief Return a JSON value constructed from an array of alternating keys
 * and values
 * @param[in] keys_vals Array of alternating keys and vals 
 * @param[in] count Number of elements in the input array 
 * @note Derived from PostgreSQL function @p json_object()
 */
text *
pg_json_object(text **keys_vals, int count)
{
  StringInfoData res;
  int count1 = count / 2;
  initStringInfo(&res);
  appendStringInfoChar(&res, '{');
  for (int i = 0; i < count1; ++i)
  {
    if (! keys_vals[i * 2])
    {
      elog(ERROR, "null value not allowed for object key");
      return NULL;
    }

    if (i > 0)
      appendStringInfoString(&res, ", ");
    escape_json_text(&res, keys_vals[i * 2]);
    appendStringInfoString(&res, " : ");
    if (! keys_vals[i * 2 + 1])
      appendStringInfoString(&res, "null");
    else
      escape_json_text(&res, keys_vals[i * 2 + 1]);
  }
  appendStringInfoChar(&res, '}');
  text *result = cstring_to_text_with_len(res.data, res.len);
  pfree(res.data);
  return result;
}

/**
 * @ingroup meos_base_json
 * @brief Return a JSON value constructed from separate key and value arrays
 * of text values
 * @param[in] keys Keys
 * @param[in] values Keys
 * @param[in] count Number of elements in the input arrays
 * @note Derived from PostgreSQL function @p json_object_two_arg()
 */
text *
pg_json_object_two_arg(text **keys, text **values, int count)
{
  StringInfoData res;
  initStringInfo(&res);
  appendStringInfoChar(&res, '{');
  for (int i = 0; i < count; ++i)
  {
    if (! keys[i])
    {
      elog(ERROR, "null value not allowed for object key");
      return NULL;
    }

    if (i > 0)
      appendStringInfoString(&res, ", ");
    escape_json_text(&res, keys[i]);
    appendStringInfoString(&res, " : ");
    if (! values[i])
      appendStringInfoString(&res, "null");
    else
      escape_json_text(&res, values[i]);
  }

  appendStringInfoChar(&res, '}');
  text *result = cstring_to_text_with_len(res.data, res.len);
  pfree(res.data);
  return result;
}

/*
 * escape_json_char
 *    Inline helper function for escape_json* functions
 */
static pg_attribute_always_inline void
escape_json_char(StringInfo buf, char c)
{
  switch (c)
  {
    case '\b':
      appendStringInfoString(buf, "\\b");
      break;
    case '\f':
      appendStringInfoString(buf, "\\f");
      break;
    case '\n':
      appendStringInfoString(buf, "\\n");
      break;
    case '\r':
      appendStringInfoString(buf, "\\r");
      break;
    case '\t':
      appendStringInfoString(buf, "\\t");
      break;
    case '"':
      appendStringInfoString(buf, "\\\"");
      break;
    case '\\':
      appendStringInfoString(buf, "\\\\");
      break;
    default:
      if ((unsigned char) c < ' ')
        appendStringInfo(buf, "\\u%04x", (int) c);
      else
        appendStringInfoCharMacro(buf, c);
      break;
  }
}

/*
 * escape_json
 *    Produce a JSON string literal, properly escaping the NUL-terminated
 *    cstring.
 */
void
escape_json(StringInfo buf, const char *str)
{
  appendStringInfoCharMacro(buf, '"');
  for (; *str != '\0'; str++)
    escape_json_char(buf, *str);
  appendStringInfoCharMacro(buf, '"');
}

/*
 * Define the number of bytes that escape_json_with_len will look ahead in the
 * input string before flushing the input string to the destination buffer.
 * Looking ahead too far could result in cachelines being evicted that will
 * need to be reloaded in order to perform the appendBinaryStringInfo call.
 * Smaller values will result in a larger number of calls to
 * appendBinaryStringInfo and introduce additional function call overhead.
 * Values larger than the size of L1d cache will likely result in worse
 * performance.
 */
#define ESCAPE_JSON_FLUSH_AFTER 512

/*
 * escape_json_with_len
 *    Produce a JSON string literal, properly escaping the possibly not
 *    NUL-terminated characters in 'str'.  'len' defines the number of bytes
 *    from 'str' to process.
 */
void
escape_json_with_len(StringInfo buf, const char *str, int len)
{
  int vlen;
  Assert(len >= 0);

  /*
   * Since we know the minimum length we'll need to append, let's just
   * enlarge the buffer now rather than incrementally making more space when
   * we run out.  Add two extra bytes for the enclosing quotes.
   */
  enlargeStringInfo(buf, len + 2);

  /*
   * Figure out how many bytes to process using SIMD.  Round 'len' down to
   * the previous multiple of sizeof(Vector8), assuming that's a power-of-2.
   */
  vlen = len & (int) (~(sizeof(Vector8) - 1));
  appendStringInfoCharMacro(buf, '"');
  for (int i = 0, copypos = 0;;)
  {
    /*
     * To speed this up, try searching sizeof(Vector8) bytes at once for
     * special characters that we need to escape.  When we find one, we
     * fall out of the Vector8 loop and copy the portion we've vector
     * searched and then we process sizeof(Vector8) bytes one byte at a
     * time.  Once done, come back and try doing vector searching again.
     * We'll also process any remaining bytes at the tail end of the
     * string byte-by-byte.  This optimization assumes that most chunks of
     * sizeof(Vector8) bytes won't contain any special characters.
     */
    for (; i < vlen; i += sizeof(Vector8))
    {
      Vector8 chunk;
      vector8_load(&chunk, (const uint8 *) &str[i]);

      /*
       * Break on anything less than ' ' or if we find a '"' or '\\'.
       * Those need special handling.  That's done in the per-byte loop.
       */
      if (vector8_has_le(chunk, (unsigned char) 0x1F) ||
        vector8_has(chunk, (unsigned char) '"') ||
        vector8_has(chunk, (unsigned char) '\\'))
        break;

#ifdef ESCAPE_JSON_FLUSH_AFTER

      /*
       * Flush what's been checked so far out to the destination buffer
       * every so often to avoid having to re-read cachelines when
       * escaping large strings.
       */
      if (i - copypos >= ESCAPE_JSON_FLUSH_AFTER)
      {
        appendBinaryStringInfo(buf, &str[copypos], i - copypos);
        copypos = i;
      }
#endif
    }

    /*
     * Write to the destination up to the point that we've vector searched
     * so far.  Do this only when switching into per-byte mode rather than
     * once every sizeof(Vector8) bytes.
     */
    if (copypos < i)
    {
      appendBinaryStringInfo(buf, &str[copypos], i - copypos);
      copypos = i;
    }

    /*
     * Per-byte loop for Vector8s containing special chars and for
     * processing the tail of the string.
     */
    for (int b = 0; b < (int) sizeof(Vector8); b++)
    {
      /* check if we've finished */
      if (i == len)
        goto done;
      Assert(i < len);
      escape_json_char(buf, str[i++]);
    }
    copypos = i;
    /* We're not done yet.  Try the vector search again. */
  }

done:
  appendStringInfoCharMacro(buf, '"');
}

/*
 * escape_json_text
 *    Append 'txt' onto 'buf' and escape using escape_json_with_len.
 *
 * This is more efficient than calling text_to_cstring and appending the
 * result as that could require an additional palloc and memcpy.
 */
void
escape_json_text(StringInfo buf, const text *txt)
{
  int len = VARSIZE_ANY_EXHDR(txt);
  char *str = VARDATA_ANY(txt);
  escape_json_with_len(buf, str, len);
  return;
}

/* Semantic actions for key uniqueness check */
static JsonParseErrorType
json_unique_object_start(void *_state)
{
  JsonUniqueParsingState *state = _state;
  JsonUniqueStackEntry *entry;
  if (!state->unique)
    return JSON_SUCCESS;

  /* push object entry to stack */
  entry = palloc(sizeof(*entry));
  entry->object_id = state->id_counter++;
  entry->parent = state->stack;
  state->stack = entry;

  return JSON_SUCCESS;
}

static JsonParseErrorType
json_unique_object_end(void *_state)
{
  JsonUniqueParsingState *state = _state;
  JsonUniqueStackEntry *entry;
  if (!state->unique)
    return JSON_SUCCESS;

  entry = state->stack;
  state->stack = entry->parent;  /* pop object from stack */
  pfree(entry);
  return JSON_SUCCESS;
}

static JsonParseErrorType
json_unique_object_field_start(void *_state, char *field, bool isnull)
{
  JsonUniqueParsingState *state = _state;
  JsonUniqueStackEntry *entry;
  if (!state->unique)
    return JSON_SUCCESS;

  /* find key collision in the current object */
  if (json_unique_check_key(&state->check, field, state->stack->object_id))
    return JSON_SUCCESS;

  state->unique = false;

  /* pop all objects entries */
  while ((entry = state->stack))
  {
    state->stack = entry->parent;
    pfree(entry);
  }
  return JSON_SUCCESS;
}

/* Validate JSON text and additionally check key uniqueness */
bool
json_validate(text *json, bool check_unique_keys, bool throw_error)
{
  JsonLexContext lex;
  JsonSemAction uniqueSemAction = {0};
  JsonUniqueParsingState state;
  JsonParseErrorType result;
  makeJsonLexContext(&lex, json, check_unique_keys);

  if (check_unique_keys)
  {
    state.lex = &lex;
    state.stack = NULL;
    state.id_counter = 0;
    state.unique = true;
    json_unique_check_init(&state.check);

    uniqueSemAction.semstate = &state;
    uniqueSemAction.object_start = json_unique_object_start;
    uniqueSemAction.object_field_start = json_unique_object_field_start;
    uniqueSemAction.object_end = json_unique_object_end;
  }

  result = pg_parse_json(&lex, check_unique_keys ? &uniqueSemAction : &nullSemAction);

  if (result != JSON_SUCCESS)
  {
    if (throw_error)
      json_errsave_error(result, &lex, NULL);
    return false;      /* invalid json */
  }

  if (check_unique_keys && !state.unique)
  {
    if (throw_error)
    {
      elog(ERROR, "duplicate JSON object key value");
    }
    return false;      /* not unique keys */
  }

  if (check_unique_keys)
    freeJsonLexContext(&lex);

  return true;        /* ok */
}

/**
 * @ingroup meos_base_json
 * @brief Returns the type of the outermost JSON value as `text`
 * @details Possible types are "object", "array", "string", "number",
 * "boolean", and "null".
 * @details Performs a single call to json_lex() to get the first token of the
 * supplied value.  This initial token uniquely determines the value's type.
 * As our input must already have been validated by json_in() or json_recv(),
 * theinitial token should never be JSON_TOKEN_OBJECT_END,
 * JSON_TOKEN_ARRAY_END, JSON_TOKEN_COLON, JSON_TOKEN_COMMA, or JSON_TOKEN_END.
 * @param[in] json JSON value 
 * @note Derived from PostgreSQL function @p json_typeof()
 */
text *
pg_json_typeof(text *json)
{
  JsonLexContext lex;
  char *type;
  JsonParseErrorType result;

  /* Lex exactly one token from the input and check its type. */
  makeJsonLexContext(&lex, json, false);
  result = json_lex(&lex);
  if (result != JSON_SUCCESS)
    json_errsave_error(result, &lex, NULL);

  switch (lex.token_type)
  {
    case JSON_TOKEN_OBJECT_START:
      type = "object";
      break;
    case JSON_TOKEN_ARRAY_START:
      type = "array";
      break;
    case JSON_TOKEN_STRING:
      type = "string";
      break;
    case JSON_TOKEN_NUMBER:
      type = "number";
      break;
    case JSON_TOKEN_TRUE:
    case JSON_TOKEN_FALSE:
      type = "boolean";
      break;
    case JSON_TOKEN_NULL:
      type = "null";
      break;
    default:
      elog(ERROR, "unexpected json token: %d", lex.token_type);
  }

  return cstring_to_text(type);
}

/*****************************************************************************/
