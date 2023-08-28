#include "gps.h"
#include <string.h>
#include "crc.h"
#include <stdlib.h>
#include <stdint.h>

enum {
  SENTENCE_GGA = 0,
  SENTENCE_GLL = 1,
  SENTENCE_GSA = 2,
  SENTENCE_GSV = 3,
  SENTENCE_MSS = 4,
  SENTENCE_RMC = 5,
  SENTENCE_VTG = 6,
  SENTENCE_UNKNOWN,
};
const char* sentence_prefix[] = { "GGA", "GLL", "GSA", "GSV",
                                    "MSS", "RMC", "VTG" };
gps_error_code_t parse_gga(gps_t gps_instance, const char* sentence, int len);
gps_error_code_t parse_gll(gps_t gps_instance, const char* sentence, int len);
gps_error_code_t gga_get_lat_lon(int* degmin, int* minfrac, char* lat_hemi, char* lon_hemi);
gps_error_code_t gll_get_lat_lon(int* degmin, int* minfrac, char* lat_hemi, char* lon_hemi);
gps_error_code_t (* const sentence_parsers[])(gps_t, const char*, int) = {
  parse_gga,
  parse_gll,
};
struct gps_gga_t {
  float lat;
  float lon;
  char time[8];
  int satc;
  float hdop;
  float altitude;
  float geoid_sep_metres;
  char lat_hemi;
  char lon_hemi;
} gga;
struct gps_gll_t {
  float lat;
  float lon;
  float time;
  int flags;
} gll;

struct gps_instance_t
{
  int last_msg_type;
};

