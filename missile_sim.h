#ifndef MISSILE_SIM_H
#define MISSILE_SIM_H

void geodetic_to_ecef(double lat, double lon, double alt, double *x, double *y, double *z);
void ecef_to_geodetic(double x, double y, double z, double *lat, double *lon, double *alt);
int missile_dynamics(double t, const double y[], double f[], void *params);

typedef struct {
    double thrust;
    double mass_flow;
    double burn_time;
    double dry_mass;
    double area;
    double cd;
    double thrust_dir_ecef[3];
    double theta0;
    // MSIS atmosphere model parameters
    int day_of_year;
    double seconds_in_day;
    double f107A;
    double f107;
    double ap;
} SimParams;

#endif