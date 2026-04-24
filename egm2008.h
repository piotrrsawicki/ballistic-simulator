#ifndef EGM2008_H
#define EGM2008_H

typedef enum {
    EGM2008_SUCCESS = 0,
    EGM2008_ERROR_LATITUDE_OUT_OF_RANGE,
    EGM2008_ERROR_LONGITUDE_OUT_OF_RANGE
} egm2008_error_t;

/* Calculates the geoid undulation (height) in meters for a given latitude and longitude.
 * Latitude is in degrees [-90.0, 90.0]. Longitude is in degrees [-180.0, 180.0]. */
egm2008_error_t egm2008_geoid_height(float latitude, float longitude, float *out_height);

#endif /* EGM2008_H */  