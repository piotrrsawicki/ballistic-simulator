#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <gsl/gsl_odeiv2.h>
#include <gsl/gsl_errno.h>
#include "missile_sim.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define OMEGA_E 7.2921159e-5
#define G0 9.80665

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

void print_help(const char *prog_name) {
    printf("Usage: %s <lat> <lon> <pitch_deg> <azimuth_deg> <mass_kg> <twr> <day_of_year> <utc_seconds> [area_m2] [cd] [isp] [fuel_fraction] [f107A] [f107] [ap] [csv_file]\n\n", prog_name);
    printf("Parameters:\n");
    printf("- <lat>: Initial launch latitude in degrees.\n");
    printf("- <lon>: Initial launch longitude in degrees.\n");
    printf("- <pitch_deg>: Launch pitch angle in degrees (0 = horizontal, 90 = straight up).\n");
    printf("- <azimuth_deg>: Launch heading/azimuth in degrees (0 = North, 90 = East, 180 = South, 270 = West).\n");
    printf("- <mass_kg>: Initial mass of the rocket in kilograms.\n");
    printf("- <twr>: Initial Thrust-to-Weight Ratio.\n");
    printf("- <day_of_year>: Day of the year for the simulation (e.g., 1 to 365).\n");
    printf("- <utc_seconds>: Seconds into the UTC day at launch (e.g., 43200 for noon UTC).\n");
    printf("- [area_m2]: (Optional) Reference area for drag calculations in square meters (default: 1.5).\n");
    printf("- [cd]: (Optional) Drag coefficient (default: 0.3).\n");
    printf("- [isp]: (Optional) Specific Impulse of the rocket engine in seconds (default: 300.0).\n");
    printf("- [fuel_fraction]: (Optional) Mass fraction of the propellant (default: 0.9).\n");
    printf("- [f107A]: (Optional) 81-day average F10.7 solar flux (default: 150.0).\n");
    printf("- [f107]: (Optional) Daily F10.7 solar flux for the previous day (default: 150.0).\n");
    printf("- [ap]: (Optional) Daily magnetic index (default: 4.0).\n");
    printf("- [csv_file]: (Optional) Path to a CSV file to log the trajectory (1Hz).\n");
}

