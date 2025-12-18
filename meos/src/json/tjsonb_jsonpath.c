/*****************************************************************************
 *
 * This MobilityDB code is provided under The PostgreSQL License.
 * Copyright (c) 2016-2025, Université libre de Bruxelles and MobilityDB
 * contributors
 *
 * MobilityDB includes portions of PostGIS version 3 source code released
 * under the GNU General Public License (GPLv2 or later).
 * Copyright (c) 2001-2025, PostGIS contributors
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
 * @file
 * @brief Basic functions for temporal JSONB
 */

/* C */
#include <assert.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
/* PostgreSQL */
#include <postgres.h>
#include "common/hashfn.h"
#include "common/int.h"
#include "nodes/pg_list.h"
#include "nodes/miscnodes.h"
#include "utils/datetime.h"
#include "utils/json.h"
#include "utils/jsonb.h"
#include "utils/jsonpath.h"
/* MEOS */
#include <meos.h>
#include <meos_json.h>
#include <pgtypes.h>
#include "temporal/temporal.h"
#include "temporal/lifting.h"
#include "temporal/type_util.h"
#include "json/tjsonb.h"

/*****************************************************************************/

/*
 * Represents "base object" and its "id" for .keyvalue() evaluation.
 */
typedef struct JsonBaseObjectInfo
{
  JsonbContainer *jbc;
  int id;
} JsonBaseObjectInfo;

/* Callbacks for executeJsonPath() */
typedef JsonbValue *(*JsonPathGetVarCallback) (void *vars, char *varName,
  int varNameLen, JsonbValue *baseObject, int *baseObjectId);
typedef int (*JsonPathCountVarsCallback) (void *vars);

/*
 * Context of jsonpath execution.
 */
typedef struct JsonPathExecContext
{
  void *vars;        /* variables to substitute into jsonpath */
  JsonPathGetVarCallback getVar;  /* callback to extract a given variable
                   * from 'vars' */
  JsonbValue *root;      /* for $ evaluation */
  JsonbValue *current;   /* for @ evaluation */
  JsonBaseObjectInfo baseObject;  /* "base object" for .keyvalue()
                   * evaluation */
  int lastGeneratedObjectId;  /* "id" counter for .keyvalue() evaluation */
  int innermostArraySize; /* for LAST array index evaluation */
  bool laxMode;    /* true for "lax" mode, false for "strict" mode */
  bool ignoreStructuralErrors; /* with "true" structural errors such as
    * absence of required json item or unexpected json item type are ignored */
  bool throwErrors;  /* with "false" all suppressible errors are suppressed */
  bool useTz;
} JsonPathExecContext;

/* Context for LIKE_REGEX execution. */
typedef struct JsonLikeRegexContext
{
  text *regex;
  int cflags;
} JsonLikeRegexContext;

/* Result of jsonpath predicate evaluation */
typedef enum JsonPathBool
{
  jpbFalse = 0,
  jpbTrue = 1,
  jpbUnknown = 2
} JsonPathBool;

/* Result of jsonpath expression evaluation */
typedef enum JsonPathExecResult
{
  jperOk = 0,
  jperNotFound = 1,
  jperError = 2
} JsonPathExecResult;

#define jperIsError(jper)      ((jper) == jperError)

/*
 * List of jsonb values with shortcut for single-value list.
 */
typedef struct JsonValueList
{
  JsonbValue *singleton;
  List       *list;
} JsonValueList;

typedef struct JsonValueListIterator
{
  JsonbValue *value;
  List       *list;
  ListCell   *next;
} JsonValueListIterator;

/* Structures for JSON_TABLE execution  */

/*
 * Struct holding the result of jsonpath evaluation, to be used as source row
 * for JsonTableGetValue() which in turn computes the values of individual
 * JSON_TABLE columns.
 */
typedef struct JsonTablePlanRowSource
{
  Datum value;
  bool isnull;
} JsonTablePlanRowSource;

/*
 * State of evaluation of row pattern derived by applying jsonpath given in
 * a JsonTablePlan to an input document given in the parent TableFunc.
 */
typedef struct JsonTablePlanState
{
  /* Original plan */
  JsonTablePlan *plan;

  /* The following fields are only valid for JsonTablePathScan plans */

  /* jsonpath to evaluate against the input doc to get the row pattern */
  JsonPath *path;

  /*
   * Memory context to use when evaluating the row pattern from the jsonpath
   */
  MemoryContext mcxt;

  /* PASSING arguments passed to jsonpath executor */
  List *args;

  /* List and iterator of jsonpath result values */
  JsonValueList found;
  JsonValueListIterator iter;

  /* Currently selected row for JsonTableGetValue() to use */
  JsonTablePlanRowSource current;

  /* Counter for ORDINAL columns */
  int ordinal;

  /* Nested plan, if any */
  struct JsonTablePlanState *nested;

  /* Left sibling, if any */
  struct JsonTablePlanState *left;

  /* Right sibling, if any */
  struct JsonTablePlanState *right;

  /* Parent plan, if this is a nested plan */
  struct JsonTablePlanState *parent;
} JsonTablePlanState;

/* Random number to identify JsonTableExecContext for sanity checking */
#define JSON_TABLE_EXEC_CONTEXT_MAGIC    418352867

typedef struct JsonTableExecContext
{
  int magic;

  /* State of the plan providing a row evaluated from "root" jsonpath */
  JsonTablePlanState *rootplanstate;

  /*
   * Per-column JsonTablePlanStates for all columns including the nested
   * ones.
   */
  JsonTablePlanState **colplanstates;
} JsonTableExecContext;

/* strict/lax flags is decomposed into four [un]wrap/error flags */
#define jspStrictAbsenceOfErrors(cxt)  (!(cxt)->laxMode)
#define jspAutoUnwrap(cxt)        ((cxt)->laxMode)
#define jspAutoWrap(cxt)        ((cxt)->laxMode)
#define jspIgnoreStructuralErrors(cxt)  ((cxt)->ignoreStructuralErrors)
#define jspThrowErrors(cxt)        ((cxt)->throwErrors)

typedef JsonPathBool (*JsonPathPredicateCallback) (JsonPathItem *jsp,
  JsonbValue *larg, JsonbValue *rarg, void *param);
typedef Numeric (*BinaryArithmFunc) (Numeric num1, Numeric num2, bool *error);

/*****************************************************************************/

extern JsonPathExecResult executeJsonPath(JsonPath *path, void *vars,
  JsonPathGetVarCallback getVar, JsonPathCountVarsCallback countVars,
  Jsonb *json, bool throwErrors, JsonValueList *result, bool useTz);
extern JsonPathExecResult executeItem(JsonPathExecContext *cxt,
  JsonPathItem *jsp, JsonbValue *jb, JsonValueList *found);
extern JsonPathExecResult executeItemOptUnwrapTarget(JsonPathExecContext *cxt,
  JsonPathItem *jsp, JsonbValue *jb, JsonValueList *found, bool unwrap);
