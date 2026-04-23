#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <gsl/gsl_odeiv2.h>
#include <gsl/gsl_errno.h>
#include "missile_sim.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define OMEGA_E 7.2921159e-5
#define G0 9.80665

int main(int argc, char **argv) {
    if (argc != 7) {
        printf("Usage: %s <lat> <lon> <pitch_deg> <azimuth_deg> <mass_kg> <twr>\n", argv[0]);
        return 1;
    }

    double lat0 = atof(argv[1]);
    double lon0 = atof(argv[2]);
    double pitch = atof(argv[3]);
    double azimuth = atof(argv[4]);
    double mass0 = atof(argv[5]);
    double twr = atof(argv[6]);

    double isp = 300.0; // Typical solid rocket Specific Impulse
    
    SimParams params;
    params.thrust = twr * mass0 * G0;
    params.mass_flow = params.thrust / (isp * G0);
    params.dry_mass = mass0 * 0.1; // Assume 90% fuel fraction
    params.burn_time = (mass0 - params.dry_mass) / params.mass_flow;
    params.area = 1.5; 
    params.cd = 0.3;

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
    
    printf("Simulating flight...\n");
    while (t < 20000.0) { // Safety timeout
        int status = gsl_odeiv2_driver_apply(d, &t, t + t_step, y);
        if (status != GSL_SUCCESS) break;

        // Re-convert ECI to ECEF at time t to check real altitude
        double ecef_x = y[0] * cos(OMEGA_E * t) + y[1] * sin(OMEGA_E * t);
        double ecef_y = -y[0] * sin(OMEGA_E * t) + y[1] * cos(OMEGA_E * t);
        double ecef_z = y[2];

        double curr_lat, curr_lon, curr_alt;
        ecef_to_geodetic(ecef_x, ecef_y, ecef_z, &curr_lat, &curr_lon, &curr_alt);

        if (curr_alt <= 0.0 && t > 10.0) {
            // Linearly interpolate the exact moment of impact
            double frac = prev_alt / (prev_alt - curr_alt);
            double impact_lat = prev_lat + frac * (curr_lat - prev_lat);
            double impact_lon = prev_lon + frac * (curr_lon - prev_lon);
            
            printf("\n--- IMPACT DETECTED ---\n");
            printf("Time of flight : %.2f seconds\n", prev_t + frac * (t - prev_t));
            printf("Impact Latitude  : %.6f\n", impact_lat);
            printf("Impact Longitude : %.6f\n", impact_lon);
            break;
        }
        prev_lat = curr_lat; prev_lon = curr_lon; prev_alt = curr_alt; prev_t = t;
    }

    gsl_odeiv2_driver_free(d);
    return 0;
}