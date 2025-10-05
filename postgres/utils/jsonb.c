/*-------------------------------------------------------------------------
 *
 * jsonb.c
 *    I/O routines for jsonb type
 *
 * Copyright (c) 2014-2025, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *    src/backend/utils/adt/jsonb.c
 *
 *-------------------------------------------------------------------------
 */

/* C */
#include <assert.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include <stdlib.h>
/* PostgreSQL */
#include <postgres.h>
#include <postgres_types.h>
#include "catalog/pg_type.h"
#include <common/hashfn.h>
#include <common/int.h>
#include <common/jsonapi.h>
#include <nodes/nodes.h>
#include <utils/json.h>
#include <utils/jsonb.h>
#include "utils/jsonfuncs.h"
#include <utils/varlena.h> /* For DatumGetTextP */

extern void escape_json_with_len(StringInfo buf, const char *str, int len);

// #include "postgres.h"
// #include "access/htup_details.h"
// #include "catalog/pg_proc.h"
// #include "catalog/pg_type.h"
// #include "funcapi.h"
// #include "libpq/pqformat.h"
// #include "miscadmin.h"
// #include "utils/builtins.h"
// #include "utils/json.h"
// #include "utils/jsonb.h"
// #include "utils/jsonfuncs.h"
// #include "utils/lsyscache.h"
// #include "utils/typcache.h"

typedef struct JsonbInState
{
  JsonbParseState *parseState;
  JsonbValue *res;
  bool    unique_keys;
  Node     *escontext;
} JsonbInState;

typedef struct JsonbAggState
{
  JsonbInState *res;
  JsonTypeCategory key_category;
  Oid      key_output_func;
  JsonTypeCategory val_category;
  Oid      val_output_func;
} JsonbAggState;

static inline Jsonb *jsonb_from_cstring(char *json, int len, bool unique_keys,
  Node *escontext);
static bool checkStringLen(size_t len, Node *escontext);
static JsonParseErrorType jsonb_in_object_start(void *pstate);
static JsonParseErrorType jsonb_in_object_end(void *pstate);
static JsonParseErrorType jsonb_in_array_start(void *pstate);
static JsonParseErrorType jsonb_in_array_end(void *pstate);
static JsonParseErrorType jsonb_in_object_field_start(void *pstate,
  char *fname, bool isnull);
static void jsonb_put_escaped_value(StringInfo out, JsonbValue *scalarVal);
static JsonParseErrorType jsonb_in_scalar(void *pstate, char *token,
  JsonTokenType tokentype);
static void composite_to_jsonb(Datum composite, JsonbInState *result);
static void array_dim_to_jsonb(JsonbInState *result, int dim, int ndims,
  int *dims, const Datum *vals, const bool *nulls, int *valcount,
  JsonTypeCategory tcategory, Oid outfuncoid);
static void array_to_jsonb_internal(Datum array, JsonbInState *result);
static void datum_to_jsonb_internal(Datum val, bool is_null,
  JsonbInState *result, JsonTypeCategory tcategory, Oid outfuncoid,
  bool key_scalar);
static void add_jsonb(Datum val, bool is_null, JsonbInState *result,
  Oid val_type, bool key_scalar);
static JsonbParseState *clone_parse_state(JsonbParseState *state);
static char *JsonbToCStringWorker(StringInfo out, JsonbContainer *in,
  int estimated_len, bool indent);
static void add_indent(StringInfo out, bool indent, int level);

/*
 * jsonb type input function
 */
Jsonb *
pg_jsonb_in(char *json)
{
  return jsonb_from_cstring(json, strlen(json), false, NULL);
}

/*
 * jsonb type output function
 */
char *
pg_jsonb_out(Jsonb *jb)
{
  return JsonbToCString(NULL, &jb->root, VARSIZE(jb));
}

/*
 * jsonb_from_text
 *
 * Turns json text string into a jsonb Datum.
 */
Datum
jsonb_from_text(text *js, bool unique_keys)
{
  return jsonb_from_cstring(VARDATA_ANY(js), VARSIZE_ANY_EXHDR(js),
    unique_keys, NULL);
}

/*
 * Get the type name of a jsonb container.
 */
static const char *
JsonbContainerTypeName(JsonbContainer *jbc)
{
  JsonbValue  scalar;
  if (JsonbExtractScalar(jbc, &scalar))
    return JsonbTypeName(&scalar);
  else if (JsonContainerIsArray(jbc))
    return "array";
  else if (JsonContainerIsObject(jbc))
    return "object";
  else
  {
    elog(ERROR, "invalid jsonb container type: 0x%08x", jbc->header);
    return "unknown";
  }
}

/*
 * Get the type name of a jsonb value.
 */
const char *
JsonbTypeName(JsonbValue *val)
{
  switch (val->type)
  {
    case jbvBinary:
      return JsonbContainerTypeName(val->val.binary.data);
    case jbvObject:
      return "object";
    case jbvArray:
      return "array";
    case jbvNumeric:
      return "number";
    case jbvString:
      return "string";
    case jbvBool:
      return "boolean";
    case jbvNull:
      return "null";
    case jbvDatetime:
      switch (val->val.datetime.typid)
      {
        case DATEOID:
          return "date";
        case TIMEOID:
          return "time without time zone";
        case TIMETZOID:
          return "time with time zone";
        case TIMESTAMPOID:
          return "timestamp without time zone";
        case TIMESTAMPTZOID:
          return "timestamp with time zone";
        default:
          elog(ERROR, "unrecognized jsonb value datetime type: %d",
             val->val.datetime.typid);
      }
      return "unknown";
    default:
      elog(ERROR, "unrecognized jsonb value type: %d", val->type);
      return "unknown";
  }
}

/*
 * SQL function jsonb_typeof(jsonb) -> text
 *
 * This function is here because the analog json function is in json.c, since
 * it uses the json parser internals not exposed elsewhere.
 */
text *
jsonb_typeof_internal(Jsonb *in)
{
  const char *result = JsonbContainerTypeName(&in->root);
  return cstring_to_text(result);
}

/*
 * jsonb_from_cstring
 *
 * Turns json string into a jsonb Datum.
 *
 * Uses the json parser (with hooks) to construct a jsonb.
 *
 * If escontext points to an ErrorSaveContext, errors are reported there
 * instead of being thrown.
 */
static inline Jsonb *
jsonb_from_cstring(char *json, int len, bool unique_keys, Node *escontext)
{
  JsonLexContext lex;
  JsonbInState state;
  JsonSemAction sem;

  memset(&state, 0, sizeof(state));
  memset(&sem, 0, sizeof(sem));
  makeJsonLexContextCstringLen(&lex, json, len, GetDatabaseEncoding(), true);

  state.unique_keys = unique_keys;
  state.escontext = escontext;
  sem.semstate = &state;

  sem.object_start = jsonb_in_object_start;
  sem.array_start = jsonb_in_array_start;
  sem.object_end = jsonb_in_object_end;
  sem.array_end = jsonb_in_array_end;
  sem.scalar = jsonb_in_scalar;
  sem.object_field_start = jsonb_in_object_field_start;

  if (!pg_parse_json_or_errsave(&lex, &sem, escontext))
    return NULL;

  /* after parsing, the item member has the composed jsonb structure */
  return JsonbValueToJsonb(state.res);
}

