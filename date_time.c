
#include <math.h>
#include <stdio.h>

#include "date_time.h"

int parse_timestamp(const char* str, int *year, int *doy, double *sec) {
    int y, m, d, h, min, s;
    if (sscanf(str, "%4d%2d%2d%2d%2d%2d", &y, &m, &d, &h, &min, &s) != 6) {
        return -1;
    }
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int is_leap = ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0));
    days_in_month[1] = is_leap ? 29 : 28;
    
    int yday = 0;
    for (int i = 0; i < m - 1; i++) {
        yday += days_in_month[i];
    }
    yday += d;
    
    *year = y;
    *doy = yday;
    *sec = h * 3600.0 + min * 60.0 + s;
    return 0;
}

typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} dt_components;

static void yds_to_datetime(int year, int day_of_year, double total_seconds, dt_components *dt) {
    double sec_of_day = fmod(total_seconds, 86400.0);
    if (sec_of_day < 0) {
        sec_of_day += 86400.0;
    }
    int extra_days = (int)floor(total_seconds / 86400.0);
    
    int sec = (int)round(sec_of_day);
    if (sec >= 86400) { // Handle rounding edge case
        sec -= 86400;
        extra_days++;
    }

    int day = day_of_year + extra_days;

    while (1) {
        int is_leap = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
        int days_in_year = is_leap ? 366 : 365;
        if (day > 0 && day <= days_in_year) break;
        
        if (day <= 0) {
            year--;
            is_leap = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
            days_in_year = is_leap ? 366 : 365;
            day += days_in_year;
        } else {
            day -= days_in_year;
            year++;
        }
    }

    dt->year = year;

    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int is_leap = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    days_in_month[1] = is_leap ? 29 : 28;

    int month = 0;
    while (month < 12 && day > days_in_month[month]) {
        day -= days_in_month[month];
        month++;
    }
    dt->month = month + 1;
    dt->day = day;

    dt->hour = sec / 3600;
    dt->minute = (sec % 3600) / 60;
    dt->second = sec % 60;
}

void format_iso8601(int year, int day_of_year, double total_seconds, char *buffer) {
    dt_components dt;
    yds_to_datetime(year, day_of_year, total_seconds, &dt);
    sprintf(buffer, "%04d-%02d-%02dT%02d:%02d:%02dZ", dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second);
}

void format_local_time(int year, int day_of_year, double utc_seconds_of_day, double tz_offset_hours, char *buffer) {
    double local_total_seconds = utc_seconds_of_day + tz_offset_hours * 3600.0;
    
    dt_components dt;
    yds_to_datetime(year, day_of_year, local_total_seconds, &dt);

    int tz_h = (int)fabs(tz_offset_hours);
    int tz_m = (int)(fmod(fabs(tz_offset_hours), 1.0) * 60.0);
    char tz_sign = (tz_offset_hours >= 0) ? '+' : '-';

    sprintf(buffer, "%04d-%02d-%02dT%02d:%02d:%02d (%c%02d:%02d)", dt.year, dt.month, dt.day, dt.hour, dt.minute, dt.second, tz_sign, tz_h, tz_m);
}

double julian_date(int year, int month, int day, int hour, int min, double sec) {
    if (month <= 2) {
        year -= 1;
        month += 12;
    }

    int A = year / 100;
    int B = 2 - A + A / 4;

    double JD = (int)(365.25 * (year + 4716))
              + (int)(30.6001 * (month + 1))
              + day + B - 1524.5;

    double day_fraction = (hour + min / 60.0 + sec / 3600.0) / 24.0;
    return JD + day_fraction;
}

double gmst_from_julian(double JD) {
    double T = (JD - 2451545.0) / 36525.0;

    double GMST_deg =
        280.46061837 +
        360.98564736629 * (JD - 2451545.0) +
        0.000387933 * T * T -
        (T * T * T) / 38710000.0;

    GMST_deg = fmod(GMST_deg, 360.0);
    if (GMST_deg < 0) GMST_deg += 360.0;

    return GMST_deg * M_PI / 180.0;
}

double get_julian_date_from_ymds(int year, int day_of_year, double seconds_in_day) {
    // Convert DOY → month/day
    int days_in_month[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    int is_leap = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    days_in_month[1] = is_leap ? 29 : 28;

    int month = 0;
    int day = day_of_year;

    while (month < 12 && day > days_in_month[month]) {
        day -= days_in_month[month];
        month++;
    }
    month += 1;

    // Extract HMS from seconds_in_day
    int hour = (int)(seconds_in_day / 3600.0);
    int minute = (int)(fmod(seconds_in_day, 3600.0) / 60.0);
    double second = fmod(seconds_in_day, 60.0);

    return julian_date(year, month, day, hour, minute, second);
}