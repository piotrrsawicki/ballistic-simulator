#include <math.h>
#include <gsl/gsl_errno.h>
#include "missile_sim.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define OMEGA_E 7.2921159e-5
#define G_MU 3.986004418e14
#define R_E 6378137.0
#define WGS84_A 6378137.0
#define WGS84_F (1.0 / 298.257223563)

static double get_air_density(double altitude) {
    if (altitude > 100000.0) return 0.0;
    return 1.225 * exp(-altitude / 8500.0);
}

void geodetic_to_ecef(double lat, double lon, double alt, double *x, double *y, double *z) {
    double lat_rad = lat * M_PI / 180.0;
    double lon_rad = lon * M_PI / 180.0;
    double b = WGS84_A * (1.0 - WGS84_F);
    double e2 = 1.0 - (b * b) / (WGS84_A * WGS84_A);
    double n = WGS84_A / sqrt(1.0 - e2 * sin(lat_rad) * sin(lat_rad));
    
    *x = (n + alt) * cos(lat_rad) * cos(lon_rad);
    *y = (n + alt) * cos(lat_rad) * sin(lon_rad);
    *z = (n * (1.0 - e2) + alt) * sin(lat_rad);
}

void ecef_to_geodetic(double x, double y, double z, double *lat, double *lon, double *alt) {
    double b = WGS84_A * (1.0 - WGS84_F);
    double e2 = 1.0 - (b * b) / (WGS84_A * WGS84_A);
    double ep2 = (WGS84_A * WGS84_A - b * b) / (b * b);
    double p = sqrt(x * x + y * y);
    double th = atan2(WGS84_A * z, b * p);
    
    double lat_rad = atan2(z + ep2 * b * pow(sin(th), 3), p - e2 * WGS84_A * pow(cos(th), 3));
    double lon_rad = atan2(y, x);
    double n = WGS84_A / sqrt(1.0 - e2 * sin(lat_rad) * sin(lat_rad));
    
    *alt = p / cos(lat_rad) - n;
    *lat = lat_rad * 180.0 / M_PI;
    *lon = lon_rad * 180.0 / M_PI;
}

int missile_dynamics(double t, const double y[], double f[], void *params) {
    SimParams *p = (SimParams *)params;
    double X = y[0], Y = y[1], Z = y[2];
    double VX = y[3], VY = y[4], VZ = y[5];
    double M = y[6];

    double r = sqrt(X * X + Y * Y + Z * Z);
    double h = r - R_E; 

    double g_coeff = -G_MU / (r * r * r);
    
    double vrel_x = VX - (-OMEGA_E * Y);
    double vrel_y = VY - (OMEGA_E * X);
    double vrel_z = VZ;
    double vrel_mag = sqrt(vrel_x * vrel_x + vrel_y * vrel_y + vrel_z * vrel_z);

    double drag_coeff = 0.0;
    if (vrel_mag > 1e-3) {
        drag_coeff = -0.5 * get_air_density(h) * vrel_mag * p->cd * p->area;
    }

    double tx = 0, ty = 0, tz = 0, dm = 0;
    if (t < p->burn_time && M > p->dry_mass) {
        double theta = OMEGA_E * t;
        tx = p->thrust * (p->thrust_dir_ecef[0] * cos(theta) - p->thrust_dir_ecef[1] * sin(theta));
        ty = p->thrust * (p->thrust_dir_ecef[0] * sin(theta) + p->thrust_dir_ecef[1] * cos(theta));
        tz = p->thrust * p->thrust_dir_ecef[2];
        dm = p->mass_flow;
    }

    f[0] = VX; f[1] = VY; f[2] = VZ;
    f[3] = g_coeff * X + (drag_coeff * (vrel_x / (vrel_mag + 1e-9)) + tx) / M;
    f[4] = g_coeff * Y + (drag_coeff * (vrel_y / (vrel_mag + 1e-9)) + ty) / M;
    f[5] = g_coeff * Z + (drag_coeff * (vrel_z / (vrel_mag + 1e-9)) + tz) / M;
    f[6] = -dm;
    return GSL_SUCCESS;
}