extern JsonPathExecResult executeItemUnwrapTargetArray(JsonPathExecContext *cxt,
  JsonPathItem *jsp, JsonbValue *jb, JsonValueList *found,
    bool unwrapElements);
extern JsonPathExecResult executeNextItem(JsonPathExecContext *cxt,
  JsonPathItem *cur, JsonPathItem *next, JsonbValue *v, JsonValueList *found,
    bool copy);
extern JsonPathExecResult executeItemOptUnwrapResult(JsonPathExecContext *cxt,
  JsonPathItem *jsp, JsonbValue *jb, bool unwrap, JsonValueList *found);
extern JsonPathExecResult executeItemOptUnwrapResultNoThrow(
  JsonPathExecContext *cxt, JsonPathItem *jsp, JsonbValue *jb, bool unwrap,
  JsonValueList *found);
extern JsonPathBool executeBoolItem(JsonPathExecContext *cxt,
  JsonPathItem *jsp, JsonbValue *jb, bool canHaveNext);
extern JsonPathBool executeNestedBoolItem(JsonPathExecContext *cxt,
  JsonPathItem *jsp, JsonbValue *jb);
extern JsonPathExecResult executeAnyItem(JsonPathExecContext *cxt,
  JsonPathItem *jsp, JsonbContainer *jbc, JsonValueList *found,
  uint32 level, uint32 first, uint32 last, bool ignoreStructuralErrors,
  bool unwrapNext);
extern JsonPathBool executePredicate(JsonPathExecContext *cxt,
  JsonPathItem *pred, JsonPathItem *larg, JsonPathItem *rarg, JsonbValue *jb,
  bool unwrapRightArg, JsonPathPredicateCallback exec, void *param);
extern JsonPathExecResult executeBinaryArithmExpr(JsonPathExecContext *cxt,
  JsonPathItem *jsp, JsonbValue *jb, BinaryArithmFunc func,
    JsonValueList *found);
extern JsonPathExecResult executeUnaryArithmExpr(JsonPathExecContext *cxt,
  JsonPathItem *jsp, JsonbValue *jb, PGFunction func, JsonValueList *found);
extern JsonPathBool executeStartsWith(JsonPathItem *jsp, JsonbValue *whole,
  JsonbValue *initial, void *param);
extern JsonPathBool executeLikeRegex(JsonPathItem *jsp, JsonbValue *str,
  JsonbValue *rarg, void *param);
extern JsonPathExecResult executeNumericItemMethod(JsonPathExecContext *cxt,
  JsonPathItem *jsp, JsonbValue *jb, bool unwrap, PGFunction func,
  JsonValueList *found);
extern JsonPathExecResult executeDateTimeMethod(JsonPathExecContext *cxt,
  JsonPathItem *jsp, JsonbValue *jb, JsonValueList *found);
extern JsonPathExecResult executeKeyValueMethod(JsonPathExecContext *cxt,
  JsonPathItem *jsp, JsonbValue *jb, JsonValueList *found);
extern JsonPathExecResult appendBoolResult(JsonPathExecContext *cxt,
  JsonPathItem *jsp, JsonValueList *found, JsonPathBool res);
extern void getJsonPathItem(JsonPathExecContext *cxt, JsonPathItem *item,
  JsonbValue *value);
extern JsonbValue *GetJsonPathVar(void *cxt, char *varName, int varNameLen,
  JsonbValue *baseObject, int *baseObjectId);
extern int  CountJsonPathVars(void *cxt);
extern void JsonItemFromDatum(Datum val, Oid typid, int32 typmod,
  JsonbValue *res);
extern void JsonbValueInitNumericDatum(JsonbValue *jbv, Datum num);
extern void getJsonPathVariable(JsonPathExecContext *cxt,
  JsonPathItem *variable, JsonbValue *value);
extern int countVariablesFromJsonb(void *varsJsonb);
extern JsonbValue *getJsonPathVariableFromJsonb(void *varsJsonb, char *varName,
  int varNameLength, JsonbValue *baseObject, int *baseObjectId);
extern int JsonbArraySize(JsonbValue *jb);
extern JsonPathBool executeComparison(JsonPathItem *cmp, JsonbValue *lv,
  JsonbValue *rv, void *p);
extern JsonPathBool compareItems(int32 op, JsonbValue *jb1, JsonbValue *jb2,
  bool useTz);
extern int  compareNumeric(Numeric a, Numeric b);
extern JsonbValue *copyJsonbValue(JsonbValue *src);
extern JsonPathExecResult getArrayIndex(JsonPathExecContext *cxt,
  JsonPathItem *jsp, JsonbValue *jb, int32 *index);
extern JsonBaseObjectInfo setBaseObject(JsonPathExecContext *cxt,
  JsonbValue *jbv, int32 id);
extern void JsonValueListClear(JsonValueList *jvl);
extern void JsonValueListAppend(JsonValueList *jvl, JsonbValue *jbv);
extern int JsonValueListLength(const JsonValueList *jvl);
extern bool JsonValueListIsEmpty(JsonValueList *jvl);
extern JsonbValue *JsonValueListHead(JsonValueList *jvl);
extern List *JsonValueListGetList(JsonValueList *jvl);
extern void JsonValueListInitIterator(const JsonValueList *jvl,
  JsonValueListIterator *it);
extern JsonbValue *JsonValueListNext(const JsonValueList *jvl,
  JsonValueListIterator *it);
extern JsonbValue *JsonbInitBinary(JsonbValue *jbv, Jsonb *jb);
extern int JsonbType(JsonbValue *jb);
extern JsonbValue *getScalar(JsonbValue *scalar, enum jbvType type);
extern JsonbValue *wrapItemsInArray(const JsonValueList *items);
extern int  compareDatetime(Datum val1, Oid typid1, Datum val2, Oid typid2,
  bool useTz, bool *cast_error);
extern void checkTimezoneIsUsedForCast(bool useTz, const char *type1,
  const char *type2);

// extern void JsonTableInitOpaque(TableFuncScanState *state, int natts);
// extern JsonTablePlanState *JsonTableInitPlan(JsonTableExecContext *cxt,
                       // JsonTablePlan *plan,
                       // JsonTablePlanState *parentstate,
                       // List *args,
                       // MemoryContext mcxt);
// extern void JsonTableSetDocument(TableFuncScanState *state, Datum value);
// extern void JsonTableResetRowPattern(JsonTablePlanState *planstate, Datum item);
// extern bool JsonTableFetchRow(TableFuncScanState *state);
// extern Datum JsonTableGetValue(TableFuncScanState *state, int colnum,
                 // Oid typid, int32 typmod, bool *isnull);
// extern void JsonTableDestroyOpaque(TableFuncScanState *state);
// extern bool JsonTablePlanScanNextRow(JsonTablePlanState *planstate);
// extern void JsonTableResetNestedPlan(JsonTablePlanState *planstate);
// extern bool JsonTablePlanJoinNextRow(JsonTablePlanState *planstate);
// extern bool JsonTablePlanNextRow(JsonTablePlanState *planstate);

/*****************************************************************************/