static bool
checkStringLen(size_t len, Node *escontext)
{
  if (len > JENTRY_OFFLENMASK)
  {
    elog(ERROR, "string too long to represent as jsonb string");
    return false;
  }
  return true;
}

static JsonParseErrorType
jsonb_in_object_start(void *pstate)
{
  JsonbInState *_state = (JsonbInState *) pstate;
  _state->res = pushJsonbValue(&_state->parseState, WJB_BEGIN_OBJECT, NULL);
  _state->parseState->unique_keys = _state->unique_keys;
  return JSON_SUCCESS;
}

static JsonParseErrorType
jsonb_in_object_end(void *pstate)
{
  JsonbInState *_state = (JsonbInState *) pstate;
  _state->res = pushJsonbValue(&_state->parseState, WJB_END_OBJECT, NULL);
  return JSON_SUCCESS;
}

static JsonParseErrorType
jsonb_in_array_start(void *pstate)
{
  JsonbInState *_state = (JsonbInState *) pstate;
  _state->res = pushJsonbValue(&_state->parseState, WJB_BEGIN_ARRAY, NULL);
  return JSON_SUCCESS;
}

static JsonParseErrorType
jsonb_in_array_end(void *pstate)
{
  JsonbInState *_state = (JsonbInState *) pstate;
  _state->res = pushJsonbValue(&_state->parseState, WJB_END_ARRAY, NULL);
  return JSON_SUCCESS;
}

static JsonParseErrorType
jsonb_in_object_field_start(void *pstate, char *fname, bool isnull)
{
  JsonbInState *_state = (JsonbInState *) pstate;
  JsonbValue  v;

  Assert(fname != NULL);
  v.type = jbvString;
  v.val.string.len = strlen(fname);
  if (!checkStringLen(v.val.string.len, _state->escontext))
    return JSON_SEM_ACTION_FAILED;
  v.val.string.val = fname;
  _state->res = pushJsonbValue(&_state->parseState, WJB_KEY, &v);
  return JSON_SUCCESS;
}

static void
jsonb_put_escaped_value(StringInfo out, JsonbValue *scalarVal)
{
  switch (scalarVal->type)
  {
    case jbvNull:
      appendBinaryStringInfo(out, "null", 4);
      break;
    case jbvString:
      escape_json_with_len(out, scalarVal->val.string.val, scalarVal->val.string.len);
      break;
    case jbvNumeric:
      appendStringInfoString(out, pg_numeric_out(scalarVal->val.numeric));
      break;
    case jbvBool:
      if (scalarVal->val.boolean)
        appendBinaryStringInfo(out, "true", 4);
      else
        appendBinaryStringInfo(out, "false", 5);
      break;
    default:
      elog(ERROR, "unknown jsonb scalar type");
  }
}

/*
 * For jsonb we always want the de-escaped value - that's what's in token
 */
static JsonParseErrorType
jsonb_in_scalar(void *pstate, char *token, JsonTokenType tokentype)
{
  JsonbInState *_state = (JsonbInState *) pstate;
  JsonbValue  v;

  switch (tokentype)
  {

    case JSON_TOKEN_STRING:
      Assert(token != NULL);
      v.type = jbvString;
      v.val.string.len = strlen(token);
      if (!checkStringLen(v.val.string.len, _state->escontext))
        return JSON_SEM_ACTION_FAILED;
      v.val.string.val = token;
      break;
    case JSON_TOKEN_NUMBER:

      /*
       * No need to check size of numeric values, because maximum
       * numeric size is well below the JsonbValue restriction
       */
      Assert(token != NULL);
      v.type = jbvNumeric;
      Numeric num = pg_numeric_in(token, -1);
      if (! num)
        return JSON_SEM_ACTION_FAILED;
      v.val.numeric = num;
      break;
    case JSON_TOKEN_TRUE:
      v.type = jbvBool;
      v.val.boolean = true;
      break;
    case JSON_TOKEN_FALSE:
      v.type = jbvBool;
      v.val.boolean = false;
      break;
    case JSON_TOKEN_NULL:
      v.type = jbvNull;
      break;
    default:
      /* should not be possible */
      elog(ERROR, "invalid json token type");
      break;
  }

  if (_state->parseState == NULL)
  {
    /* single scalar */
    JsonbValue  va;

    va.type = jbvArray;
    va.val.array.rawScalar = true;
    va.val.array.nElems = 1;

    _state->res = pushJsonbValue(&_state->parseState, WJB_BEGIN_ARRAY, &va);
    _state->res = pushJsonbValue(&_state->parseState, WJB_ELEM, &v);
    _state->res = pushJsonbValue(&_state->parseState, WJB_END_ARRAY, NULL);
  }
  else
  {
    JsonbValue *o = &_state->parseState->contVal;

    switch (o->type)
    {
      case jbvArray:
        _state->res = pushJsonbValue(&_state->parseState, WJB_ELEM, &v);
        break;
      case jbvObject:
        _state->res = pushJsonbValue(&_state->parseState, WJB_VALUE, &v);
        break;
      default:
        elog(ERROR, "unexpected parent of nested structure");
    }
  }

  return JSON_SUCCESS;
}

/*
 * JsonbToCString
 *     Converts jsonb value to a C-string.
 *
 * If 'out' argument is non-null, the resulting C-string is stored inside the
 * StringBuffer.  The resulting string is always returned.
 *
 * A typical case for passing the StringInfo in rather than NULL is where the
 * caller wants access to the len attribute without having to call strlen, e.g.
 * if they are converting it to a text* object.
 */
char *
JsonbToCString(StringInfo out, JsonbContainer *in, int estimated_len)
{
  return JsonbToCStringWorker(out, in, estimated_len, false);
}

/*
 * same thing but with indentation turned on
 */
char *
JsonbToCStringIndent(StringInfo out, JsonbContainer *in, int estimated_len)
{
  return JsonbToCStringWorker(out, in, estimated_len, true);
}

/*
 * common worker for above two functions
 */
