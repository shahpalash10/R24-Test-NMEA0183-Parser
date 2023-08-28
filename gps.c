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

struct gps_instance_t
{
  int last_msg_type;
  void* msg_data; 
};

int next_field(const char* sentence, int len, int offset) {
  for (int i = offset; i < len; i++) {
    if (sentence[i] == ',') return i;
  }
  return -1;
}

gps_t gps_init() {
  struct gps_instance_t* var = calloc(1, sizeof(struct gps_instance_t));
  if (var == NULL) {
    return NULL;
  }

  var->msg_data = malloc(max(sizeof(struct gps_gga_t), sizeof(struct gps_gll_t)));
  if (var->msg_data == NULL) {
  
    free(var);
    return NULL;
  }

  return var;
}

gps_error_code_t gps_destroy(gps_t gps_instance) {
  free(gps_instance->msg_data);
  free(gps_instance);
  return GPS_NO_ERROR;
}

gps_error_code_t gps_update(gps_t gps_instance, const char* sentence, int len) {
  if (verify_checksum(sentence, len) == 0) return GPS_INVALID_CHECKSUM;
  int current_sentence = SENTENCE_UNKNOWN;
  for (int i = 0; i < SENTENCE_UNKNOWN; i++) {
    if (strncmp(sentence + 3, sentence_prefix[i], 3) == 0) {
      current_sentence = i;
      break;
    }
  }
  if (current_sentence == SENTENCE_UNKNOWN) {
    return GPS_UNKNOWN_PREFIX;
  }
  
  if (current_sentence == SENTENCE_GGA || current_sentence == SENTENCE_GLL) {
    gps_instance->last_msg_type = current_sentence;
    return sentence_parsers[current_sentence](gps_instance, sentence, len);
  } else {
    return GPS_UNIMPLEMENTED;
  }
}

gps_error_code_t gps_get_lat_lon(gps_t gps_instance, int* degmin, int* minfrac, char* lat_hemi, char* lon_hemi) {
  if (gps_instance->last_msg_type == SENTENCE_GGA) {
    struct gps_gga_t* gga = gps_instance->msg_data;
    degmin[0] = (int) gga->lat;
    minfrac[0] = (int) ((gga->lat - degmin[0]) * 10000);
    *lat_hemi = gga->lat_hemi;

    degmin[1] = (int) gga->lon;
    minfrac[1] = (int) ((gga->lon - degmin[1]) * 10000);
    *lon_hemi = gga->lon_hemi;

    return GPS_NO_ERROR;
  } else if (gps_instance->last_msg_type == SENTENCE_GLL) {
    struct gps_gll_t* gll = gps_instance->msg_data;
  }
  return GPS_UNIMPLEMENTED;
}

gps_error_code_t gps_get_geoid_sep(gps_t gps_instance, float* geoid_sep_metres) {
  if (gps_instance->last_msg_type == SENTENCE_GGA) {
    struct gps_gga_t* gga = gps_instance->msg_data;
    *geoid_sep_metres = gga->geoid_sep_metres;
    return GPS_NO_ERROR;
  }
  return GPS_UNIMPLEMENTED;
}

gps_error_code_t parse_gga(gps_t gps_instance, const char* sentence, int len) {
  struct gps_gga_t* gga = gps_instance->msg_data;
  return GPS_NO_ERROR;
}

gps_error_code_t parse_gll(gps_t gps_instance, const char* sentence, int len) {
  struct gps_gll_t* gll = gps_instance->msg_data;
  return GPS_NO_ERROR;
}
