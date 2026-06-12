#ifndef POSTGRES_H
#define POSTGRES_H

#include <stdint.h>

#define DatumGetPointer(X) ((Pointer) (X))

typedef char *Pointer;
typedef uintptr_t Datum;

typedef int8_t  int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32 DateADT;
typedef int64 TimeADT;
typedef int64 Timestamp;
typedef int64 TimestampTz;
typedef int64 TimeOffset;
typedef int32 fsec_t;      /* fractional seconds (in microseconds) */

typedef struct
{
  TimeOffset time;  /* all time units other than days, months and years */
  int32 day;        /* days, after time for alignment */
  int32 month;      /* months and years, after time for alignment */
} Interval;

typedef struct varlena
{
  char vl_len_[4];  /* Do not touch this field directly! */
  char vl_dat[];    /* Data content is here */
} varlena;

typedef varlena text;
typedef struct varlena bytea;

/* The following functions have the same name as external PostgreSQL functions */

extern text *cstring_to_text(const char *str);
extern char *text_to_cstring(const text *txt);
extern bool bool_in(const char *str);
extern char *bool_out(bool b);
extern double float8_in(const char *str);
extern char *float8_out(double num, int maxdd);
extern text *text_in(const char *str);
extern char *text_out(const text *txt);
extern text *text_upper(const text *txt);
extern text *text_lower(const text *txt);
extern text *text_initcap(const text *txt);
extern DateADT add_date_int(DateADT date, int32 days);
extern Timestamp add_timestamptz_interval(TimestampTz tstz, const Interval *interv);
extern DateADT date_in(const char *str);
extern char *date_out(DateADT date);
extern int interval_cmp(const Interval *interv1, const Interval *interv2);
extern Interval *interval_in(const char *str, int32 typmod);
extern Interval *pg_interval_in(const char *str, int32 typmod);
extern Interval *interval_make(int32 years, int32 months, int32 weeks, int32 days, int32 hours, int32 mins, double secs);
extern char *interval_out(const Interval *interv);
extern char *pg_interval_out(const Interval *interv);
extern TimeADT time_in(const char *str, int32 typmod);
extern char *time_out(TimeADT time);
extern Timestamp timestamp_in(const char *str, int32 typmod);
extern char *timestamp_out(Timestamp ts);
extern TimestampTz timestamptz_in(const char *str, int32 typmod);
extern TimestampTz pg_timestamptz_in(const char *str, int32 typmod);
extern char *timestamptz_out(TimestampTz tstz);

#endif /* POSTGRES_H */