static char *
JsonbToCStringWorker(StringInfo out, JsonbContainer *in, int estimated_len,
  bool indent)
{
  bool first = true;
  JsonbIterator *it;
  JsonbValue  v;
  JsonbIteratorToken type = WJB_DONE;
  int level = 0;
  bool redo_switch = false;

  /* If we are indenting, don't add a space after a comma */
  int ispaces = indent ? 1 : 2;

  /*
   * Don't indent the very first item. This gets set to the indent flag at
   * the bottom of the loop.
   */
  bool    use_indent = false;
  bool    raw_scalar = false;
  bool    last_was_key = false;

  if (out == NULL)
    out = makeStringInfo();

  enlargeStringInfo(out, (estimated_len >= 0) ? estimated_len : 64);

  it = JsonbIteratorInit(in);

  while (redo_switch ||
       ((type = JsonbIteratorNext(&it, &v, false)) != WJB_DONE))
  {
    redo_switch = false;
    switch (type)
    {
      case WJB_BEGIN_ARRAY:
        if (!first)
          appendBinaryStringInfo(out, ", ", ispaces);

        if (!v.val.array.rawScalar)
        {
          add_indent(out, use_indent && !last_was_key, level);
          appendStringInfoCharMacro(out, '[');
        }
        else
          raw_scalar = true;

        first = true;
        level++;
        break;
      case WJB_BEGIN_OBJECT:
        if (!first)
          appendBinaryStringInfo(out, ", ", ispaces);

        add_indent(out, use_indent && !last_was_key, level);
        appendStringInfoCharMacro(out, '{');

        first = true;
        level++;
        break;
      case WJB_KEY:
        if (!first)
          appendBinaryStringInfo(out, ", ", ispaces);
        first = true;

        add_indent(out, use_indent, level);

        /* json rules guarantee this is a string */
        jsonb_put_escaped_value(out, &v);
        appendBinaryStringInfo(out, ": ", 2);

        type = JsonbIteratorNext(&it, &v, false);
        if (type == WJB_VALUE)
        {
          first = false;
          jsonb_put_escaped_value(out, &v);
        }
        else
        {
          Assert(type == WJB_BEGIN_OBJECT || type == WJB_BEGIN_ARRAY);

          /*
           * We need to rerun the current switch() since we need to
           * output the object which we just got from the iterator
           * before calling the iterator again.
           */
          redo_switch = true;
        }
        break;
      case WJB_ELEM:
        if (!first)
          appendBinaryStringInfo(out, ", ", ispaces);
        first = false;

        if (!raw_scalar)
          add_indent(out, use_indent, level);
        jsonb_put_escaped_value(out, &v);
        break;
      case WJB_END_ARRAY:
        level--;
        if (!raw_scalar)
        {
          add_indent(out, use_indent, level);
          appendStringInfoCharMacro(out, ']');
        }
        first = false;
        break;
      case WJB_END_OBJECT:
        level--;
        add_indent(out, use_indent, level);
        appendStringInfoCharMacro(out, '}');
        first = false;
        break;
      default:
        elog(ERROR, "unknown jsonb iterator token type");
    }
    use_indent = indent;
    last_was_key = redo_switch;
  }

  Assert(level == 0);

  return out->data;
}

static void
add_indent(StringInfo out, bool indent, int level)
{
  if (indent)
  {
    appendStringInfoCharMacro(out, '\n');
    appendStringInfoSpaces(out, level * 4);
  }
}

#if 0 /* NOT USED */

/*
 * Turn a Datum into jsonb, adding it to the result JsonbInState.
 *
 * tcategory and outfuncoid are from a previous call to json_categorize_type,
 * except that if is_null is true then they can be invalid.
 *
 * If key_scalar is true, the value is stored as a key, so insist
 * it's of an acceptable type, and force it to be a jbvString.
 *
 * Note: currently, we assume that result->escontext is NULL and errors
 * will be thrown.
 */
static void
datum_to_jsonb_internal(Datum val, bool is_null, JsonbInState *result,
  JsonTypeCategory tcategory, Oid outfuncoid, bool key_scalar)
{
  char *outputstr;
  bool numeric_error;
  JsonbValue jb;
  bool scalar_jsonb = false;

  /* Convert val to a JsonbValue in jb (in most cases) */
  if (is_null)
  {
    Assert(!key_scalar);
    jb.type = jbvNull;
  }
  else if (key_scalar &&
       (tcategory == JSONTYPE_ARRAY || tcategory == JSONTYPE_COMPOSITE ||
        tcategory == JSONTYPE_JSON || tcategory == JSONTYPE_JSONB ||
        tcategory == JSONTYPE_CAST))
  {
    elog(ERROR, "key value must be scalar, not array, composite, or json");
    return;
  }
  else
  {
    if (tcategory == JSONTYPE_CAST)
      val = OidFunctionCall1(outfuncoid, val);

    switch (tcategory)
    {
      case JSONTYPE_ARRAY:
        array_to_jsonb_internal(val, result);
        break;
      case JSONTYPE_COMPOSITE:
        composite_to_jsonb(val, result);
        break;
      case JSONTYPE_BOOL:
        if (key_scalar)
        {
          outputstr = DatumGetBool(val) ? "true" : "false";
          jb.type = jbvString;
          jb.val.string.len = strlen(outputstr);
          jb.val.string.val = outputstr;
        }
        else
        {
          jb.type = jbvBool;
          jb.val.boolean = DatumGetBool(val);
        }
        break;
      case JSONTYPE_NUMERIC:
        outputstr = OidOutputFunctionCall(outfuncoid, val);
        if (key_scalar)
        {
          /* always quote keys */
          jb.type = jbvString;
          jb.val.string.len = strlen(outputstr);
          jb.val.string.val = outputstr;
        }
        else
        {
          /*
           * Make it numeric if it's a valid JSON number, otherwise
           * a string. Invalid numeric output will always have an
           * 'N' or 'n' in it (I think).
           */
          numeric_error = (strchr(outputstr, 'N') != NULL ||
            strchr(outputstr, 'n') != NULL);
          if (!numeric_error)
          {
            jb.type = jbvNumeric;
            jb.val.numeric = pg_numeric_in(outputstr, -1);
            pfree(outputstr);
          }
          else
          {
            jb.type = jbvString;
            jb.val.string.len = strlen(outputstr);
            jb.val.string.val = outputstr;
          }
        }
        break;
      case JSONTYPE_DATE:
        jb.type = jbvString;
        jb.val.string.val = JsonEncodeDateTime(NULL, val, DATEOID, NULL);
        jb.val.string.len = strlen(jb.val.string.val);
        break;
      case JSONTYPE_TIMESTAMP:
        jb.type = jbvString;
        jb.val.string.val = JsonEncodeDateTime(NULL, val, TIMESTAMPOID, NULL);
        jb.val.string.len = strlen(jb.val.string.val);
        break;
      case JSONTYPE_TIMESTAMPTZ:
        jb.type = jbvString;
        jb.val.string.val = JsonEncodeDateTime(NULL, val, TIMESTAMPTZOID, NULL);
        jb.val.string.len = strlen(jb.val.string.val);
        break;
      case JSONTYPE_CAST:
      case JSONTYPE_JSON:
        {
          /* parse the json right into the existing result object */
          JsonLexContext lex;
          text *json = DatumGetTextP(val);
          makeJsonLexContext(&lex, json, true);

          JsonSemAction sem;
          memset(&sem, 0, sizeof(sem));
          sem.semstate = result;
          sem.object_start = jsonb_in_object_start;
          sem.array_start = jsonb_in_array_start;
          sem.object_end = jsonb_in_object_end;
          sem.array_end = jsonb_in_array_end;
          sem.scalar = jsonb_in_scalar;
          sem.object_field_start = jsonb_in_object_field_start;

          pg_parse_json_or_ereport(&lex, &sem);
          freeJsonLexContext(&lex);
        }
        break;
      case JSONTYPE_JSONB:
        {
          Jsonb *jsonb = DatumGetJsonbP(val);
          JsonbIterator *it = JsonbIteratorInit(&jsonb->root);
          if (JB_ROOT_IS_SCALAR(jsonb))
          {
            (void) JsonbIteratorNext(&it, &jb, true);
            Assert(jb.type == jbvArray);
            (void) JsonbIteratorNext(&it, &jb, true);
            scalar_jsonb = true;
          }
          else
          {
            JsonbIteratorToken type;
            while ((type = JsonbIteratorNext(&it, &jb, false)) != WJB_DONE)
            {
              if (type == WJB_END_ARRAY || type == WJB_END_OBJECT ||
                type == WJB_BEGIN_ARRAY || type == WJB_BEGIN_OBJECT)
                result->res = pushJsonbValue(&result->parseState, type, NULL);
              else
                result->res = pushJsonbValue(&result->parseState, type, &jb);
            }
          }
        }
        break;
      default:
        outputstr = OidOutputFunctionCall(outfuncoid, val);
        jb.type = jbvString;
        jb.val.string.len = strlen(outputstr);
        (void) checkStringLen(jb.val.string.len, NULL);
        jb.val.string.val = outputstr;
        break;
    }
  }

  /* Now insert jb into result, unless we did it recursively */
  if (!is_null && !scalar_jsonb &&
    tcategory >= JSONTYPE_JSON && tcategory <= JSONTYPE_CAST)
  {
    /* work has been done recursively */
    return;
  }
  else if (result->parseState == NULL)
  {
    /* single root scalar */
    JsonbValue  va;
    va.type = jbvArray;
    va.val.array.rawScalar = true;
    va.val.array.nElems = 1;

    result->res = pushJsonbValue(&result->parseState, WJB_BEGIN_ARRAY, &va);
    result->res = pushJsonbValue(&result->parseState, WJB_ELEM, &jb);
    result->res = pushJsonbValue(&result->parseState, WJB_END_ARRAY, NULL);
  }
  else
  {
    JsonbValue *o = &result->parseState->contVal;
    switch (o->type)
    {
      case jbvArray:
        result->res = pushJsonbValue(&result->parseState, WJB_ELEM, &jb);
        break;
      case jbvObject:
        result->res = pushJsonbValue(&result->parseState,
          key_scalar ? WJB_KEY : WJB_VALUE, &jb);
        break;
      default:
        elog(ERROR, "unexpected parent of nested structure");
    }
  }
}

