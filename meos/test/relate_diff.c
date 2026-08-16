/* SPDX-License-Identifier: PostgreSQL */
/**
 * @file
 * @brief Print the native DE-9IM matrix for the WKT geometry pairs read on
 * the standard input, one `wkt1|wkt2` pair per line
 * @details Each line yields either the nine-character matrix or the word
 * `UNSUPPORTED` when the pair falls outside the coverage of #meos_relate.
 * Comparing the output with the `ST_Relate` answers for the same corpus gives
 * the divergence between the native engine and the reference implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <meos.h>
#include <meos_geo.h>
#include <meos_internal_geo.h>
#include <liblwgeom.h>

int
main(void)
{
  meos_initialize();
  /* The corpus holds geometries of any size, and a truncated line would be
   * reported as a parse failure of the engine rather than of the reader */
  char *line = NULL;
  size_t linesize = 0;
  while (getline(&line, &linesize, stdin) > 0)
  {
    char *nl = strchr(line, '\n');
    if (nl)
      *nl = '\0';
    char *sep = strchr(line, '|');
    if (! sep)
      continue;
    *sep = '\0';
    LWGEOM *g1 = lwgeom_from_wkt(line, LW_PARSER_CHECK_NONE);
    LWGEOM *g2 = lwgeom_from_wkt(sep + 1, LW_PARSER_CHECK_NONE);
    if (! g1 || ! g2)
    {
      printf("PARSE-ERROR\n");
      continue;
    }
    char matrix[10];
    if (meos_relate(g1, g2, matrix))
      printf("%s\n", matrix);
    else
      printf("UNSUPPORTED\n");
    lwgeom_free(g1);
    lwgeom_free(g2);
  }
  free(line);
  meos_finalize();
  return 0;
}
