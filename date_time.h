#ifndef DATE_TIME_H
#define DATE_TIME_H

int parse_timestamp(const char* str, int *year, int *doy, double *sec);
void format_iso8601(int year, int day_of_year, double total_seconds, char *buffer);
void format_local_time(int year, int day_of_year, double utc_seconds_of_day, double tz_offset_hours, char *buffer);
double julian_date(int year, int month, int day, int hour, int min, double sec);
double gmst_from_julian(double JD);
double get_julian_date_from_ymds(int year, int day_of_year, double seconds_in_day);

#endif