/*
 * Process a single dimension of an array.
 * If it's the innermost dimension, output the values, otherwise call
 * ourselves recursively to process the next dimension.
 */
static void
array_dim_to_jsonb(JsonbInState *result, int dim, int ndims, int *dims,
  const Datum *vals, const bool *nulls, int *valcount,
  JsonTypeCategory tcategory, Oid outfuncoid)
{
  int      i;

  Assert(dim < ndims);

  result->res = pushJsonbValue(&result->parseState, WJB_BEGIN_ARRAY, NULL);

  for (i = 1; i <= dims[dim]; i++)
  {
    if (dim + 1 == ndims)
    {
      datum_to_jsonb_internal(vals[*valcount], nulls[*valcount], result, tcategory,
                  outfuncoid, false);
      (*valcount)++;
    }
    else
    {
      array_dim_to_jsonb(result, dim + 1, ndims, dims, vals, nulls,
                 valcount, tcategory, outfuncoid);
    }
  }

  result->res = pushJsonbValue(&result->parseState, WJB_END_ARRAY, NULL);
}

/*
 * Turn an array into JSON.
 */
static void
array_to_jsonb_internal(Datum array, JsonbInState *result)
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
    result->res = pushJsonbValue(&result->parseState, WJB_BEGIN_ARRAY, NULL);
    result->res = pushJsonbValue(&result->parseState, WJB_END_ARRAY, NULL);
    return;
  }

  get_typlenbyvalalign(element_type, &typlen, &typbyval, &typalign);

  json_categorize_type(element_type, true, &tcategory, &outfuncoid);

  deconstruct_array(v, element_type, typlen, typbyval, typalign,
    &elements, &nulls, &nitems);

  array_dim_to_jsonb(result, 0, ndim, dim, elements, nulls, &count, tcategory,
    outfuncoid);

  pfree(elements);
  pfree(nulls);
}

/*
 * Turn a composite / record into JSON.
 */
static void
composite_to_jsonb(Datum composite, JsonbInState *result)
{
  HeapTupleHeader td;
  Oid      tupType;
  int32    tupTypmod;
  TupleDesc  tupdesc;
  HeapTupleData tmptup,
         *tuple;
  int      i;

  td = DatumGetHeapTupleHeader(composite);

  /* Extract rowtype info and find a tupdesc */
  tupType = HeapTupleHeaderGetTypeId(td);
  tupTypmod = HeapTupleHeaderGetTypMod(td);
  tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);

  /* Build a temporary HeapTuple control structure */
  tmptup.t_len = HeapTupleHeaderGetDatumLength(td);
  tmptup.t_data = td;
  tuple = &tmptup;

  result->res = pushJsonbValue(&result->parseState, WJB_BEGIN_OBJECT, NULL);

  for (i = 0; i < tupdesc->natts; i++)
  {
    Datum    val;
    bool    isnull;
    char     *attname;
    JsonTypeCategory tcategory;
    Oid      outfuncoid;
    JsonbValue  v;
    Form_pg_attribute att = TupleDescAttr(tupdesc, i);

    if (att->attisdropped)
      continue;

    attname = NameStr(att->attname);

    v.type = jbvString;
    /* don't need checkStringLen here - can't exceed maximum name length */
    v.val.string.len = strlen(attname);
    v.val.string.val = attname;

    result->res = pushJsonbValue(&result->parseState, WJB_KEY, &v);
    val = heap_getattr(tuple, i + 1, tupdesc, &isnull);

    if (isnull)
    {
      tcategory = JSONTYPE_NULL;
      outfuncoid = InvalidOid;
    }
    else
      json_categorize_type(att->atttypid, true, &tcategory, &outfuncoid);

    datum_to_jsonb_internal(val, isnull, result, tcategory, outfuncoid, false);
  }

  result->res = pushJsonbValue(&result->parseState, WJB_END_OBJECT, NULL);
  ReleaseTupleDesc(tupdesc);
}

/*
 * Append JSON text for "val" to "result".
 *
 * This is just a thin wrapper around datum_to_jsonb.  If the same type will be
 * printed many times, avoid using this; better to do the json_categorize_type
 * lookups only once.
 */
static void
add_jsonb(Datum val, bool is_null, JsonbInState *result, Oid val_type,
  bool key_scalar)
{
  JsonTypeCategory tcategory;
  Oid outfuncoid;

  if (val_type == InvalidOid)
  {
    elog(ERROR, "could not determine input data type");
    return;
  }

  if (is_null)
  {
    tcategory = JSONTYPE_NULL;
    outfuncoid = InvalidOid;
  }
  else
    json_categorize_type(val_type, true, &tcategory, &outfuncoid);

  datum_to_jsonb_internal(val, is_null, result, tcategory, outfuncoid,
    key_scalar);
  return;
}

