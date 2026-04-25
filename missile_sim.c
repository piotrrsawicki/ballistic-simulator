#include <math.h>
#include <gsl/gsl_errno.h>
#include "missile_sim.h"
#include "MSIS/nrlmsise-00.h"
#include "egm2008.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define OMEGA_E 7.2921159e-5
#define G_MU 3.986004418e14
#define R_E 6378137.0
#define WGS84_A 6378137.0
#define WGS84_F (1.0 / 298.257223563)
#define WGS84_J2 1.08262982e-3

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

    // Convert ECI to ECEF to get current position over rotating Earth
    double ecef_x = X * cos(OMEGA_E * t) + Y * sin(OMEGA_E * t);
    double ecef_y = -X * sin(OMEGA_E * t) + Y * cos(OMEGA_E * t);
    double ecef_z = Z;

    // Convert ECEF to geodetic to get altitude for air density calculation
    double lat, lon, alt;
    ecef_to_geodetic(ecef_x, ecef_y, ecef_z, &lat, &lon, &alt);
    
    float geoid_h = 0.0f;
    egm2008_geoid_height((float)lat, (float)lon, &geoid_h);
    double msl_alt = alt - (double)geoid_h;
    if (msl_alt < 0) msl_alt = 0;

    // Get air density from NRLMSISE-00 model
    struct nrlmsise_output output;
    struct nrlmsise_input input = {0}; // Zero-initialize struct to prevent uninitialized fields (e.g., year)
    struct nrlmsise_flags flags;

    for (int i = 0; i < 24; i++) {
        flags.switches[i] = 1;
    }
    flags.switches[0] = 0; // Use standard cm/g output (multiplied by 1000 below)

    double total_seconds = p->seconds_in_day + t;
    input.doy = p->day_of_year + (int)(total_seconds / 86400.0);
    input.sec = fmod(total_seconds, 86400.0);
    input.alt = msl_alt / 1000.0; // Altitude in km
    input.g_lat = lat;
    input.g_long = lon;
    input.lst = input.sec / 3600.0 + lon / 15.0;
    input.f107A = p->f107A;
    input.f107 = p->f107;
    input.ap = p->ap;
    input.ap_a = NULL;

    // Use gtd7d to include anomalous oxygen, which provides effective total mass density for drag above 500km
    gtd7d(&input, &flags, &output);
    double air_density = output.d[5] * 1000.0; // Convert g/cm^3 to kg/m^3

    double r = sqrt(X * X + Y * Y + Z * Z);
    
    // J2 Gravity Perturbation
    double r2 = r * r;
    double preCommon = 1.5 * WGS84_J2 * (WGS84_A * WGS84_A) / r2;
    double sinLat2 = (Z * Z) / r2;
    double GMOverr3 = G_MU / (r2 * r);
    double gx = -GMOverr3 * (1.0 + preCommon * (1.0 - 5.0 * sinLat2)) * X;
    double gy = -GMOverr3 * (1.0 + preCommon * (1.0 - 5.0 * sinLat2)) * Y;
    double gz = -GMOverr3 * (1.0 + preCommon * (3.0 - 5.0 * sinLat2)) * Z;

    double vrel_x = VX - (-OMEGA_E * Y);
    double vrel_y = VY - (OMEGA_E * X);
    double vrel_z = VZ;
    double vrel_mag = sqrt(vrel_x * vrel_x + vrel_y * vrel_y + vrel_z * vrel_z);

    double drag_coeff = 0.0;
    if (vrel_mag > 1e-3) {
        drag_coeff = -0.5 * air_density * vrel_mag * vrel_mag * p->cd * p->area;
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
    f[3] = gx + (drag_coeff * (vrel_x / (vrel_mag + 1e-9)) + tx) / M;
    f[4] = gy + (drag_coeff * (vrel_y / (vrel_mag + 1e-9)) + ty) / M;
    f[5] = gz + (drag_coeff * (vrel_z / (vrel_mag + 1e-9)) + tz) / M;
    f[6] = -dm;
    return GSL_SUCCESS;
}