/*
 * Initialize a binary JsonbValue with the given jsonb container.
 */
JsonbValue *
JsonbInitBinary(JsonbValue *jbv, Jsonb *jb)
{
  jbv->type = jbvBinary;
  jbv->val.binary.data = &jb->root;
  jbv->val.binary.len = VARSIZE_ANY_EXHDR(jb);
  return jbv;
}

/* Save base object and its id needed for the execution of .keyvalue(). */
JsonBaseObjectInfo
setBaseObject(JsonPathExecContext *cxt, JsonbValue *jbv, int32 id)
{
  JsonBaseObjectInfo baseObject = cxt->baseObject;
  cxt->baseObject.jbc = jbv->type != jbvBinary ? NULL :
    (JsonbContainer *) jbv->val.binary.data;
  cxt->baseObject.id = id;
  return baseObject;
}

/*
 * Get the value of variable passed to jsonpath executor
 */
void
getJsonPathVariable(JsonPathExecContext *cxt, JsonPathItem *variable,
  JsonbValue *value)
{
  int varNameLength;
  JsonbValue baseObject;
  int baseObjectId;
  JsonbValue *v;

  Assert(variable->type == jpiVariable);
  char *varName = jspGetString(variable, &varNameLength);
  if (cxt->vars == NULL ||
      (v = cxt->getVar(cxt->vars, varName, varNameLength, &baseObject,
      &baseObjectId)) == NULL)
  {
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "could not find jsonpath variable \"%s\"",
      pnstrdup(varName, varNameLength));
  }

  if (baseObjectId > 0)
  {
    *value = *v;
    setBaseObject(&baseObject, baseObjectId);
  }
}

/*
 * Convert jsonpath's scalar or variable node to actual jsonb value.
 *
 * If node is a variable then its id returned, otherwise 0 returned.
 */
void
getJsonPathItem(JsonPathExecContext *cxt, JsonPathItem *item,
  JsonbValue *value)
{
  switch (item->type)
  {
    case jpiNull:
      value->type = jbvNull;
      break;
    case jpiBool:
      value->type = jbvBool;
      value->val.boolean = jspGetBool(item);
      break;
    case jpiNumeric:
      value->type = jbvNumeric;
      value->val.numeric = jspGetNumeric(item);
      break;
    case jpiString:
      value->type = jbvString;
      value->val.string.val = jspGetString(item, &value->val.string.len);
      break;
    case jpiVariable:
      getJsonPathVariable(item, value);
      return;
    default:
      meos_error(ERROR, MEOS_ERR_INTERNAL_ERROR,
        "unexpected jsonpath item type");
  }
}

void
JsonValueListAppend(JsonValueList *jvl, JsonbValue *jbv)
{
  if (jvl->singleton)
  {
    jvl->list = list_make2(jvl->singleton, jbv);
    jvl->singleton = NULL;
  }
  else if (!jvl->list)
    jvl->singleton = jbv;
  else
    jvl->list = lappend(jvl->list, jbv);
}

JsonbValue *
copyJsonbValue(JsonbValue *src)
{
  JsonbValue *dst = palloc(sizeof(*dst));
  *dst = *src;
  return dst;
}

/*
 * Implementation of several jsonpath nodes:
 *  - jpiAny (.** accessor),
 *  - jpiAnyKey (.* accessor),
 *  - jpiAnyArray ([*] accessor)
 */
JsonPathExecResult
executeAnyItem(JsonPathExecContext *cxt, JsonPathItem *jsp, JsonbContainer *jbc,
  JsonValueList *found, uint32 level, uint32 first, uint32 last,
  bool ignoreStructuralErrors, bool unwrapNext)
{
  JsonPathExecResult res = jperNotFound;

  // check_stack_depth();

  if (level > last)
    return res;

  int32 r;
  JsonbValue v;
  JsonbIterator *it = JsonbIteratorInit(jbc);
  /* Recursively iterate over jsonb objects/arrays */
  while ((r = JsonbIteratorNext(&it, &v, true)) != WJB_DONE)
  {
    if (r == WJB_KEY)
    {
      r = JsonbIteratorNext(&it, &v, true);
      Assert(r == WJB_VALUE);
    }

    if (r == WJB_VALUE || r == WJB_ELEM)
    {
      if (level >= first ||
        (first == PG_UINT32_MAX && last == PG_UINT32_MAX &&
         v.type != jbvBinary))  /* leaves only requested */
      {
        /* check expression */
        if (jsp)
        {
          if (ignoreStructuralErrors)
          {
            bool savedIgnoreStructuralErrors = cxt->ignoreStructuralErrors;
            cxt->ignoreStructuralErrors = true;
            res = executeItemOptUnwrapTarget(jsp, &v, found, unwrapNext);
            cxt->ignoreStructuralErrors = savedIgnoreStructuralErrors;
          }
          else
            res = executeItemOptUnwrapTarget(jsp, &v, found, unwrapNext);
          if (jperIsError(res))
            break;
          if (res == jperOk && ! found)
            break;
        }
        else if (found)
          JsonValueListAppend(found, copyJsonbValue(&v));
        else
          return jperOk;
      }

      if (level < last && v.type == jbvBinary)
      {
        res = executeAnyItem(jsp, v.val.binary.data, found, level + 1,
           first, last, ignoreStructuralErrors, unwrapNext);
        if (jperIsError(res))
          break;
        if (res == jperOk && found == NULL)
          break;
      }
    }
  }
  return res;
}

/*
 * Unwrap current array item and execute jsonpath for each of its elements.
 */
JsonPathExecResult
executeItemUnwrapTargetArray(JsonPathExecContext *cxt, JsonPathItem *jsp,
  JsonbValue *jb, JsonValueList *found, bool unwrapElements)
{
  if (jb->type != jbvBinary)
  {
    Assert(jb->type != jbvArray);
    meos_error(ERROR, MEOS_ERR_INTERNAL_ERROR,
      "invalid jsonb array value type: %d", jb->type);
    return jperError;
  }
  return executeAnyItem(jsp, jb->val.binary.data, found, 1, 1, 1, false,
    unwrapElements);
}

/*
 * Execute jsonpath with automatic unwrapping of current item in lax mode.
 */
JsonPathExecResult
executeItem(JsonPathExecContext *cxt, JsonPathItem *jsp, JsonbValue *jb,
  JsonValueList *found)
{
  return executeItemOptUnwrapTarget(jsp, jb, found, jspAutoUnwrap(cxt));
}

/*
 * Execute next jsonpath item if exists.  Otherwise put "v" to the "found"
 * list if provided.
 */
JsonPathExecResult
executeNextItem(JsonPathExecContext *cxt, JsonPathItem *cur,
  JsonPathItem *next, JsonbValue *v, JsonValueList *found, bool copy)
{
  JsonPathItem elem;
  bool hasNext;

  if (! cur)
    hasNext = next != NULL;
  else if (next)
    hasNext = jspHasNext(cur);
  else
  {
    next = &elem;
    hasNext = jspGetNext(cur, next);
  }
  if (hasNext)
    return executeItem(next, v, found);
  if (found)
    JsonValueListAppend(found, copy ? copyJsonbValue(v) : v);
  return jperOk;
}