/*
 * Is the given type immutable when coming out of a JSONB context?
 *
 * At present, datetimes are all considered mutable, because they
 * depend on timezone.  XXX we should also drill down into objects and
 * arrays, but do not.
 */
bool
to_jsonb_is_immutable(Oid typoid)
{
  JsonTypeCategory tcategory;
  Oid outfuncoid;

  json_categorize_type(typoid, true, &tcategory, &outfuncoid);

  switch (tcategory)
  {
    case JSONTYPE_NULL:
    case JSONTYPE_BOOL:
    case JSONTYPE_JSON:
    case JSONTYPE_JSONB:
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
      // return func_volatile(outfuncoid) == PROVOLATILE_IMMUTABLE;
      return true;
  }

  return false;        /* not reached */
}

/*
 * SQL function to_jsonb(anyvalue)
 */
Jsonb *
to_jsonb_internal(Datum val, Oid val_type)
{
  if (val_type == InvalidOid)
  {
    elog(ERROR, "could not determine input data type");
    return NULL;
  }

  JsonTypeCategory tcategory;
  Oid outfuncoid;
  json_categorize_type(val_type, true, &tcategory, &outfuncoid);
  return datum_to_jsonb(val, tcategory, outfuncoid);
}

/*
 * Turn a Datum into jsonb.
 *
 * tcategory and outfuncoid are from a previous call to json_categorize_type.
 */
Datum
datum_to_jsonb(Datum val, JsonTypeCategory tcategory, Oid outfuncoid)
{
  JsonbInState result;
  memset(&result, 0, sizeof(JsonbInState));
  datum_to_jsonb_internal(val, false, &result, tcategory, outfuncoid, false);
  return JsonbPGetDatum(JsonbValueToJsonb(result.res));
}

Datum
jsonb_build_object_worker(int nargs, const Datum *args, const bool *nulls,
  const Oid *types, bool absent_on_null, bool unique_keys)
{
  if (nargs % 2 != 0)
  {
    elog(ERROR, "argument list must have even number of elements");
    return (Datum) 0;
  }

  JsonbInState result;
  memset(&result, 0, sizeof(JsonbInState));

  result.res = pushJsonbValue(&result.parseState, WJB_BEGIN_OBJECT, NULL);
  result.parseState->unique_keys = unique_keys;
  result.parseState->skip_nulls = absent_on_null;

  for (int i = 0; i < nargs; i += 2)
  {
    /* process key */
    if (nulls[i])
    {
      elog(ERROR, "argument %d: key must not be null", i + 1);
      return (Datum) 0;
    }

    /* skip null values if absent_on_null */
    bool skip = absent_on_null && nulls[i + 1];

    /* we need to save skipped keys for the key uniqueness check */
    if (skip && !unique_keys)
      continue;

    add_jsonb(args[i], false, &result, types[i], true);
    /* process value */
    add_jsonb(args[i + 1], nulls[i + 1], &result, types[i + 1], false);
  }
  result.res = pushJsonbValue(&result.parseState, WJB_END_OBJECT, NULL);
  return JsonbPGetDatum(JsonbValueToJsonb(result.res));
}

/*
 * SQL function jsonb_build_object(variadic "any")
 */
Datum
jsonb_build_object_internal(XX)
{
  Datum *args;
  bool *nulls;
  Oid *types;

  /* build argument values to build the object */
  int nargs = extract_variadic_args(fcinfo, 0, true, &args, &types, &nulls);

  if (nargs < 0)
    return NULL;

  PG_RETURN_DATUM(jsonb_build_object_worker(nargs, args, nulls, types,
    false, false));
}

/*
 * degenerate case of jsonb_build_object where it gets 0 arguments.
 */
Jsonb *
jsonb_build_object_noargs_internal(XX)
{
  JsonbInState result;
  memset(&result, 0, sizeof(JsonbInState));
  (void) pushJsonbValue(&result.parseState, WJB_BEGIN_OBJECT, NULL);
  result.res = pushJsonbValue(&result.parseState, WJB_END_OBJECT, NULL);
  return JsonbValueToJsonb(result.res);
}

Datum
jsonb_build_array_worker(int nargs, const Datum *args, const bool *nulls,
  const Oid *types, bool absent_on_null)
{
  JsonbInState result;
  memset(&result, 0, sizeof(JsonbInState));
  result.res = pushJsonbValue(&result.parseState, WJB_BEGIN_ARRAY, NULL);
  for (int i = 0; i < nargs; i++)
  {
    if (absent_on_null && nulls[i])
      continue;
    add_jsonb(args[i], nulls[i], &result, types[i], false);
  }
  result.res = pushJsonbValue(&result.parseState, WJB_END_ARRAY, NULL);
  return JsonbPGetDatum(JsonbValueToJsonb(result.res));
}

/*
 * SQL function jsonb_build_array(variadic "any")
 */
Datum
jsonb_build_array_internal(XX)
{
  Datum     *args;
  bool     *nulls;
  Oid       *types;

  /* build argument values to build the object */
  int nargs = extract_variadic_args(fcinfo, 0, true, &args, &types, &nulls);

  if (nargs < 0)
    return NULL;

  PG_RETURN_DATUM(jsonb_build_array_worker(nargs, args, nulls, types, false));
}

/*
 * degenerate case of jsonb_build_array where it gets 0 arguments.
 */
Jsonb *
jsonb_build_array_noargs_internal(XX)
{
  JsonbInState result;

  memset(&result, 0, sizeof(JsonbInState));

  (void) pushJsonbValue(&result.parseState, WJB_BEGIN_ARRAY, NULL);
  result.res = pushJsonbValue(&result.parseState, WJB_END_ARRAY, NULL);

  return JsonbValueToJsonb(result.res);
}

/*
 * SQL function jsonb_object(text[])
 *
 * take a one or two dimensional array of text as name value pairs
 * for a jsonb object.
 *
 */
Jsonb *
jsonb_object_internal(Datum *in_datums, bool *in_nulls, int in_count)
{
  JsonbInState result;
  memset(&result, 0, sizeof(JsonbInState));
  (void) pushJsonbValue(&result.parseState, WJB_BEGIN_OBJECT, NULL);
  
  int count = in_count / 2;
  for (int i = 0; i < count; ++i)
  {
    JsonbValue  v;
    char *str;
    int len;

    if (in_nulls[i * 2])
    {
      elog(ERROR, "null value not allowed for object key");
      return NULL;
    }

    str = TextDatumGetCString(in_datums[i * 2]);
    len = strlen(str);

    v.type = jbvString;
    v.val.string.len = len;
    v.val.string.val = str;
    (void) pushJsonbValue(&result.parseState, WJB_KEY, &v);

    if (in_nulls[i * 2 + 1])
    {
      v.type = jbvNull;
    }
    else
    {
      str = TextDatumGetCString(in_datums[i * 2 + 1]);
      len = strlen(str);

      v.type = jbvString;

      v.val.string.len = len;
      v.val.string.val = str;
    }

    (void) pushJsonbValue(&result.parseState, WJB_VALUE, &v);
  }

close_object:
  result.res = pushJsonbValue(&result.parseState, WJB_END_OBJECT, NULL);
  return JsonbValueToJsonb(result.res);
}

