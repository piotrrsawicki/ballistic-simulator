
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

void format_iso8601(int year, int day_of_year, double total_seconds, char *buffer) {
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int sec = (int)total_seconds;
    int extra_days = sec / 86400;
    sec = sec % 86400;
    
    int day = day_of_year + extra_days;
    
    while (1) {
        int is_leap = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
        int days_in_year = is_leap ? 366 : 365;
        if (day <= days_in_year) break;
        day -= days_in_year;
        year++;
    }

    int is_leap = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    days_in_month[1] = is_leap ? 29 : 28;

    int month = 0;
    while (month < 12 && day > days_in_month[month]) {
        day -= days_in_month[month];
        month++;
    }

    int hour = sec / 3600;
    int minute = (sec % 3600) / 60;
    int second = sec % 60;

    sprintf(buffer, "%04d-%02d-%02dT%02d:%02d:%02dZ", year, month + 1, day, hour, minute, second);
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