/*
 * Execute unary or binary predicate.
 *
 * Predicates have existence semantics, because their operands are item
 * sequences. Pairs of items from the left and right operand's sequences are
 * checked.  TRUE returned only if any pair satisfying the condition is found.
 * In strict mode, even if the desired pair has already been found, all pairs
 * still need to be examined to check the absence of errors.  If any error
 * occurs, UNKNOWN (analogous to SQL NULL) is returned.
 */
JsonPathBool
executePredicate(JsonPathExecContext *cxt, JsonPathItem *pred,
  JsonPathItem *larg, JsonPathItem *rarg, JsonbValue *jb, bool unwrapRightArg,
  JsonPathPredicateCallback exec, void *param)
{
  JsonPathExecResult res;
  JsonValueListIterator lseqit;
  JsonValueList lseq = {0};
  JsonValueList rseq = {0};
  JsonbValue *lval;
  bool error = false;
  bool found = false;

  /* Left argument is always auto-unwrapped. */
  res = executeItemOptUnwrapResultNoThrow(larg, jb, true, &lseq);
  if (jperIsError(res))
    return jpbUnknown;

  if (rarg)
  {
    /* Right argument is conditionally auto-unwrapped. */
    res = executeItemOptUnwrapResultNoThrow(rarg, jb, unwrapRightArg, &rseq);
    if (jperIsError(res))
      return jpbUnknown;
  }

  JsonValueListInitIterator(&lseq, &lseqit);
  while ((lval = JsonValueListNext(&lseq, &lseqit)))
  {
    JsonbValue *rval;
    bool first = true;
    JsonValueListIterator rseqit;
    JsonValueListInitIterator(&rseq, &rseqit);
    if (rarg)
      rval = JsonValueListNext(&rseq, &rseqit);
    else
      rval = NULL;
    /* Loop over right arg sequence or do single pass otherwise */
    while (rarg ? (rval != NULL) : first)
    {
      JsonPathBool res = exec(pred, lval, rval, param);
      if (res == jpbUnknown)
      {
        if (jspStrictAbsenceOfErrors(cxt))
          return jpbUnknown;
        error = true;
      }
      else if (res == jpbTrue)
      {
        if (!jspStrictAbsenceOfErrors(cxt))
          return jpbTrue;
        found = true;
      }
      first = false;
      if (rarg)
        rval = JsonValueListNext(&rseq, &rseqit);
    }
  }
  if (found)          /* possible only in strict mode */
    return jpbTrue;
  if (error)          /* possible only in lax mode */
    return jpbUnknown;
  return jpbFalse;
}

/* Execute boolean-valued jsonpath expression. */
JsonPathBool
executeBoolItem(JsonPathExecContext *cxt, JsonPathItem *jsp, JsonbValue *jb,
  bool canHaveNext)
{
  JsonPathItem larg, rarg;
  JsonPathBool res, res2;

  /* since this function recurses, it could be driven to stack overflow */
  // check_stack_depth();

  if (!canHaveNext && jspHasNext(jsp))
  {
    meos_error(ERROR,´MEOS_ERR_INTERNAL_ERROR,
      "boolean jsonpath item cannot have next item");
    return jpbUnknown;
  }

  switch (jsp->type)
  {
    case jpiAnd:
      jspGetLeftArg(jsp, &larg);
      res = executeBoolItem(&larg, jb, false);
      if (res == jpbFalse)
        return jpbFalse;
      /* SQL/JSON says that we should check second arg in case of jperError */
      jspGetRightArg(jsp, &rarg);
      res2 = executeBoolItem(&rarg, jb, false);
      return res2 == jpbTrue ? res : res2;

    case jpiOr:
      jspGetLeftArg(jsp, &larg);
      res = executeBoolItem(&larg, jb, false);
      if (res == jpbTrue)
        return jpbTrue;
      jspGetRightArg(jsp, &rarg);
      res2 = executeBoolItem(&rarg, jb, false);
      return res2 == jpbFalse ? res : res2;

    case jpiNot:
      jspGetArg(jsp, &larg);
      res = executeBoolItem(&larg, jb, false);
      if (res == jpbUnknown)
        return jpbUnknown;
      return res == jpbTrue ? jpbFalse : jpbTrue;

    case jpiIsUnknown:
      jspGetArg(jsp, &larg);
      res = executeBoolItem(&larg, jb, false);
      return res == jpbUnknown ? jpbTrue : jpbFalse;

    case jpiEqual:
    case jpiNotEqual:
    case jpiLess:
    case jpiGreater:
    case jpiLessOrEqual:
    case jpiGreaterOrEqual:
      jspGetLeftArg(jsp, &larg);
      jspGetRightArg(jsp, &rarg);
      return executePredicate(jsp, &larg, &rarg, jb, true, executeComparison,
        cxt);

    case jpiStartsWith:    /* 'whole STARTS WITH initial' */
      jspGetLeftArg(jsp, &larg);  /* 'whole' */
      jspGetRightArg(jsp, &rarg); /* 'initial' */
      return executePredicate(jsp, &larg, &rarg, jb, false, executeStartsWith,
        NULL);

    case jpiLikeRegex:    /* 'expr LIKE_REGEX pattern FLAGS flags' */
      {
        /*
         * 'expr' is a sequence-returning expression.  'pattern' is a
         * regex string literal.  SQL/JSON standard requires XQuery
         * regexes, but we use Postgres regexes here.  'flags' is a
         * string literal converted to integer flags at compile-time.
         */
        JsonLikeRegexContext lrcxt = {0};
        jspInitByBuffer(&larg, jsp->base, jsp->content.like_regex.expr);
        return executePredicate(jsp, &larg, NULL, jb, false, executeLikeRegex,
          &lrcxt);
      }

    case jpiExists:
      jspGetArg(jsp, &larg);

      if (jspStrictAbsenceOfErrors(cxt))
      {
        /*
         * In strict mode we must get a complete list of values to
         * check that there are no errors at all.
         */
        JsonValueList vals = {0};
        JsonPathExecResult res =
          executeItemOptUnwrapResultNoThrow(&larg, jb, false, &vals);
        if (jperIsError(res))
          return jpbUnknown;
        return JsonValueListIsEmpty(&vals) ? jpbFalse : jpbTrue;
      }
      else
      {
        JsonPathExecResult res =
          executeItemOptUnwrapResultNoThrow(&larg, jb, false, NULL);
        if (jperIsError(res))
          return jpbUnknown;
        return res == jperOk ? jpbTrue : jpbFalse;
      }

    default:
      meos_error(ERROR, MEOS_ERR_INTERNAL_ERROR,
        "invalid boolean jsonpath item type: %d", jsp->type);
      return jpbUnknown;
  }
}

/*
 * Main jsonpath executor function: walks on jsonpath structure, finds
 * relevant parts of jsonb and evaluates expressions over them.
 * When 'unwrap' is true current SQL/JSON item is unwrapped if it is an array.
 */
