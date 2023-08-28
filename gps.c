#include "gps.h"
#include <string.h>
#include "crc.h"
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

enum {
  SENTENCE_GGA = 0,
  SENTENCE_GLL = 1,
  SENTENCE_RMC = 2,
  SENTENCE_UNKNOWN,
};

const char* sentence_prefix[] = { "GGA", "GLL", "RMC" };

gps_error_code_t parse_gga(gps_t gps_instance, const char* sentence, int len);
gps_error_code_t parse_gll(gps_t gps_instance, const char* sentence, int len);
gps_error_code_t parse_rmc(gps_t gps_instance, const char* sentence, int len);
gps_error_code_t gga_get_lat_lon(int* degmin, int* minfrac, char* lat_hemi, char* lon_hemi);
gps_error_code_t gll_get_lat_lon(int* degmin, int* minfrac, char* lat_hemi, char* lon_hemi);
gps_error_code_t rmc_get_lat_lon(int* degmin, int* minfrac, char* lat_hemi, char* lon_hemi);

gps_error_code_t (* const sentence_parsers[])(gps_t, const char*, int) = {
  parse_gga,
  parse_gll,
  parse_rmc,
};

struct gps_instance_t {
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

  var->msg_data = malloc(max(sizeof(struct gps_gga_t), max(sizeof(struct gps_gll_t), sizeof(struct gps_rmc_t))));
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
  
  if (current_sentence == SENTENCE_GGA || current_sentence == SENTENCE_GLL || current_sentence == SENTENCE_RMC) {
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
  } else if (gps_instance->last_msg_type == SENTENCE_RMC) {
    struct gps_rmc_t* rmc = gps_instance->msg_data;
    degmin[0] = (int) rmc->lat;
    minfrac[0] = (int) ((rmc->lat - degmin[0]) * 10000);
    *lat_hemi = rmc->lat_hemi;

    degmin[1] = (int) rmc->lon;
    minfrac[1] = (int) ((rmc->lon - degmin[1]) * 10000);
    *lon_hemi = rmc->lon_hemi;

    return GPS_NO_ERROR;
  }
  return GPS_UNIMPLEMENTED;
}

gps_error_code_t gps_get_time(gps_t gps_instance, struct tm* time) {
  if (gps_instance->last_msg_type == SENTENCE_GGA) {
    struct gps_gga_t* gga = gps_instance->msg_data;
    return GPS_NO_ERROR;
  } else if (gps_instance->last_msg_type == SENTENCE_GLL) {
    struct gps_gll_t* gll = gps_instance->msg_data;
    return GPS_NO_ERROR;
  }
  return GPS_UNIMPLEMENTED;
}

gps_error_code_t gps_get_altitude(gps_t gps_instance, float* msl_metres) {
  if (gps_instance->last_msg_type == SENTENCE_GGA) {
    struct gps_gga_t* gga = gps_instance->msg_data;
    *msl_metres = gga->altitude;
    return GPS_NO_ERROR;
  }
  return GPS_UNIMPLEMENTED;
}

gps_error_code_t gll_get_lat_lon(int* degmin, int* minfrac, char* lat_hemi, char* lon_hemi) {
  if (gps_instance->last_msg_type == SENTENCE_GLL) {
    struct gps_gll_t* gll = gps_instance->msg_data;
    return GPS_NO_ERROR;
  }
  return GPS_UNIMPLEMENTED;
}

gps_error_code_t parse_gga(gps_t gps_instance, const char* sentence, int len) {
  struct gps_gga_t* gga = gps_instance->msg_data;
  int fieldc = 0;
  for (int i = 0, j = 0; i < len; i = j + 1) {
    j = next_field(sentence, len, i);
    if (j == -1) j = len;
    int empty_field = i == j;

    if (fieldc == 1) {
      if (!empty_field) {
        gga->time[0] = sentence[i];
        gga->time[1] = sentence[i + 1];
        gga->time[2] = ':';
        gga->time[3] = sentence[i + 2];
        gga->time[4] = sentence[i + 3];
        gga->time[5] = ':';
        gga->time[6] = sentence[i + 4];
        gga->time[7] = sentence[i + 5];
      }
 } else if (fieldc == 2) {
      if (!empty_field) {
        gga->lat = strtod(sentence + i, NULL);
      }
    } else if (fieldc == 3) {
      if (!empty_field) {
        gga->lat_hemi = sentence[i];
      }
    } else if (fieldc == 4) {
      if (!empty_field) {
        gga->lon = strtod(sentence + i, NULL);
      }
    } else if (fieldc == 5) {
      if (!empty_field) {
        gga->lon_hemi = sentence[i];
      }
    } else if (fieldc == 9) {
      if (!empty_field) {
        gga->altitude = strtof(sentence + i, NULL);
      }
    } else if (fieldc == 11) {
      if (!empty_field) {
        gga->geoid_sep_metres = strtof(sentence + i, NULL);
      }
    }
    fieldc++;
  }
  return GPS_NO_ERROR;
}
gps_error_code_t parse_gll(gps_t gps_instance, const char* sentence, int len) {
  struct gps_gll_t* gll = gps_instance->msg_data;
  int fieldc = 0;
  for (int i = 0, j = 0; i < len; i = j + 1) {
    j = next_field(sentence, len, i);
    if (j == -1) j = len;
    int empty_field = i == j;

    if (fieldc == 5) {
      if (!empty_field) {
        gll->lat = strtod(sentence + i, NULL);
      }
    } else if (fieldc == 6) {
      if (!empty_field) {
        gll->lat_hemi = sentence[i];
      }
    } else if (fieldc == 7) {
      if (!empty_field) {
        gll->lon = strtod(sentence + i, NULL);
      }
    } else if (fieldc == 8) {
      if (!empty_field) {
        gll->lon_hemi = sentence[i];
      }
    }
    fieldc++;
  }
  return GPS_NO_ERROR;
}

struct gps_rmc_t {
  float lat;
  float lon;
  char time[8];
  char lat_hemi;
  char lon_hemi;
} rmc;

gps_error_code_t parse_rmc(gps_t gps_instance, const char* sentence, int len) {
  struct gps_rmc_t* rmc_data = gps_instance->msg_data;
  int fieldc = 0;
  for (int i = 0, j = 0; i < len; i = j + 1) {
    j = next_field(sentence, len, i);
    if (j == -1) j = len;
    int empty_field = i == j;

    if (fieldc == 1) {
      if (!empty_field) {
        rmc_data->time[0] = sentence[i];
        rmc_data->time[1] = sentence[i + 1];
        rmc_data->time[2] = ':';
        rmc_data->time[3] = sentence[i + 2];
        rmc_data->time[4] = sentence[i + 3];
        rmc_data->time[5] = ':';
        rmc_data->time[6] = sentence[i + 4];
        rmc_data->time[7] = sentence[i + 5];
      }
    } else if (fieldc == 3) {
      if (!empty_field) {
        rmc_data->lat = strtod(sentence + i, NULL);
      }
    } else if (fieldc == 4) {
      if (!empty_field) {
        rmc_data->lat_hemi = sentence[i];
      }
    } else if (fieldc == 5) {
      if (!empty_field) {
        rmc_data->lon = strtod(sentence + i, NULL);
      }
    } else if (fieldc == 6) {
      if (!empty_field) {
        rmc_data->lon_hemi = sentence[i];
      }
    }
    fieldc++;
  }
  return GPS_NO_ERROR;
}


