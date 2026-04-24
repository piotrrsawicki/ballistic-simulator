#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <gsl/gsl_odeiv2.h>
#include <gsl/gsl_errno.h>
#include "missile_sim.h"
#include "egm2008.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define OMEGA_E 7.2921159e-5
#define G0 9.80665

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

void print_help(const char *prog_name) {
    printf("Usage: %s [options]\n\n", prog_name);
    printf("Options:\n");
    printf("  --lat <val>            Initial launch latitude in degrees (default: 28.458566)\n");
    printf("  --lon <val>            Initial launch longitude in degrees (default: -80.528418)\n");
    printf("  --pitch <val>          Launch pitch angle in degrees (0=horizontal, 90=vertical) (default: 45.0)\n");
    printf("  --azimuth <val>        Launch heading/azimuth in degrees (0=N, 90=E, 180=S, 270=W) (default: 45.0)\n");
    printf("  --mass <val>           Initial mass of the rocket in kg (default: 1000.0)\n");
    printf("  --twr <val>            Initial Thrust-to-Weight Ratio (default: 2.0)\n");
    printf("  --time <val>           Launch time in UTC (YYYYMMDDHHmmss) (default: current time)\n");
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
    time_t t_now = time(NULL);
    struct tm *tm_now = gmtime(&t_now);
    int launch_year = tm_now->tm_year + 1900;
    int day_of_year = tm_now->tm_yday + 1;
    double seconds_in_day = tm_now->tm_hour * 3600.0 + tm_now->tm_min * 60.0 + tm_now->tm_sec;
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
        {"time", required_argument, 0, 'T'},
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
            case 'T':
                if (parse_timestamp(optarg, &launch_year, &day_of_year, &seconds_in_day) != 0) {
                    fprintf(stderr, "Error: Invalid time format. Use YYYYMMDDHHmmss.\n");
                    return 1;
                }
                break;
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

    float initial_geoid = 0.0f;
    egm2008_geoid_height((float)lat0, (float)lon0, &initial_geoid);
    double initial_alt_wgs84 = (double)initial_geoid; // Launch from MSL = 0.0

    double x0, y0, z0;
    geodetic_to_ecef(lat0, lon0, initial_alt_wgs84, &x0, &y0, &z0);

    // State: X, Y, Z, Vx, Vy, Vz, Mass (Initial ECI velocity due to Earth rotation)
    double y[7] = {x0, y0, z0, -OMEGA_E * y0, OMEGA_E * x0, 0.0, mass0};

    gsl_odeiv2_system sys = {missile_dynamics, NULL, 7, &params};
    gsl_odeiv2_driver *d = gsl_odeiv2_driver_alloc_y_new(&sys, gsl_odeiv2_step_rk8pd, 1e-3, 1e-8, 1e-8);

    double t = 0.0, t_step = 1.0;
    double prev_lat = lat0, prev_lon = lon0, prev_msl_alt = 0.0, prev_t = 0.0;
    double prev_vx = y[3], prev_vy = y[4], prev_vz = y[5];
    double prev_x = y[0], prev_y = y[1], prev_mass = y[6];
    double max_alt = 0.0;
    
    FILE *csv = NULL;
    char ts_buf[32];
    if (csv_filename) {
        csv = fopen(csv_filename, "w");
        if (csv) {
            format_iso8601(launch_year, day_of_year, seconds_in_day, ts_buf);
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
        
        float curr_geoid = 0.0f;
        egm2008_geoid_height((float)curr_lat, (float)curr_lon, &curr_geoid);
        double curr_msl_alt = curr_alt - (double)curr_geoid;

        if (curr_msl_alt > max_alt) max_alt = curr_msl_alt;

        if (csv) {
            format_iso8601(launch_year, day_of_year, seconds_in_day + t, ts_buf);
            fprintf(csv, "%s,%.15f,%.15f,%.15f\n", ts_buf, curr_lon, curr_lat, curr_msl_alt);
        }

        if (curr_msl_alt <= 0.0 && t > 10.0) {
            // Linearly interpolate the exact moment of impact
            double frac = prev_msl_alt / (prev_msl_alt - curr_msl_alt);
            double impact_lat = prev_lat + frac * (curr_lat - prev_lat);
            double impact_lon = prev_lon + frac * (curr_lon - prev_lon);
            
            if (csv) {
                format_iso8601(launch_year, day_of_year, seconds_in_day + prev_t + frac * (t - prev_t), ts_buf);
                fprintf(csv, "%s,%.15f,%.15f,%.15f\n", ts_buf, impact_lon, impact_lat, 0.0);
            }

            double tof = prev_t + frac * (t - prev_t);
            int hours = (int)(tof / 3600.0);
            int minutes = (int)(fmod(tof, 3600.0) / 60.0);
            double seconds = fmod(tof, 60.0);
            
            // Calculate orthodromic (great-circle) distance in km using Haversine formula
            double lat1 = lat0 * M_PI / 180.0;
            double lon1 = lon0 * M_PI / 180.0;
            double lat2 = impact_lat * M_PI / 180.0;
            double lon2 = impact_lon * M_PI / 180.0;
            double dlat = lat2 - lat1;
            double dlon = lon2 - lon1;
            double a_hav = sin(dlat/2) * sin(dlat/2) + cos(lat1) * cos(lat2) * sin(dlon/2) * sin(dlon/2);
            double c_hav = 2 * atan2(sqrt(a_hav), sqrt(1 - a_hav));
            double distance_km = 6378.137 * c_hav; // Multiplied by equatorial Earth radius

            // Interpolate the exact velocity vectors at impact
            double impact_vx = prev_vx + frac * (y[3] - prev_vx);
            double impact_vy = prev_vy + frac * (y[4] - prev_vy);
            double impact_vz = prev_vz + frac * (y[5] - prev_vz);
            double impact_x = prev_x + frac * (y[0] - prev_x);
            double impact_y = prev_y + frac * (y[1] - prev_y);

            // Calculate impact speed relative to the rotating Earth
            double vrel_x = impact_vx - (-OMEGA_E * impact_y);
            double vrel_y = impact_vy - (OMEGA_E * impact_x);
            double vrel_z = impact_vz;

            double impact_speed_ms = sqrt(vrel_x * vrel_x + vrel_y * vrel_y + vrel_z * vrel_z);
            double impact_speed_kmh = impact_speed_ms * 3.6;

            // Calculate kinetic energy
            double impact_mass = prev_mass + frac * (y[6] - prev_mass);
            double impact_energy_j = 0.5 * impact_mass * impact_speed_ms * impact_speed_ms;
            double impact_energy_tnt_kg = impact_energy_j / 4.184e6;

            printf("\n--- IMPACT DETECTED ---\n");
            printf("Time of flight : %.2f seconds (%02d%02d%05.2f)\n", tof, hours, minutes, seconds);
            printf("Impact coordinates (lat, long)  : %.15f, %.15f\n", impact_lat, impact_lon);
            printf("Impact Distance  : %.3f km\n", distance_km);
            printf("Maximum Altitude : %.3f km\n", max_alt / 1000.0);
            printf("Impact Speed     : %.2f m/s (%.2f km/h)\n", impact_speed_ms, impact_speed_kmh);
            printf("Impact Energy    : %.2e J (%.2f kg TNT eq.)\n", impact_energy_j, impact_energy_tnt_kg);
            break;
        }
        prev_lat = curr_lat; prev_lon = curr_lon; prev_msl_alt = curr_msl_alt; prev_t = t;
        prev_vx = y[3]; prev_vy = y[4]; prev_vz = y[5];
        prev_x = y[0]; prev_y = y[1]; prev_mass = y[6];
    }

    if (csv) {
        fclose(csv);
    }
    gsl_odeiv2_driver_free(d);
    return 0;
}