JsonPathExecResult
executeItemOptUnwrapTarget(JsonPathExecContext *cxt, JsonPathItem *jsp,
  JsonbValue *jb, JsonValueList *found, bool unwrap)
{
  JsonPathItem elem;
  JsonPathExecResult res = jperNotFound;
  JsonBaseObjectInfo baseObject;

  // check_stack_depth();
  // CHECK_FOR_INTERRUPTS();

  switch (jsp->type)
  {
    case jpiNull:
    case jpiBool:
    case jpiNumeric:
    case jpiString:
    case jpiVariable:
      {
        bool hasNext = jspGetNext(jsp, &elem);
        if (! hasNext && ! found && jsp->type != jpiVariable)
        {
          /*
           * Skip evaluation, but not for variables.  We must
           * trigger an error for the missing variable.
           */
          res = jperOk;
          break;
        }
        JsonbValue vbuf;
        JsonbValue *v = hasNext ? &vbuf : palloc(sizeof(*v));
        baseObject = cxt->baseObject;
        getJsonPathItem(jsp, v);
        res = executeNextItem(jsp, &elem, v, found, hasNext);
        cxt->baseObject = baseObject;
      }
      break;

      /* all boolean item types: */
    case jpiAnd:
    case jpiOr:
    case jpiNot:
    case jpiIsUnknown:
    case jpiEqual:
    case jpiNotEqual:
    case jpiLess:
    case jpiGreater:
    case jpiLessOrEqual:
    case jpiGreaterOrEqual:
    case jpiExists:
    case jpiStartsWith:
    case jpiLikeRegex:
      {
        JsonPathBool st = executeBoolItem(jsp, jb, true);
        res = appendBoolResult(jsp, found, st);
        break;
      }

    case jpiAdd:
      return numeric_add_opt_error(jsp, jb, found);

    case jpiSub:
      return numeric_sub_opt_error(jsp, jb, found);

    case jpiMul:
      return numeric_mul_opt_error(jsp, jb, found);

    case jpiDiv:
      return numeric_div_opt_error(jsp, jb, found);

    case jpiMod:
      return numeric_mod_opt_error(jsp, jb, found);

    case jpiPlus:
      return executeUnaryArithmExpr(jsp, jb, NULL, found);

    case jpiMinus:
      return pg_numeric_uminus(jsp, jb);

    case jpiAnyArray:
      if (JsonbType(jb) == jbvArray)
      {
        bool hasNext = jspGetNext(jsp, &elem);
        res = executeItemUnwrapTargetArray(hasNext ? &elem : NULL, jb, found,
          jspAutoUnwrap(cxt));
      }
      else if (jspAutoWrap(cxt))
        res = executeNextItem(jsp, NULL, jb, found, true);
      else if (!jspIgnoreStructuralErrors(cxt))
      {
        meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
          "jsonpath wildcard array accessor can only be applied to an array");
        return jperError;
      }
      break;

    case jpiAnyKey:
      if (JsonbType(jb) == jbvObject)
      {
        bool hasNext = jspGetNext(jsp, &elem);
        if (jb->type != jbvBinary)
        {
          meos_error(ERROR, MEOS_ERR_INTERNAL_ERROR,
            "invalid jsonb object type: %d", jb->type);
          return jperError;
        }
        return executeAnyItem(hasNext ? &elem : NULL, jb->val.binary.data,
           found, 1, 1, 1, false, jspAutoUnwrap(cxt));
      }
      else if (unwrap && JsonbType(jb) == jbvArray)
        return executeItemUnwrapTargetArray(jsp, jb, found, false);
      else if (! jspIgnoreStructuralErrors(cxt))
      {
        Assert(found);
        meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
          "jsonpath wildcard member accessor can only be applied to an object");
        return jperError;
      }
      break;

    case jpiIndexArray:
      if (JsonbType(jb) == jbvArray || jspAutoWrap(cxt))
      {
        int innermostArraySize = cxt->innermostArraySize;
        int i;
        int size = JsonbArraySize(jb);
        bool singleton = size < 0;
        bool hasNext = jspGetNext(jsp, &elem);
        if (singleton)
          size = 1;
        cxt->innermostArraySize = size; /* for LAST evaluation */
        for (i = 0; i < jsp->content.array.nelems; i++)
        {
          JsonPathItem from, to;
          int32 index, index_from, index_to;
          bool range = jspGetArraySubscript(jsp, &from, &to, i);
          res = getArrayIndex(&from, jb, &index_from);
          if (jperIsError(res))
            break;
          if (range)
          {
            res = getArrayIndex(&to, jb, &index_to);
            if (jperIsError(res))
              break;
          }
          else
            index_to = index_from;
          if (!jspIgnoreStructuralErrors(cxt) &&
              (index_from < 0 || index_from > index_to || index_to >= size))
          {
            meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
              "jsonpath array subscript is out of bounds");
            return jperError;
          }

          if (index_from < 0)
            index_from = 0;
          if (index_to >= size)
            index_to = size - 1;
          res = jperNotFound;
          for (index = index_from; index <= index_to; index++)
          {
            JsonbValue *v;
            bool copy;
            if (singleton)
            {
              v = jb;
              copy = true;
            }
            else
            {
              v = getIthJsonbValueFromContainer(jb->val.binary.data,
                (uint32) index);
              if (v == NULL)
                continue;
              copy = false;
            }

            if (!hasNext && !found)
              return jperOk;
            res = executeNextItem(jsp, &elem, v, found, copy);
            if (jperIsError(res))
              break;
            if (res == jperOk && !found)
              break;
          }
          if (jperIsError(res))
            break;
          if (res == jperOk && !found)
            break;
        }
        cxt->innermostArraySize = innermostArraySize;
      }
      else if (!jspIgnoreStructuralErrors(cxt))
      {
        meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
          "jsonpath array accessor can only be applied to an array");
        return jperError;
      }
      break;

    case jpiAny:
      {
        bool hasNext = jspGetNext(jsp, &elem);
        /* first try without any intermediate steps */
        if (jsp->content.anybounds.first == 0)
        {
          bool savedIgnoreStructuralErrors = cxt->ignoreStructuralErrors;
          cxt->ignoreStructuralErrors = true;
          res = executeNextItem(jsp, &elem, jb, found, true);
          cxt->ignoreStructuralErrors = savedIgnoreStructuralErrors;
          if (res == jperOk && !found)
            break;
        }
        if (jb->type == jbvBinary)
          res = executeAnyItem(hasNext ? &elem : NULL,
            jb->val.binary.data, found, 1, jsp->content.anybounds.first,
            jsp->content.anybounds.last, true, jspAutoUnwrap(cxt));
        break;
      }

    case jpiKey:
      if (JsonbType(jb) == jbvObject)
      {
        JsonbValue key;
        key.type = jbvString;
        key.val.string.val = jspGetString(jsp, &key.val.string.len);
        JsonbValue *v = findJsonbValueFromContainer(jb->val.binary.data,
          JB_FOBJECT, &key);
        if (v != NULL)
        {
          res = executeNextItem(jsp, NULL, v, found, false);
          /* free value if it was not added to found list */
          if (jspHasNext(jsp) || !found)
            pfree(v);
        }
        else if (!jspIgnoreStructuralErrors(cxt))
        {
          Assert(found);
          if (!jspThrowErrors(cxt))
            return jperError;
          meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
            "JSON object does not contain key \"%s\"",
            pnstrdup(key.val.string.val, key.val.string.len));
          return jperError;
        }
      }
      else if (unwrap && JsonbType(jb) == jbvArray)
        return executeItemUnwrapTargetArray(jsp, jb, found, false);
      else if (!jspIgnoreStructuralErrors(cxt))
      {
        Assert(found);
        meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
          "jsonpath member accessor can only be applied to an object");
        return jperError;
      }
      break;

    case jpiCurrent:
      res = executeNextItem(jsp, NULL, cxt->current, found, true);
      break;

    case jpiRoot:
      jb = cxt->root;
      baseObject = setBaseObject(jb, 0);
      res = executeNextItem(jsp, NULL, jb, found, true);
      cxt->baseObject = baseObject;
      break;

    case jpiFilter:
      {
        JsonPathBool st;
        if (unwrap && JsonbType(jb) == jbvArray)
          return executeItemUnwrapTargetArray(jsp, jb, found, false);
        jspGetArg(jsp, &elem);
        st = executeNestedBoolItem(&elem, jb);
        if (st != jpbTrue)
          res = jperNotFound;
        else
          res = executeNextItem(jsp, NULL, jb, found, true);
        break;
      }

    case jpiType:
      {
        JsonbValue *jbv = palloc(sizeof(*jbv));
        jbv->type = jbvString;
        jbv->val.string.val = pstrdup(JsonbTypeName(jb));
        jbv->val.string.len = strlen(jbv->val.string.val);
        res = executeNextItem(jsp, NULL, jbv, found, false);
      }
      break;

    case jpiSize:
      {
        int size = JsonbArraySize(jb);
        if (size < 0)
        {
          if (!jspAutoWrap(cxt))
          {
            if (!jspIgnoreStructuralErrors(cxt))
            {
              meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
                "jsonpath item method .%s() can only be applied to an array",
                jspOperationName(jsp->type));
              return jperError;
            }
            break;
          }
          size = 1;
        }
        jb = palloc(sizeof(*jb));
        jb->type = jbvNumeric;
        jb->val.numeric = int64_to_numeric(size);
        res = executeNextItem(jsp, NULL, jb, found, false);
      }
      break;

    case jpiAbs:
      return pg_numeric_abs(jsp, jb, unwrap, , found);

    case jpiFloor:
      return pg_numeric_floor(jsp, jb, unwrap, found);

    case jpiCeiling:
      return pg_numeric_ceil(jsp, jb, unwrap, , found);

    case jpiDouble:
      {
        JsonbValue jbv;
        if (unwrap && JsonbType(jb) == jbvArray)
          return executeItemUnwrapTargetArray(jsp, jb, found, false);
        if (jb->type == jbvNumeric)
        {
          char *tmp = pg_numeric_out(jb->val.numeric);
          double val = float64_in(tmp);
          if (val == LONG_MAX)
          {
            meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
              "argument \"%s\" of jsonpath item method .%s() is invalid for type %s",
              tmp, jspOperationName(jsp->type), "double precision");
            return jperError;
          }
          if (isinf(val) || isnan(val))
          {
            meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
              "NaN or Infinity is not allowed for jsonpath item method .%s()",
              jspOperationName(jsp->type));
            return jperError;
          }
          res = jperOk;
        }
        else if (jb->type == jbvString)
        {
          /* cast string as double */
          char *tmp = pnstrdup(jb->val.string.val, jb->val.string.len);
          double val = float8_in(tmp);
          if (isinf(val) || isnan(val))
          {
            meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
              "NaN or Infinity is not allowed for jsonpath item method .%s()",
              jspOperationName(jsp->type));
            return jperError;
          }
          jb = &jbv;
          jb->type = jbvNumeric;
          jb->val.numeric = float8_to_numeric(val);
          res = jperOk;
        }
        if (res == jperNotFound)
        {
          meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
            "jsonpath item method .%s() can only be applied to a string or numeric value",
            jspOperationName(jsp->type));
          return jperError;
        }
        res = executeNextItem(jsp, NULL, jb, found, true);
      }
      break;

    case jpiDatetime:
    case jpiDate:
    case jpiTime:
    case jpiTimeTz:
    case jpiTimestamp:
    case jpiTimestampTz:
      if (unwrap && JsonbType(jb) == jbvArray)
        return executeItemUnwrapTargetArray(jsp, jb, found, false);
      return executeDateTimeMethod(jsp, jb, found);

    case jpiKeyValue:
      if (unwrap && JsonbType(jb) == jbvArray)
        return executeItemUnwrapTargetArray(jsp, jb, found, false);
      return executeKeyValueMethod(jsp, jb, found);

    case jpiLast:
      {
        JsonbValue tmpjbv, *lastjbv;
        int last;
        bool hasNext = jspGetNext(jsp, &elem);
        if (cxt->innermostArraySize < 0)
        {
          meos_error(ERROR, MEOS_ERR_INTERNAL_ERROR,
            "evaluating jsonpath LAST outside of array subscript");
          return jperError;
        }
        if (!hasNext && !found)
        {
          res = jperOk;
          break;
        }
        last = cxt->innermostArraySize - 1;
        lastjbv = hasNext ? &tmpjbv : palloc(sizeof(*lastjbv));
        lastjbv->type = jbvNumeric;
        lastjbv->val.numeric = int64_to_numeric(last);
        res = executeNextItem(jsp, &elem, lastjbv, found, hasNext);
      }
      break;

    case jpiBigint:
      {
        JsonbValue jbv;
        Datum datum;
        if (unwrap && JsonbType(jb) == jbvArray)
          return executeItemUnwrapTargetArray(jsp, jb, found, false);
        if (jb->type == jbvNumeric)
        {
          bool have_error;
          int64 val = pg_numeric_int8_opt_error(jb->val.numeric, &have_error);
          if (have_error)
          {
            meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
              "argument \"%s\" of jsonpath item method .%s() is invalid for type %s",
              pg_numeric_out(jb->val.numeric),
              jspOperationName(jsp->type), "bigint");
            return jperError;
          }
          datum = Int64GetDatum(val);
          res = jperOk;
        }
        else if (jb->type == jbvString)
        {
          /* cast string as bigint */
          char *tmp = pnstrdup(jb->val.string.val, jb->val.string.len);
          int64 value = int64_in(tmp);
          if (value == LONG_MAX)
          {
            meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
              "argument \"%s\" of jsonpath item method .%s() is invalid for type %s",
              tmp, jspOperationName(jsp->type), "bigint");
            return jperError;
          }
          res = jperOk;
        }
        if (res == jperNotFound)
        {
          meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
            "jsonpath item method .%s() can only be applied to a string or numeric value",
            jspOperationName(jsp->type));
          return jperError;
        }
        jb = &jbv;
        jb->type = jbvNumeric;
        jb->val.numeric = int64_to_numeric(value);
        res = executeNextItem(jsp, NULL, jb, found, true);
      }
      break;

    case jpiBoolean:
      {
        JsonbValue jbv;
        bool bval;
        if (unwrap && JsonbType(jb) == jbvArray)
          return executeItemUnwrapTargetArray(jsp, jb, found, false);
        if (jb->type == jbvBool)
        {
          bval = jb->val.boolean;
          res = jperOk;
        }
        else if (jb->type == jbvNumeric)
        {
          char *tmp = pg_numeric_out(jb->val.numeric);
          ival = int32_in(tmp);
          if (ival == INT_MAX)
          {
            meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
              "argument \"%s\" of jsonpath item method .%s() is invalid for type %s",
              tmp, jspOperationName(jsp->type), "boolean");
            return jperError;
          }
          ival = DatumGetInt32(datum);
          if (ival == 0)
            bval = false;
          else
            bval = true;
          res = jperOk;
        }
        else if (jb->type == jbvString)
        {
          /* cast string as boolean */
          char *tmp = pnstrdup(jb->val.string.val, jb->val.string.len);

          if (!parse_bool(tmp, &bval))
          {
            meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
              "argument \"%s\" of jsonpath item method .%s() is invalid for type %s",
              tmp, jspOperationName(jsp->type), "boolean");
            return jperError;
          }
          res = jperOk;
        }

        if (res == jperNotFound)
        {
          meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
            "jsonpath item method .%s() can only be applied to a boolean, string, or numeric value",
            jspOperationName(jsp->type));
          return jperError;
        }
        jb = &jbv;
        jb->type = jbvBool;
        jb->val.boolean = bval;
        res = executeNextItem(jsp, NULL, jb, found, true);
      }
      break;

    case jpiDecimal:
    case jpiNumber:
      {
        JsonbValue jbv;
        Numeric num;
        char *numstr = NULL;
        if (unwrap && JsonbType(jb) == jbvArray)
          return executeItemUnwrapTargetArray(jsp, jb, found, false);

        if (jb->type == jbvNumeric)
        {
          num = jb->val.numeric;
          if (numeric_is_nan(num) || numeric_is_inf(num))
          {
            meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
              "NaN or Infinity is not allowed for jsonpath item method .%s()",
              jspOperationName(jsp->type));
            return jperError;
          }

          if (jsp->type == jpiDecimal)
            numstr = pg_numeric_out(num);
          res = jperOk;
        }
        else if (jb->type == jbvString)
        {
          /* cast string as number */
          bool noerr = true;
          numstr = pnstrdup(jb->val.string.val, jb->val.string.len);
          Numeric num = pg_numeric_in(numstr, -1); // TODO Check for error
          if (! noerr)
          {
            meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
              "argument \"%s\" of jsonpath item method .%s() is invalid for type %s",
              numstr, jspOperationName(jsp->type), "numeric");
            return jperError;
          }
          num = DatumGetNumeric(datum);
          if (numeric_is_nan(num) || numeric_is_inf(num))
          {
            meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
              "NaN or Infinity is not allowed for jsonpath item method .%s()",
              jspOperationName(jsp->type));
            return jperError;
          }
          res = jperOk;
        }
        if (res == jperNotFound)
       ´{
          meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
            "jsonpath item method .%s() can only be applied to a string or numeric value",
            jspOperationName(jsp->type));
          return jperError;
        }

        /*
         * If we have arguments, then they must be the precision and
         * optional scale used in .decimal().  Convert them to the
         * typmod equivalent and then truncate the numeric value per
         * this typmod details.
         */
        if (jsp->type == jpiDecimal && jsp->content.args.left)
        {
          Datum numdatum;
          Datum dtypmod;
          int32 precision;
          int32 scale = 0;
          bool have_error;
          ArrayType  *arrtypmod;
          Datum datums[2];
          char pstr[12];  /* sign, 10 digits and '\0' */
          char sstr[12];  /* sign, 10 digits and '\0' */

          jspGetLeftArg(jsp, &elem);
          if (elem.type != jpiNumeric)
            meos_error(ERROR, MEOS_ERR_INTERNAL_ERROR,
              "invalid jsonpath item type for .decimal() precision");

          precision = pg_numeric_int4_opt_error(jspGetNumeric(&elem),
            &have_error);
          if (have_error)
          {
            meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
              "precision of jsonpath item method .%s() is out of range for type integer",
              jspOperationName(jsp->type));
            return jperError;
          }
          if (jsp->content.args.right)
          {
            jspGetRightArg(jsp, &elem);
            if (elem.type != jpiNumeric)
            {
              meos_error(ERROR, MEOS_ERR_INTERNAL_ERROR,
                "invalid jsonpath item type for .decimal() scale");
              return jperError;
            }
            scale = pg_numeric_int4_opt_error(jspGetNumeric(&elem), &have_error);
            if (have_error)
            {
              meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
                "scale of jsonpath item method .%s() is out of range for type integer",
                jspOperationName(jsp->type));
              return jperError;
            }
          }

          /*
           * numerictypmodin() takes the precision and scale in the
           * form of CString arrays.
           */
          pg_ltoa(precision, pstr);
          datums[0] = CStringGetDatum(pstr);
          pg_ltoa(scale, sstr);
          datums[1] = CStringGetDatum(sstr);
          arrtypmod = construct_array_builtin(datums, 2, CSTRINGOID);
          dtypmod = DirectFunctionCall1(numerictypmodin,
            PointerGetDatum(arrtypmod));
          /* Convert numstr to Numeric with typmod */
          Assert(numstr != NULL);
          bool noerr = true; // TODO Check for error
          num = pg_numeric_in(numstr, -1);
          if (! noerr)
          {
            meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
              "argument \"%s\" of jsonpath item method .%s() is invalid for type %s",
              numstr, jspOperationName(jsp->type), "numeric");
            return jperError;
          }
          num = DatumGetNumeric(numdatum);
          pfree(arrtypmod);
        }
        jb = &jbv;
        jb->type = jbvNumeric;
        jb->val.numeric = num;
        res = executeNextItem(jsp, NULL, jb, found, true);
      }
      break;

    case jpiInteger:
      {
        JsonbValue jbv;
        Datum datum;
        if (unwrap && JsonbType(jb) == jbvArray)
          return executeItemUnwrapTargetArray(jsp, jb, found, false);

        if (jb->type == jbvNumeric)
        {
          bool have_error;
          int32 val;
          val = pg_numeric_int4_opt_error(jb->val.numeric, &have_error);
          if (have_error)
          {
            meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
              "argument \"%s\" of jsonpath item method .%s() is invalid for type %s",
              pg_numeric_out(jb->val.numeric),
              jspOperationName(jsp->type), "integer");
            return jperError;
          }
          datum = Int32GetDatum(val);
          res = jperOk;
        }
        else if (jb->type == jbvString)
        {
          /* cast string as integer */
          char *tmp = pnstrdup(jb->val.string.val, jb->val.string.len);
          int value = int32_in(tmp);
          if (value == INT_MAX)
          {
            meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
              "argument \"%s\" of jsonpath item method .%s() is invalid for type %s",
              tmp, jspOperationName(jsp->type), "integer");
            return jperError;
          }
          res = jperOk;
        }

        if (res == jperNotFound)
        {
          meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
            "jsonpath item method .%s() can only be applied to a string or numeric value",
            jspOperationName(jsp->type));
          return jperError;
        }
        jb = &jbv;
        jb->type = jbvNumeric;
        jb->val.numeric = int32_to_numeric(datum);
        res = executeNextItem(jsp, NULL, jb, found, true);
      }
      break;

    case jpiStringFunc:
      {
        JsonbValue jbv;
        char *tmp = NULL;
        if (unwrap && JsonbType(jb) == jbvArray)
          return executeItemUnwrapTargetArray(jsp, jb, found, false);
        switch (JsonbType(jb))
        {
          case jbvString:
            /*
             * Value is not necessarily null-terminated, so we do
             * pnstrdup() here.
             */
            tmp = pnstrdup(jb->val.string.val, jb->val.string.len);
            break;
          case jbvNumeric:
            tmp = pg_numeric_out(jb->val.numeric);
            break;
          case jbvBool:
            tmp = (jb->val.boolean) ? "true" : "false";
            break;
          case jbvDatetime:
            {
              char buf[MAXDATELEN + 1];
              JsonEncodeDateTime(buf, jb->val.datetime.value,
                jb->val.datetime.typid, &jb->val.datetime.tz);
              tmp = pstrdup(buf);
            }
            break;
          case jbvNull:
          case jbvArray:
          case jbvObject:
          case jbvBinary:
            meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
              "jsonpath item method .%s() can only be applied to a boolean, string, numeric, or datetime value",
              jspOperationName(jsp->type));
            return jperError;
        }

        jb = &jbv;
        Assert(tmp != NULL);  /* We must have set tmp above */
        jb->val.string.val = tmp;
        jb->val.string.len = strlen(jb->val.string.val);
        jb->type = jbvString;
        res = executeNextItem(jsp, NULL, jb, found, true);
      }
      break;

    default:
      meos_error(ERROR, MEOS_ERR_INTERNAL_ERROR,
        "unrecognized jsonpath item type: %d", jsp->type);
      return jperError;
  }
  return res;
}

