#include "../pgtypes/postgres.h"
#include "../pgtypes/utils/timestamp_def.h"
#include "../pgtypes/utils/timestamp.h"
#include "../pgtypes/utils/date.h"
/* PostgreSQL-compatible (unprefixed) base-type I/O declarations. These
 * mirror the surface that the exported meos.h exposes via
 * postgres_ext_defs.in.h, so in-tree consumers (and the MEOS C test
 * suite) that call the unprefixed names see their prototypes. */
#include "../pgtypes/pg_bool.h"
#include "../pgtypes/pg_text.h"
#include "../pgtypes/pg_float.h"
#include "../pgtypes/pg_date.h"
#include "../pgtypes/pg_interval.h"
#include "../pgtypes/pg_timestamp.h"
