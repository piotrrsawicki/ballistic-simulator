#include "egm2008.h"
#include "egm2008-data.h"
#include <math.h>

/* Grid size in degrees. */
#define EGM2008_SCALE 0.25f

/* Grid dimensions based on s_egm2008grid. */
#define EGM2008_COLS 1441
#define EGM2008_ROWS 721

#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

static float interpolate(float a, float b, float proportion) {
    return a + ((b - a) * proportion);
}

static float lookup_height(int r, int c) {
    /* The s_egm2008grid array is in centimeters, convert to meters. */
    return (float)s_egm2008grid[r * EGM2008_COLS + c] * 0.01f;
}

egm2008_error_t egm2008_geoid_height(float latitude, float longitude, float *out_height) {
    if (latitude < -90.0f || latitude > 90.0f) {
        return EGM2008_ERROR_LATITUDE_OUT_OF_RANGE;
    }
    if (longitude < -180.0f || longitude > 180.0f) {
        return EGM2008_ERROR_LONGITUDE_OUT_OF_RANGE;
    }

    /* Translate and scale latitude.
     * Array starts at 90 (North Pole) and goes down to -90 (South Pole). */
    float lat_trans = (90.0f - latitude) / EGM2008_SCALE;
    int r_low = (int)floorf(lat_trans);
    int r_high = CLAMP(r_low + 1, 0, EGM2008_ROWS - 1);

    float r_scalar = 0.0f;
    if (r_low != r_high) {
        r_scalar = (lat_trans - (float)r_low) / (float)(r_high - r_low);
    }

    /* Translate and scale longitude.
     * Array starts at 0 and goes easterly to 360. */
    float lon_adj = longitude;
    if (lon_adj < 0.0f) {
        lon_adj += 360.0f;
    }
    float lon_trans = lon_adj / EGM2008_SCALE;
    int c_low = (int)floorf(lon_trans);
    int c_high = CLAMP(c_low + 1, 0, EGM2008_COLS - 1);

    float c_scalar = 0.0f;
    if (c_low != c_high) {
        c_scalar = (lon_trans - (float)c_low) / (float)(c_high - c_low);
    }

    /* Look up the offsets at each of the four corners of the bounding box. */
    float h00 = lookup_height(r_low, c_low);
    float h01 = lookup_height(r_low, c_high);
    float h10 = lookup_height(r_high, c_low);
    float h11 = lookup_height(r_high, c_high);

    /* Interpolate a height for the actual lat/lon inside the bounding box. */
    float latitude_lo = interpolate(h00, h10, r_scalar);
    float latitude_hi = interpolate(h01, h11, r_scalar);
    
    if (out_height) {
        *out_height = interpolate(latitude_lo, latitude_hi, c_scalar);
    }

    return EGM2008_SUCCESS;
}