/*
 * Definition of JsonPathGetVarCallback for when JsonPathExecContext.vars
 * is specified as a jsonb value.
 */
JsonbValue *
getJsonPathVariableFromJsonb(void *varsJsonb, char *varName, int varNameLength,
  JsonbValue *baseObject, int *baseObjectId)
{
  Jsonb *vars = varsJsonb;
  JsonbValue tmp;
  tmp.type = jbvString;
  tmp.val.string.val = varName;
  tmp.val.string.len = varNameLength;
  JsonbValue *result = findJsonbValueFromContainer(&vars->root, JB_FOBJECT,
    &tmp);
  if (result == NULL)
  {
    *baseObjectId = -1;
    return NULL;
  }
  *baseObjectId = 1;
  JsonbInitBinary(baseObject, vars);
  return result;
}

/*****************************************************************************/

/*
 * Interface to jsonpath executor
 *
 * 'path' - jsonpath to be executed
 * 'vars' - variables to be substituted to jsonpath
 * 'getVar' - callback used by getJsonPathVariable() to extract variables from
 *    'vars'
 * 'countVars' - callback to count the number of jsonpath variables in 'vars'
 * 'json' - target document for jsonpath evaluation
 * 'throwErrors' - whether we should throw suppressible errors
 * 'result' - list to store result items into
 *
 * Returns an error if a recoverable error happens during processing, or NULL
 * on no error.
 *
 * Note, jsonb and jsonpath values should be available and untoasted during
 * work because JsonPathItem, JsonbValue and result item could have pointers
 * into input values.  If caller needs to just check if document matches
 * jsonpath, then it doesn't provide a result arg.  In this case executor
 * works till first positive result and does not check the rest if possible.
 * In other case it tries to find all the satisfied result items.
 */