int next_field(const char* sentence, int len, int offset) {
  for (int i = offset; i < len; i++) {
    if (sentence[i] == ',') return i;
  }
  return -1;
}
gps_t gps_init() {
  struct gps_instance_t* var = calloc(1, sizeof(struct gps_instance_t));
  return var;
}
gps_error_code_t gps_destroy(gps_t gps_instance) {
  free(gps_instance);
  return GPS_NO_ERROR;
}
gps_error_code_t gps_update(gps_t gps_instance, const char* sentence, int len) {
  if (verify_checksum(sentence, len) == 0) return GPS_INVALID_CHECKSUM;
  int current_sentence = SENTENCE_UNKNOWN;
  for (int i = 0; i < SENTENCE_UNKNOWN; i++) {
    if (strncmp(sentence+3, sentence_prefix[i], 3) == 0) {
      current_sentence = i;
      break;
    }
  }
  if (current_sentence == SENTENCE_UNKNOWN) {
    return GPS_UNKNOWN_PREFIX;
  }
  
  if (current_sentence == SENTENCE_GGA || current_sentence == SENTENCE_GLL)
    return sentence_parsers[current_sentence](gps_instance, sentence, len);
  else
    return GPS_UNIMPLEMENTED;
}
gps_error_code_t gps_get_lat_lon(gps_t gps_instance, int* degmin, int* minfrac, char* lat_hemi, char* lon_hemi) {
  if (gps_instance->last_msg_type == SENTENCE_GGA)
    return gga_get_lat_lon(degmin, minfrac, lat_hemi, lon_hemi);
  if (gps_instance->last_msg_type == SENTENCE_GLL)
    return gll_get_lat_lon(degmin, minfrac, lat_hemi, lon_hemi);
  return GPS_UNIMPLEMENTED;
}
gps_error_code_t gps_get_time(gps_t gps_instance, struct tm* time) {
  return GPS_UNIMPLEMENTED;
}
gps_error_code_t gps_get_altitude(gps_t gps_instance, float* msl_metres) {
  return GPS_UNIMPLEMENTED;
}
gps_error_code_t gga_get_lat_lon(int* degmin, int* minfrac, char* lat_hemi, char* lon_hemi) {
  degmin[0] = (int) gga.lat;
  minfrac[0] = (int) ((gga.lat - degmin[0])*10000);
  *lat_hemi = gga.lat_hemi;
  
  degmin[1] = (int) gga.lon;
  minfrac[1] = (int) ((gga.lon - degmin[1])*10000);
  *lon_hemi = gga.lon_hemi;
  
  return GPS_NO_ERROR;
}
gps_error_code_t gps_get_geoid_sep(gps_t gps_instance, float* geoid_sep_metres) {
  if (gps_instance->last_msg_type == SENTENCE_GGA) {
    *geoid_sep_metres = gga.geoid_sep_metres;
    return GPS_NO_ERROR;
  }
  else return GPS_UNIMPLEMENTED;
}
gps_error_code_t parse_gga(gps_t gps_instance, const char* sentence, int len) {
  gps_instance->last_msg_type = SENTENCE_GGA;
  int fieldc = 0;
  for (int i = 0, j = 0; i < len; i = j+1) {
    j = next_field(sentence, len, i);
    if (j == -1) j = len;
    int empty_field = i == j;

    if (fieldc == 1) {
    
      if (empty_field) return GPS_NO_TIME;
      strncpy(gga.time, sentence, 6);
    }
    else if (fieldc == 2) {
    
      if (!empty_field)
        gga.lat = strtod(sentence+i, NULL);
    }
    else if (fieldc == 3) {
   
      if (!empty_field && sentence[i] == 'S')
        gga.lat_hemi = 'S';
      else
        gga.lat_hemi = 'N';
    }
    else if (fieldc == 4) {
  
      if (!empty_field)
        gga.lon = strtod(sentence+i, NULL);
    }
    else if (fieldc == 5) {
   
      if (!empty_field && sentence[i] == 'W')
        gga.lon_hemi = 'W';
      else
        gga.lon_hemi = 'E';
    }
    else if (fieldc == 6) {
      
      if (empty_field) return GPS_NO_FIX_TYPE;
      gga.time[7] |= (sentence[i] != '0') << 3;
    }
    else if (fieldc == 7) {
      if (!empty_field)
        gga.satc = strtol(sentence+i, NULL, 10);
    }
    else if (fieldc == 8) {
      if (!empty_field)
        gga.hdop = strtod(sentence+i, NULL);
    }
    else if (fieldc == 9) {
      if (!empty_field)
        gga.altitude = strtod(sentence+i, NULL);
    }
    else if (fieldc == 10) {
      if (!empty_field)
        gga.geoid_sep_metres = strtod(sentence+i, NULL);
    }
    fieldc++;
  }
  return GPS_NO_ERROR;
}
gps_error_code_t gll_get_lat_lon(int* degmin, int* minfrac, char* lat_hemi, char* lon_hemi) {
  return GPS_UNIMPLEMENTED;
}
gps_error_code_t parse_gll(gps_t gps_instance, const char* sentence, int len) {
  gps_instance->last_msg_type = SENTENCE_GLL;
  int fieldc = 0;
  for (int i = 0, j = 0; i < len; i = j+1) {
    j = next_field(sentence, len, i);
    if (j == -1) j = len;
    int empty_field = i == j;

    if (fieldc == 1) {
      
      if (!empty_field)
        gll.lat = strtod(sentence+i, NULL);
    }
    else if (fieldc == 2) {
    
      if (!empty_field && sentence[i] == 'S')
        gll.flags &= 0b10;
    }
    else if (fieldc == 3) {

      if (!empty_field)
        gll.lon = strtod(sentence+i, NULL);
    }
    else if (fieldc == 4) {

      if (!empty_field && sentence[i] == 'W')
        gll.flags |= 0b01;
    }
    else if (fieldc == 5) {

      if (empty_field) return GPS_NO_TIME;
      gll.time = strtod(sentence+i, NULL);
    }
    else if (fieldc == 6) {
      if (empty_field) return GPS_NO_FIX_TYPE;
      gll.flags = (sentence[i] == 'A') << 3;
    }
    fieldc++;
  }
  return GPS_NO_ERROR;
}
