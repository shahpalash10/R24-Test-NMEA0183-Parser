#ifndef GPS_H
#define GPS_H

#include <time.h>

struct gps_instance_t;
typedef struct gps_instance_t* gps_t;

typedef enum {
  GPS_NO_ERROR = 0,
  GPS_INVALID_CHECKSUM,
  GPS_UNKNOWN_PREFIX,
  GPS_UNIMPLEMENTED,
  GPS_PARSE_ERROR,
  GPS_NO_TIME = 33,
  GPS_NO_FIX_TYPE = 34,
} gps_error_code_t;

gps_t gps_init();
gps_error_code_t gps_destroy(gps_t gps_instance);
gps_error_code_t gps_update(gps_t gps_instance, const char* sentence, int len);
gps_error_code_t gps_get_lat_lon(gps_t gps_instance, int* degmin, int* minfrac, char* lat_hemi, char* lon_hemi);
gps_error_code_t gps_get_time(gps_t gps_instance, struct tm* time);
gps_error_code_t gps_get_altitude(gps_t gps_instance, float* msl_metres);
gps_error_code_t gps_get_geoid_sep(gps_t gps_instance, float* geoid_sep_metres);

#endif