JsonPathExecResult
executeJsonPath(JsonPath *path, void *vars, JsonPathGetVarCallback getVar,
  JsonPathCountVarsCallback countVars, Jsonb *json, bool throwErrors,
  JsonValueList *result, bool useTz)
{
  JsonPathExecContext cxt;
  JsonPathExecResult res;
  JsonPathItem jsp;
  JsonbValue jbv;

  jspInit(&jsp, path);
  if (! JsonbExtractScalar(&json->root, &jbv))
    JsonbInitBinary(&jbv, json);
  cxt.vars = vars;
  cxt.getVar = getVar;
  cxt.laxMode = (path->header & JSONPATH_LAX) != 0;
  cxt.ignoreStructuralErrors = cxt.laxMode;
  cxt.root = &jbv;
  cxt.current = &jbv;
  cxt.baseObject.jbc = NULL;
  cxt.baseObject.id = 0;
  /* 1 + number of base objects in vars */
  cxt.lastGeneratedObjectId = 1 + countVars(vars);
  cxt.innermostArraySize = -1;
  cxt.throwErrors = throwErrors;
  cxt.useTz = useTz;

  if (jspStrictAbsenceOfErrors(&cxt) && !result)
  {
    /*
     * In strict mode we must get a complete list of values to check that
     * there are no errors at all.
     */
    JsonValueList vals = {0};
    res = executeItem(&cxt, &jsp, &jbv, &vals);
    if (jperIsError(res))
      return res;
    return JsonValueListIsEmpty(&vals) ? jperNotFound : jperOk;
  }
  res = executeItem(&cxt, &jsp, &jbv, result);
  Assert(!throwErrors || !jperIsError(res));
  return res;
}

/*
 * jsonb_path_match
 *    Returns jsonpath predicate result item for the specified jsonb value.
 *    See jsonb_path_exists() comment for details regarding error handling.
 */
int 
jsonb_path_match_int(const Jsonb *jb, const JsonPath *jp, const Jsonb *vars,
  bool silent, bool tz)
{
  JsonValueList found = {0};
  (void) executeJsonPath((JsonPath *) jp, (void *) vars,
    getJsonPathVariableFromJsonb, countVariablesFromJsonb, (Jsonb *) jb,
    ! silent, &found, tz);

  if (JsonValueListLength(&found) == 1)
  {
    JsonbValue *jbv = JsonValueListHead(&found);
    if (jbv->type == jbvBool)
      return (int) jbv->val.boolean;
    if (jbv->type == jbvNull)
      return -1;
  }
  if (! silent)
    meos_error(ERROR, MEOS_ERR_INVALID_ARG_VALUE,
      "single boolean result is expected");
  return -1;
}

/*****************************************************************************/