int main(int argc, char **argv) {
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        print_help(argv[0]);
        return 0;
    }

    if (argc < 9 || argc > 17) {
        printf("Usage: %s <lat> <lon> <pitch_deg> <azimuth_deg> <mass_kg> <twr> <day_of_year> <utc_seconds> [area_m2] [cd] [isp] [fuel_fraction] [f107A] [f107] [ap] [csv_file]\n", argv[0]);
        printf("Try '%s --help' for more information.\n", argv[0]);
        return 1;
    }

    double lat0 = atof(argv[1]);
    double lon0 = atof(argv[2]);
    double pitch = atof(argv[3]);
    double azimuth = atof(argv[4]);
    double mass0 = atof(argv[5]);
    double twr = atof(argv[6]);
    int day_of_year = atoi(argv[7]);
    double seconds_in_day = atof(argv[8]);

    double isp = (argc >= 12) ? atof(argv[11]) : 300.0;
    
    SimParams params;
    params.thrust = twr * mass0 * G0;
    params.mass_flow = params.thrust / (isp * G0);
    double fuel_fraction = (argc >= 13) ? atof(argv[12]) : 0.9;
    params.dry_mass = mass0 * (1.0 - fuel_fraction);
    params.burn_time = (mass0 - params.dry_mass) / params.mass_flow;
    params.area = (argc >= 10) ? atof(argv[9]) : 1.5;
    params.cd = (argc >= 11) ? atof(argv[10]) : 0.3;

    // MSIS atmosphere model parameters
    params.day_of_year = day_of_year;
    params.seconds_in_day = seconds_in_day;
    params.f107A = (argc >= 14) ? atof(argv[13]) : 150.0;
    params.f107 = (argc >= 15) ? atof(argv[14]) : 150.0;
    params.ap = (argc >= 16) ? atof(argv[15]) : 4.0;
    const char *csv_filename = (argc >= 17) ? argv[16] : "simulation.csv";

    // Calculate thrust directional vector relative to ECEF
    double pitch_rad = pitch * M_PI / 180.0;
    double az_rad = azimuth * M_PI / 180.0;
    double E = cos(pitch_rad) * sin(az_rad);
    double N = cos(pitch_rad) * cos(az_rad);
    double U = sin(pitch_rad);

    double lat_rad = lat0 * M_PI / 180.0;
    double lon_rad = lon0 * M_PI / 180.0;
    double slon = sin(lon_rad), clon = cos(lon_rad);
    double slat = sin(lat_rad), clat = cos(lat_rad);

    params.thrust_dir_ecef[0] = -slon * E - slat * clon * N + clat * clon * U;
    params.thrust_dir_ecef[1] =  clon * E - slat * slon * N + clat * slon * U;
    params.thrust_dir_ecef[2] =  clat * N + slat * U;

    double x0, y0, z0;
    geodetic_to_ecef(lat0, lon0, 0.0, &x0, &y0, &z0);

    // State: X, Y, Z, Vx, Vy, Vz, Mass (Initial ECI velocity due to Earth rotation)
    double y[7] = {x0, y0, z0, -OMEGA_E * y0, OMEGA_E * x0, 0.0, mass0};

    gsl_odeiv2_system sys = {missile_dynamics, NULL, 7, &params};
    gsl_odeiv2_driver *d = gsl_odeiv2_driver_alloc_y_new(&sys, gsl_odeiv2_step_rk8pd, 1e-3, 1e-8, 1e-8);

    double t = 0.0, t_step = 1.0;
    double prev_lat = lat0, prev_lon = lon0, prev_alt = 0.0, prev_t = 0.0;
    
    time_t t_now = time(NULL);
    struct tm *tm_now = gmtime(&t_now);
    int current_year = tm_now->tm_year + 1900;

    FILE *csv = NULL;
    char ts_buf[32];
    if (csv_filename) {
        csv = fopen(csv_filename, "w");
        if (csv) {
            format_iso8601(current_year, day_of_year, seconds_in_day, ts_buf);
            fprintf(csv, "Timestamp,Longitude,Latitude,Altitude\n");
            fprintf(csv, "%s,%.15f,%.15f,%.15f\n", ts_buf, lon0, lat0, 0.0);
        } else {
            fprintf(stderr, "Warning: Could not open CSV file '%s' for writing.\n", csv_filename);
        }
    }

    printf("Simulating flight...\n");
    while (t < 2000000.0) { // Safety timeout
        int status = gsl_odeiv2_driver_apply(d, &t, t + t_step, y);
        if (status != GSL_SUCCESS) break;

        // Re-convert ECI to ECEF at time t to check real altitude
        double ecef_x = y[0] * cos(OMEGA_E * t) + y[1] * sin(OMEGA_E * t);
        double ecef_y = -y[0] * sin(OMEGA_E * t) + y[1] * cos(OMEGA_E * t);
        double ecef_z = y[2];

        double curr_lat, curr_lon, curr_alt;
        ecef_to_geodetic(ecef_x, ecef_y, ecef_z, &curr_lat, &curr_lon, &curr_alt);

        if (csv) {
            format_iso8601(current_year, day_of_year, seconds_in_day + t, ts_buf);
            fprintf(csv, "%s,%.15f,%.15f,%.15f\n", ts_buf, curr_lon, curr_lat, curr_alt);
        }

        if (curr_alt <= 0.0 && t > 10.0) {
            // Linearly interpolate the exact moment of impact
            double frac = prev_alt / (prev_alt - curr_alt);
            double impact_lat = prev_lat + frac * (curr_lat - prev_lat);
            double impact_lon = prev_lon + frac * (curr_lon - prev_lon);
            
            if (csv) {
                format_iso8601(current_year, day_of_year, seconds_in_day + prev_t + frac * (t - prev_t), ts_buf);
                fprintf(csv, "%s,%.15f,%.15f,%.15f\n", ts_buf, impact_lon, impact_lat, 0.0);
            }

            printf("\n--- IMPACT DETECTED ---\n");
            printf("Time of flight : %.2f seconds\n", prev_t + frac * (t - prev_t));
            printf("Impact Latitude  : %.15f\n", impact_lat);
            printf("Impact Longitude : %.15f\n", impact_lon);
            break;
        }
        prev_lat = curr_lat; prev_lon = curr_lon; prev_alt = curr_alt; prev_t = t;
    }

    if (csv) {
        fclose(csv);
    }
    gsl_odeiv2_driver_free(d);
    return 0;
}