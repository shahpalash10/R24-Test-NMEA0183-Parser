#include "unity.h"
#include "unity_internals.h"
#include "gps.h"

void test_gga_parsing_over_examples(void) {
    const char* examples[] = { 
        "$GPGGA,002153.000,3342.6618,N,11751.3858,W,1,10,1.2,27.0,M,-34.2,M,,0000*5E",
    };
    gps_t handle = gps_init(); 
    TEST_ASSERT_EQUAL(GPS_NO_ERROR, gps_update(handle, examples[0], strlen(examples[0]))); 
    
    int latlon_degmins[2]; 
    int latlon_minfracs[2]; 
    char lat_hemi[2];
    char lon_hemi[2]; 
    
    gps_get_lat_lon(handle, latlon_degmins, latlon_minfracs, lat_hemi, lon_hemi); 
    
    TEST_ASSERT_EQUAL(3342, latlon_degmins[0]); 
    TEST_ASSERT_EQUAL(6618, latlon_minfracs[0]);
    TEST_ASSERT_EQUAL_STRING("N", lat_hemi); 
    
    TEST_ASSERT_EQUAL(11751, latlon_degmins[1]); 
    TEST_ASSERT_EQUAL(3858, latlon_minfracs[1]);
    TEST_ASSERT_EQUAL_STRING("W", lon_hemi); 
}

void test_geoid_separation(void) {
  const char* examples[] = {
    "$GPGGA,002153.000,3342.6618,N,11751.3858,W,1,10,1.2,27.0,M,-34.2,M,,0000*5E",
  };
  gps_t handle = gps_init();
  TEST_ASSERT_EQUAL(GPS_NO_ERROR, gps_update(handle, examples[0], strlen(examples[0])));
  float geoid_sep;
  gps_get_geoid_sep(handle, &geoid_sep);
  TEST_ASSERT_EQUAL_FLOAT(-34.2, geoid_sep);
}

void test_gll_parsing_over_examples(void) {
  const char* examples[] = {
    "$GPGLL,3723.2475,N,12158.3416,W,161229.487,A,A*41",
  };
  gps_t handle = gps_init();
  TEST_ASSERT_EQUAL(GPS_NO_ERROR, gps_update(handle, examples[0], strlen(examples[0])));
}

void setUp() {}
void tearDown() {}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_gga_parsing_over_examples);
  RUN_TEST(test_geoid_separation);
  RUN_TEST(test_gll_parsing_over_examples);
  return UNITY_END();
}