/*
 * SQL function jsonb_object(text[], text[])
 *
 * take separate name and value arrays of text to construct a jsonb object
 * pairwise.
 */
Jsonb *
jsonb_object_two_arg_internal(XX)
{
  ArrayType  *key_array = PG_GETARG_ARRAYTYPE_P(0);
  ArrayType  *val_array = PG_GETARG_ARRAYTYPE_P(1);
  int      nkdims = ARR_NDIM(key_array);
  int      nvdims = ARR_NDIM(val_array);
  Datum     *key_datums,
         *val_datums;
  bool     *key_nulls,
         *val_nulls;
  int      key_count,
        val_count,
        i;
  JsonbInState result;

  memset(&result, 0, sizeof(JsonbInState));

  (void) pushJsonbValue(&result.parseState, WJB_BEGIN_OBJECT, NULL);

  if (nkdims > 1 || nkdims != nvdims)
  {
    elog(ERROR, "wrong number of array subscripts");
    return NULL;
  }

  if (nkdims == 0)
    goto close_object;

  deconstruct_array_builtin(key_array, TEXTOID, &key_datums, &key_nulls, &key_count);
  deconstruct_array_builtin(val_array, TEXTOID, &val_datums, &val_nulls, &val_count);

  if (key_count != val_count)
  {
    elog(ERROR, "mismatched array dimensions");
    return NULL;
  }

  for (int i = 0; i < key_count; ++i)
  {
    JsonbValue  v;
    char     *str;
    int      len;

    if (key_nulls[i])
    {
      elog(ERROR, "null value not allowed for object key");
      return NULL;
    }

    str = TextDatumGetCString(key_datums[i]);
    len = strlen(str);

    v.type = jbvString;

    v.val.string.len = len;
    v.val.string.val = str;

    (void) pushJsonbValue(&result.parseState, WJB_KEY, &v);

    if (val_nulls[i])
    {
      v.type = jbvNull;
    }
    else
    {
      str = TextDatumGetCString(val_datums[i]);
      len = strlen(str);

      v.type = jbvString;

      v.val.string.len = len;
      v.val.string.val = str;
    }

    (void) pushJsonbValue(&result.parseState, WJB_VALUE, &v);
  }

  pfree(key_datums);
  pfree(key_nulls);
  pfree(val_datums);
  pfree(val_nulls);

close_object:
  result.res = pushJsonbValue(&result.parseState, WJB_END_OBJECT, NULL);

  return JsonbValueToJsonb(result.res);
}

/*
 * shallow clone of a parse state, suitable for use in aggregate
 * final functions that will only append to the values rather than
 * change them.
 */
static JsonbParseState *
clone_parse_state(JsonbParseState *state)
{
  if (state == NULL)
    return NULL;

  JsonbParseState *result = palloc(sizeof(JsonbParseState));
  JsonbParseState *icursor = state;
  JsonbParseState *ocursor = result;
  for (;;)
  {
    ocursor->contVal = icursor->contVal;
    ocursor->size = icursor->size;
    ocursor->unique_keys = icursor->unique_keys;
    ocursor->skip_nulls = icursor->skip_nulls;
    icursor = icursor->next;
    if (icursor == NULL)
      break;
    ocursor->next = palloc(sizeof(JsonbParseState));
    ocursor = ocursor->next;
  }
  ocursor->next = NULL;
  return result;
}

static JsonbInState *
jsonb_agg_transfn_worker(FunctionCallInfo fcinfo, bool absent_on_null)
{
  JsonbAggState *state;
  JsonbInState elem;
  Datum    val;
  JsonbInState *result;
  bool    single_scalar = false;
  JsonbIterator *it;
  Jsonb     *jbelem;
  JsonbValue  v;
  JsonbIteratorToken type;

  /* set up the accumulator on the first go round */

  if (PG_ARGISNULL(0))
  {
    Oid arg_type = get_fn_expr_argtype(fcinfo->flinfo, 1);

    if (arg_type == InvalidOid)
    {
      elog(ERROR, "could not determine input data type");
      return NULL;
    }

    state = palloc(sizeof(JsonbAggState));
    result = palloc0(sizeof(JsonbInState));
    state->res = result;
    result->res = pushJsonbValue(&result->parseState, WJB_BEGIN_ARRAY, NULL);

    json_categorize_type(arg_type, true, &state->val_category,
      &state->val_output_func);
  }
  else
  {
    state = (JsonbAggState *) PG_GETARG_POINTER(0);
    result = state->res;
  }

  if (absent_on_null && PG_ARGISNULL(1))
    return PointerGetDatum(state);

  /* turn the argument into jsonb in the normal function context */

  val = PG_ARGISNULL(1) ? (Datum) 0 : PG_GETARG_DATUM(1);

  memset(&elem, 0, sizeof(JsonbInState));

  datum_to_jsonb_internal(val, PG_ARGISNULL(1), &elem, state->val_category,
              state->val_output_func, false);

  jbelem = JsonbValueToJsonb(elem.res);

  /* switch to the aggregate context for accumulation operations */

  it = JsonbIteratorInit(&jbelem->root);
  while ((type = JsonbIteratorNext(&it, &v, false)) != WJB_DONE)
  {
    switch (type)
    {
      case WJB_BEGIN_ARRAY:
        if (v.val.array.rawScalar)
          single_scalar = true;
        else
          result->res = pushJsonbValue(&result->parseState, type, NULL);
        break;
      case WJB_END_ARRAY:
        if (!single_scalar)
          result->res = pushJsonbValue(&result->parseState, type, NULL);
        break;
      case WJB_BEGIN_OBJECT:
      case WJB_END_OBJECT:
        result->res = pushJsonbValue(&result->parseState, type, NULL);
        break;
      case WJB_ELEM:
      case WJB_KEY:
      case WJB_VALUE:
        if (v.type == jbvString)
        {
          /* copy string values in the aggregate context */
          char *buf = palloc(v.val.string.len + 1);
          snprintf(buf, v.val.string.len + 1, "%s", v.val.string.val);
          v.val.string.val = buf;
        }
        else if (v.type == jbvNumeric)
        {
          /* same for numeric */
          v.val.numeric = pg_numeric_uplus(v.val.numeric);
        }
        result->res = pushJsonbValue(&result->parseState, type, &v);
        break;
      default:
        elog(ERROR, "unknown jsonb iterator token type");
    }
  }
  return state;
}

/*
 * jsonb_agg aggregate function
 */
Datum
jsonb_agg_transfn_internal(XX)
{
  return jsonb_agg_transfn_worker(fcinfo, false);
}

/*
 * jsonb_agg_strict aggregate function
 */
Datum
jsonb_agg_strict_transfn_internal(XX)
{
  return jsonb_agg_transfn_worker(fcinfo, true);
}

