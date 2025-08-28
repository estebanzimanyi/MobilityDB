/*-------------------------------------------------------------------------
 *
 * pgfnames.c
 *	  directory handling functions
 *
 * Portions Copyright (c) 1996-2025, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/common/pgfnames.c
 *
 *-------------------------------------------------------------------------
 */

// MEOS
// #ifndef FRONTEND
// #include "postgres.h"
// #else
// #include "postgres_fe.h"
// #endif

#include <dirent.h>
// MEOS
#include "postgres.h"
#include "../../include/meos.h"

// MEOS
// #ifndef FRONTEND
// #define pg_log_warning(...) elog(WARNING, __VA_ARGS__)
// #else
// #include "common/logging.h"
// #endif

/*
 * pgfnames
 *
 * return a list of the names of objects in the argument directory.  Caller
 * must call pgfnames_cleanup later to free the memory allocated by this
 * function.
 */
char	  **
pgfnames(const char *path)
{
	DIR		   *dir;
	struct dirent *file;
	char	  **filenames;
	int			numnames = 0;
	int			fnsize = 200;	/* enough for many small dbs */

	dir = opendir(path);
	if (dir == NULL)
	{
		// MEOS
		// pg_log_warning("could not open directory \"%s\": %m", path);
		meos_error(WARNING, MEOS_ERR_DIRECTORY_ERROR,
		"could not open directory \"%s\": %m", path);
		return NULL;
	}

	filenames = (char **) palloc(fnsize * sizeof(char *));

	while (errno = 0, (file = readdir(dir)) != NULL)
	{
		if (strcmp(file->d_name, ".") != 0 && strcmp(file->d_name, "..") != 0)
		{
			if (numnames + 1 >= fnsize)
			{
				fnsize *= 2;
				filenames = (char **) repalloc(filenames,
											   fnsize * sizeof(char *));
			}
			filenames[numnames++] = pstrdup(file->d_name);
		}
	}

	if (errno)
	// MEOS
	// pg_log_warning("could not read directory \"%s\": %m", path);
	{
		meos_error(WARNING, MEOS_ERR_DIRECTORY_ERROR,
			"could not read directory \"%s\": %m", path);
		return NULL;
	}

	filenames[numnames] = NULL;

	if (closedir(dir))
	// MEOS
		// pg_log_warning("could not close directory \"%s\": %m", path);
	{
		meos_error(WARNING, MEOS_ERR_DIRECTORY_ERROR,
			"could not close directory \"%s\": %m", path);
		return NULL;
	}

	return filenames;
}


/*
 *	pgfnames_cleanup
 *
 *	deallocate memory used for filenames
 */
void
pgfnames_cleanup(char **filenames)
{
	char	  **fn;

	for (fn = filenames; *fn; fn++)
		pfree(*fn);

	pfree(filenames);
}
