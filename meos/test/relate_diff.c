/* SPDX-License-Identifier: PostgreSQL */
/**
 * @file
 * @brief Check the native DE-9IM matrix of the WKT geometry pairs read on the
 * standard input, one `wkt1|wkt2` or `wkt1|wkt2|expected` record per line
 * @details A record without an expected matrix prints the nine-character
 * matrix, or the word `UNSUPPORTED` when the pair falls outside the coverage
 * of #meos_relate, which compares against the answers another implementation
 * gives for the same corpus. A record with an expected matrix is checked
 * against it and counted, the exit status reporting whether every record
 * passed. An uncovered pair counts as a failure, so a coverage gap shows up
 * as a red run rather than as a silently skipped record. The expected matrix
 * follows the DE-9IM pattern alphabet, so `T` accepts any non-empty
 * intersection and `*` accepts anything
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <meos.h>
#include <meos_geo.h>
#include <meos_internal_geo.h>
#include <liblwgeom.h>

/**
 * @brief Return true if a matrix satisfies an expected DE-9IM pattern
 */
static bool
matrix_matches(const char *matrix, const char *pattern)
{
  for (int i = 0; i < 9; i++)
  {
    if (pattern[i] == '*')
      continue;
    if (pattern[i] == 'T')
    {
      if (matrix[i] == 'F')
        return false;
      continue;
    }
    if (pattern[i] != matrix[i])
      return false;
  }
  return true;
}

int
main(void)
{
  meos_initialize();
  /* The corpus holds geometries of any size, and a truncated line would be
   * reported as a parse failure of the engine rather than of the reader */
  char *line = NULL;
  size_t linesize = 0;
  int checked = 0, passed = 0, uncovered = 0;
  while (getline(&line, &linesize, stdin) > 0)
  {
    char *nl = strchr(line, '\n');
    if (nl)
      *nl = '\0';
    /* A corpus file carries its provenance in leading comment lines */
    if (line[0] == '#')
      continue;
    char *sep = strchr(line, '|');
    if (! sep)
      continue;
    *sep = '\0';
    char *second = sep + 1;
    char *expected = strchr(second, '|');
    if (expected)
      *expected++ = '\0';

    LWGEOM *g1 = lwgeom_from_wkt(line, LW_PARSER_CHECK_NONE);
    LWGEOM *g2 = lwgeom_from_wkt(second, LW_PARSER_CHECK_NONE);
    if (! g1 || ! g2)
    {
      printf("PARSE-ERROR\n");
      continue;
    }
    char matrix[10];
    bool covered = meos_relate(g1, g2, matrix);
    if (! expected)
      printf("%s\n", covered ? matrix : "UNSUPPORTED");
    else if (! covered)
    {
      uncovered++;
      printf("UNCOVERED expected=%s  %s|%s\n", expected, line, second);
    }
    else
    {
      checked++;
      if (matrix_matches(matrix, expected))
        passed++;
      else
        printf("FAIL got=%s expected=%s  %s|%s\n", matrix, expected, line,
          second);
    }
    lwgeom_free(g1);
    lwgeom_free(g2);
  }
  free(line);
  meos_finalize();
  if (checked || uncovered)
  {
    printf("%d of %d checked records pass, %d uncovered\n", passed, checked,
      uncovered);
    return (passed == checked && uncovered == 0) ? 0 : 1;
  }
  return 0;
}