Jsonb *
jsonb_agg_finalfn_internal(JsonbAggState *arg)
{
  if (! arg)
    return NULL;    /* returns null iff no input values */

  /*
   * We need to do a shallow clone of the argument in case the final
   * function is called more than once, so we avoid changing the argument. A
   * shallow clone is sufficient as we aren't going to change any of the
   * values, just add the final array end marker.
   */
  JsonbInState result;
  memset(&result, 0, sizeof(JsonbInState));
  result.parseState = clone_parse_state(arg->res->parseState);
  result.res = pushJsonbValue(&result.parseState, WJB_END_ARRAY, NULL);

  return JsonbValueToJsonb(result.res);
}

static JsonbAggState *
jsonb_object_agg_transfn_worker(FunctionCallInfo fcinfo,
  bool absent_on_null, bool unique_keys)
{
  JsonbInState elem;
  JsonbAggState *state;
  Datum val;
  JsonbInState *result;
  bool single_scalar;
  JsonbIterator *it;
  Jsonb *jbkey, *jbval;
  JsonbValue  v;
  JsonbIteratorToken type;
  bool skip;

  /* set up the accumulator on the first go round */

  if (PG_ARGISNULL(0))
  {
    Oid arg_type;

    state = palloc(sizeof(JsonbAggState));
    result = palloc0(sizeof(JsonbInState));
    state->res = result;
    result->res = pushJsonbValue(&result->parseState, WJB_BEGIN_OBJECT, NULL);
    result->parseState->unique_keys = unique_keys;
    result->parseState->skip_nulls = absent_on_null;

    arg_type = get_fn_expr_argtype(fcinfo->flinfo, 1);
    if (arg_type == InvalidOid)
    {
      elog(ERROR, "could not determine input data type");
      return NULL;
    }

    json_categorize_type(arg_type, true, &state->key_category,
      &state->key_output_func);

    arg_type = get_fn_expr_argtype(fcinfo->flinfo, 2);

    if (arg_type == InvalidOid)
    {
      elog(ERROR, "could not determine input data type");
      return NULL;
    }

    json_categorize_type(arg_type, true, &state->val_category,
      &state->val_output_func);
  }
  else
  {
    state = (JsonbAggState *) PG_GETARG_POINTER(0);
    result = state->res;
  }

  /* turn the argument into jsonb in the normal function context */

  if (PG_ARGISNULL(1))
  {
    elog(ERROR, "field name must not be null");
    return NULL;
  }

  /*
   * Skip null values if absent_on_null unless key uniqueness check is
   * needed (because we must save keys in this case).
   */
  skip = absent_on_null && PG_ARGISNULL(2);

  if (skip && !unique_keys)
    PG_RETURN_POINTER(state);

  val = PG_GETARG_DATUM(1);

  memset(&elem, 0, sizeof(JsonbInState));

  datum_to_jsonb_internal(val, false, &elem, state->key_category,
    state->key_output_func, true);

  jbkey = JsonbValueToJsonb(elem.res);

  val = PG_ARGISNULL(2) ? (Datum) 0 : PG_GETARG_DATUM(2);

  memset(&elem, 0, sizeof(JsonbInState));

  datum_to_jsonb_internal(val, PG_ARGISNULL(2), &elem, state->val_category,
    state->val_output_func, false);

  jbval = JsonbValueToJsonb(elem.res);
  it = JsonbIteratorInit(&jbkey->root);

  /*
   * keys should be scalar, and we should have already checked for that
   * above when calling datum_to_jsonb, so we only need to look for these
   * things.
   */

  while ((type = JsonbIteratorNext(&it, &v, false)) != WJB_DONE)
  {
    switch (type)
    {
      case WJB_BEGIN_ARRAY:
        if (!v.val.array.rawScalar)
        {
          elog(ERROR, "unexpected structure for key");
          return NULL;
        }
      case WJB_ELEM:
        if (v.type == jbvString)
        {
          /* copy string values in the aggregate context */
          char *buf = palloc(v.val.string.len + 1);

          snprintf(buf, v.val.string.len + 1, "%s", v.val.string.val);
          v.val.string.val = buf;
        }
        else
        {
          elog(ERROR, "object keys must be strings");
          return NULL;
        }
        result->res = pushJsonbValue(&result->parseState, WJB_KEY, &v);

        if (skip)
        {
          v.type = jbvNull;
          result->res = pushJsonbValue(&result->parseState, WJB_VALUE, &v);
          return state;
        }

        break;
      case WJB_END_ARRAY:
        break;
      default:
      {
        elog(ERROR, "unexpected structure for key");
        return NULL;
      }
    }
  }

  it = JsonbIteratorInit(&jbval->root);
  single_scalar = false;

  /*
   * values can be anything, including structured and null, so we treat them
   * as in json_agg_transfn, except that single scalars are always pushed as
   * WJB_VALUE items.
   */

  while ((type = JsonbIteratorNext(&it, &v, false)) != WJB_DONE)
  {
    switch (type)
    {
      case WJB_BEGIN_ARRAY:
        if (v.val.array.rawScalar)
          single_scalar = true;
        else
          result->res = pushJsonbValue(&result->parseState, type, NULL);
        break;
      case WJB_END_ARRAY:
        if (!single_scalar)
          result->res = pushJsonbValue(&result->parseState, type, NULL);
        break;
      case WJB_BEGIN_OBJECT:
      case WJB_END_OBJECT:
        result->res = pushJsonbValue(&result->parseState, type, NULL);
        break;
      case WJB_ELEM:
      case WJB_KEY:
      case WJB_VALUE:
        if (v.type == jbvString)
        {
          /* copy string values in the aggregate context */
          char     *buf = palloc(v.val.string.len + 1);

          snprintf(buf, v.val.string.len + 1, "%s", v.val.string.val);
          v.val.string.val = buf;
        }
        else if (v.type == jbvNumeric)
        {
          /* same for numeric */
          v.val.numeric = pg_numeric_uplus(v.val.numeric);
        }
        result->res = pushJsonbValue(&result->parseState,
          single_scalar ? WJB_VALUE : type, &v);
        break;
      default:
        elog(ERROR, "unknown jsonb iterator token type");
    }
  }

  return state;
}

/*
 * jsonb_object_agg aggregate function
 */
Datum
jsonb_object_agg_transfn_internal(XX)
{
  return jsonb_object_agg_transfn_worker(fcinfo, false, false);
}

/*
 * jsonb_object_agg_strict aggregate function
 */
Datum
jsonb_object_agg_strict_transfn_internal(XX)
{
  return jsonb_object_agg_transfn_worker(fcinfo, true, false);
}

/*
 * jsonb_object_agg_unique aggregate function
 */
Datum
jsonb_object_agg_unique_transfn_internal(XX)
{
  return jsonb_object_agg_transfn_worker(fcinfo, false, true);
}

/*
 * jsonb_object_agg_unique_strict aggregate function
 */
Datum
jsonb_object_agg_unique_strict_transfn_internal(XX)
{
  return jsonb_object_agg_transfn_worker(fcinfo, true, true);
}

