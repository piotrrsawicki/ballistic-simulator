#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
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
    printf("Usage: %s [options]\n\n", prog_name);
    printf("Options:\n");
    printf("  --lat <val>            Initial launch latitude in degrees (default: 28.458566)\n");
    printf("  --lon <val>            Initial launch longitude in degrees (default: -80.528418)\n");
    printf("  --pitch <val>          Launch pitch angle in degrees (0=horizontal, 90=vertical) (default: 45.0)\n");
    printf("  --azimuth <val>        Launch heading/azimuth in degrees (0=N, 90=E, 180=S, 270=W) (default: 45.0)\n");
    printf("  --mass <val>           Initial mass of the rocket in kg (default: 1000.0)\n");
    printf("  --twr <val>            Initial Thrust-to-Weight Ratio (default: 2.0)\n");
    printf("  --day <val>            Day of the year for the simulation [1-365] (default: 120)\n");
    printf("  --sec <val>            Seconds into the UTC day at launch (default: 43200.0)\n");
    printf("  --area <val>           Reference area for drag calculations in m^2 (default: 1.5)\n");
    printf("  --cd <val>             Drag coefficient (default: 0.3)\n");
    printf("  --isp <val>            Specific Impulse of the engine in seconds (default: 300.0)\n");
    printf("  --fuel_fraction <val>  Mass fraction of the propellant (default: 0.9)\n");
    printf("  --f107A <val>          81-day average F10.7 solar flux (default: 150.0)\n");
    printf("  --f107 <val>           Daily F10.7 solar flux for previous day (default: 150.0)\n");
    printf("  --ap <val>             Daily magnetic index (default: 4.0)\n");
    printf("  --csv <file>           Path to a CSV file to log the trajectory (1Hz)\n");
    printf("  -h, --help             Show this help message\n");
}

int main(int argc, char **argv) {
    double lat0 = 28.458566;
    double lon0 = -80.528418;
    double pitch = 45.0;
    double azimuth = 45.0;
    double mass0 = 1000.0;
    double twr = 2.0;
    int day_of_year = 120;
    double seconds_in_day = 43200.0;
    double area = 1.5;
    double cd = 0.3;
    double isp = 300.0;
    double fuel_fraction = 0.9;
    double f107A = 150.0;
    double f107 = 150.0;
    double ap = 4.0;
    const char *csv_filename = NULL;

    static struct option long_options[] = {
        {"lat", required_argument, 0, 'l'},
        {"lon", required_argument, 0, 'o'},
        {"pitch", required_argument, 0, 'p'},
        {"azimuth", required_argument, 0, 'z'},
        {"mass", required_argument, 0, 'm'},
        {"twr", required_argument, 0, 't'},
        {"day", required_argument, 0, 'd'},
        {"sec", required_argument, 0, 's'},
        {"area", required_argument, 0, 'A'},
        {"cd", required_argument, 0, 'C'},
        {"isp", required_argument, 0, 'I'},
        {"fuel_fraction", required_argument, 0, 'F'},
        {"f107A", required_argument, 0, 'x'},
        {"f107", required_argument, 0, 'y'},
        {"ap", required_argument, 0, 'a'},
        {"csv", required_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'l': lat0 = atof(optarg); break;
            case 'o': lon0 = atof(optarg); break;
            case 'p': pitch = atof(optarg); break;
            case 'z': azimuth = atof(optarg); break;
            case 'm': mass0 = atof(optarg); break;
            case 't': twr = atof(optarg); break;
            case 'd': day_of_year = atoi(optarg); break;
            case 's': seconds_in_day = atof(optarg); break;
            case 'A': area = atof(optarg); break;
            case 'C': cd = atof(optarg); break;
            case 'I': isp = atof(optarg); break;
            case 'F': fuel_fraction = atof(optarg); break;
            case 'x': f107A = atof(optarg); break;
            case 'y': f107 = atof(optarg); break;
            case 'a': ap = atof(optarg); break;
            case 'c': csv_filename = optarg; break;
            case 'h': print_help(argv[0]); return 0;
            default: print_help(argv[0]); return 1;
        }
    }
    
    SimParams params;
    params.thrust = twr * mass0 * G0;
    params.mass_flow = params.thrust / (isp * G0);
    params.dry_mass = mass0 * (1.0 - fuel_fraction);
    params.burn_time = (mass0 - params.dry_mass) / params.mass_flow;
    params.area = area;
    params.cd = cd;

    // MSIS atmosphere model parameters
    params.day_of_year = day_of_year;
    params.seconds_in_day = seconds_in_day;
    params.f107A = f107A;
    params.f107 = f107;
    params.ap = ap;

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