Jsonb *
jsonb_object_agg_finalfn_internal(JsonbAggState *arg)
{
  if (! arg)
    return NULL;    /* returns null iff no input values */

  /*
   * We need to do a shallow clone of the argument's res field in case the
   * final function is called more than once, so we avoid changing the
   * aggregate state value.  A shallow clone is sufficient as we aren't
   * going to change any of the values, just add the final object end
   * marker.
   */
  JsonbInState result;
  memset(&result, 0, sizeof(JsonbInState));
  result.parseState = clone_parse_state(arg->res->parseState);
  result.res = pushJsonbValue(&result.parseState, WJB_END_OBJECT, NULL);

  return JsonbValueToJsonb(result.res);
}

#endif /* NOT USED */

/*
 * Extract scalar value from raw-scalar pseudo-array jsonb.
 */
bool
JsonbExtractScalar(JsonbContainer *jbc, JsonbValue *res)
{
  JsonbIterator *it;
  JsonbIteratorToken tok PG_USED_FOR_ASSERTS_ONLY;
  JsonbValue  tmp;

  if (!JsonContainerIsArray(jbc) || !JsonContainerIsScalar(jbc))
  {
    /* inform caller about actual type of container */
    res->type = (JsonContainerIsArray(jbc)) ? jbvArray : jbvObject;
    return false;
  }

  /*
   * A root scalar is stored as an array of one element, so we get the array
   * and then its first (and only) member.
   */
  it = JsonbIteratorInit(jbc);

  tok = JsonbIteratorNext(&it, &tmp, true);
  Assert(tok == WJB_BEGIN_ARRAY);
  Assert(tmp.val.array.nElems == 1 && tmp.val.array.rawScalar);

  tok = JsonbIteratorNext(&it, res, true);
  Assert(tok == WJB_ELEM);
  Assert(IsAJsonbScalar(res));

  tok = JsonbIteratorNext(&it, &tmp, true);
  Assert(tok == WJB_END_ARRAY);

  tok = JsonbIteratorNext(&it, &tmp, true);
  Assert(tok == WJB_DONE);

  return true;
}

/*
 * Emit correct, translatable cast error message
 */
static void
cannotCastJsonbValue(enum jbvType type, const char *sqltype)
{
  static const struct
  {
    enum jbvType type;
    const char *msg;
  } messages[] =
  {
    {jbvNull, gettext_noop("cannot cast jsonb null to type %s")},
    {jbvString, gettext_noop("cannot cast jsonb string to type %s")},
    {jbvNumeric, gettext_noop("cannot cast jsonb numeric to type %s")},
    {jbvBool, gettext_noop("cannot cast jsonb boolean to type %s")},
    {jbvArray, gettext_noop("cannot cast jsonb array to type %s")},
    {jbvObject, gettext_noop("cannot cast jsonb object to type %s")},
    {jbvBinary, gettext_noop("cannot cast jsonb array or object to type %s")}
  };

  for (int i = 0; i < lengthof(messages); i++)
    if (messages[i].type == type)
    {
      elog(ERROR, messages[i].msg, sqltype);
    }

  /* should be unreachable */
  elog(ERROR, "unknown jsonb type: %d", (int) type);
}

bool
jsonb_to_bool(Jsonb *in)
{
  JsonbValue v;
  if (!JsonbExtractScalar(&in->root, &v))
    cannotCastJsonbValue(v.type, "boolean");

  if (v.type == jbvNull)
    return NULL;

  if (v.type != jbvBool)
    cannotCastJsonbValue(v.type, "boolean");

  return v.val.boolean;
}

Numeric
jsonb_to_numeric(Jsonb *in)
{
  Numeric retValue;

  JsonbValue v;
  if (!JsonbExtractScalar(&in->root, &v))
    cannotCastJsonbValue(v.type, "numeric");

  if (v.type == jbvNull)
    return NULL;

  if (v.type != jbvNumeric)
    cannotCastJsonbValue(v.type, "numeric");

  /*
   * v.val.numeric points into jsonb body, so we need to make a copy to
   * return
   */
  retValue = DatumGetNumericCopy(NumericGetDatum(v.val.numeric));
  return retValue;
}

int16
jsonb_int16(Jsonb *in)
{
  JsonbValue  v;
  if (!JsonbExtractScalar(&in->root, &v))
    cannotCastJsonbValue(v.type, "smallint");
  if (v.type == jbvNull)
    return NULL;
  if (v.type != jbvNumeric)
    cannotCastJsonbValue(v.type, "smallint");
  return numeric_to_int16(v.val.numeric);
}

int32
jsonb_to_int32(Jsonb *in)
{
  JsonbValue  v;
  if (!JsonbExtractScalar(&in->root, &v))
    cannotCastJsonbValue(v.type, "integer");

  if (v.type == jbvNull)
    return NULL;

  if (v.type != jbvNumeric)
    cannotCastJsonbValue(v.type, "integer");

  return numeric_to_int32(v.val.numeric);
}

int64
jsonb_to_int64(Jsonb *in)
{
  JsonbValue v;
  if (!JsonbExtractScalar(&in->root, &v))
    cannotCastJsonbValue(v.type, "bigint");

  if (v.type == jbvNull)
    return NULL;

  if (v.type != jbvNumeric)
    cannotCastJsonbValue(v.type, "bigint");

  return numeric_to_int64(v.val.numeric);
}

float4
jsonb_to_float4(Jsonb *jb)
{
  JsonbValue v;
  if (!JsonbExtractScalar(&jb->root, &v))
    cannotCastJsonbValue(v.type, "real");

  if (v.type == jbvNull)
    return FLT_MAX;

  if (v.type != jbvNumeric)
    cannotCastJsonbValue(v.type, "real");

  return numeric_to_float4(v.val.numeric);
}

float8
jsonb_to_float8(Jsonb *jb)
{
  JsonbValue v;
  if (!JsonbExtractScalar(&jb->root, &v))
    cannotCastJsonbValue(v.type, "double precision");

  if (v.type == jbvNull)
    return DBL_MAX;

  if (v.type != jbvNumeric)
    cannotCastJsonbValue(v.type, "double precision");

  return numeric_to_float8(v.val.numeric);
}

/*
 * Convert jsonb to a C-string stripping quotes from scalar strings.
 */
char *
JsonbUnquote(Jsonb *jb)
{
  if (JB_ROOT_IS_SCALAR(jb))
  {
    JsonbValue v;
    (void) JsonbExtractScalar(&jb->root, &v);

    if (v.type == jbvString)
      return pnstrdup(v.val.string.val, v.val.string.len);
    else if (v.type == jbvBool)
      return pstrdup(v.val.boolean ? "true" : "false");
    else if (v.type == jbvNumeric)
      return pg_numeric_out(v.val.numeric);
    else if (v.type == jbvNull)
      return pstrdup("null");
    else
    {
      elog(ERROR, "unrecognized jsonb value type %d", v.type);
      return NULL;
    }
  }
  else
    return JsonbToCString(NULL, &jb->root, VARSIZE(jb));
}

/*****************************************************************************/

