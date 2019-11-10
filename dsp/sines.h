/* Alternative sine functions for benchmarking and qualitive omparission
 */


// One sine quadrant
static const float QuarterSine[64] = {0.0, 0.0245, 0.0491, 0.0736, 0.098, 0.1224, 0.1467, 0.171, 0.1951, 0.2191, 0.243, 0.2667, 0.2903, 0.3137, 0.3369, 0.3599, 0.3827, 0.4052, 0.4276, 0.4496, 0.4714, 0.4929, 0.5141, 0.535, 0.5556, 0.5758, 0.5957, 0.6152, 0.6344, 0.6532, 0.6716, 0.6895, 0.7071, 0.7242, 0.741, 0.7572, 0.773, 0.7883, 0.8032, 0.8176, 0.8315, 0.8449, 0.8577, 0.8701, 0.8819, 0.8932, 0.904, 0.9142, 0.9239, 0.933, 0.9415, 0.9495, 0.9569, 0.9638, 0.97, 0.9757, 0.9808, 0.9853, 0.9892, 0.9925, 0.9952, 0.9973, 0.9988, 0.9997};

static float qtab_sin(float brads_f) {
  const uint_fast8_t brads = ((unsigned long )brads_f) & 0xFF;
  const uint_fast8_t ix = (brads & 0x3F);
  const uint_fast8_t quarter = (brads & 0xC0);
  if (quarter < 128) {
    if (quarter == 0) {
      return QuarterSine[ix];
    } else {
      return (!ix ? 1.0 : QuarterSine[64 - ix]);
    }
  } else {
    if (quarter == 128) {
      return -QuarterSine[ix];
    } else {
      return (!ix ? -1.0 : -QuarterSine[64 - ix]);
    }
  }
  /* The switch case is quite a bit slower.
  switch(quarter) {
    case 0: return QuarterSine[ix];
    case 64: return (!ix ? 1.0 : QuarterSine[64 - ix]);
    case 128: return -QuarterSine[ix];
    case 192: return (!ix ? -1.0 : -QuarterSine[64 - ix]);
    default: return 0;
  }
  */
}

// A full sine
static const float FullSine[256]={
 0.000000, 0.024541, 0.049068, 0.073565, 0.098017, 0.122411, 0.146730, 0.170962, 0.195090, 0.219101, 0.242980, 0.266713, 0.290285, 0.313682, 0.336890, 0.359895,
 0.382683, 0.405241, 0.427555, 0.449611, 0.471397, 0.492898, 0.514103, 0.534998, 0.555570, 0.575808, 0.595699, 0.615232, 0.634393, 0.653173, 0.671559, 0.689541,
 0.707107, 0.724247, 0.740951, 0.757209, 0.773011, 0.788347, 0.803208, 0.817585, 0.831470, 0.844854, 0.857729, 0.870087, 0.881921, 0.893224, 0.903989, 0.914210,
 0.923880, 0.932993, 0.941544, 0.949528, 0.956940, 0.963776, 0.970031, 0.975702, 0.980785, 0.985278, 0.989176, 0.992479, 0.995185, 0.997290, 0.998795, 0.999699,
 1.000000, 0.999699, 0.998796, 0.997291, 0.995185, 0.992480, 0.989177, 0.985278, 0.980786, 0.975702, 0.970032, 0.963776, 0.956941, 0.949529, 0.941545, 0.932993,
 0.923880, 0.914210, 0.903990, 0.893225, 0.881922, 0.870088, 0.857729, 0.844854, 0.831470, 0.817586, 0.803208, 0.788347, 0.773011, 0.757209, 0.740952, 0.724248,
 0.707107, 0.689541, 0.671559, 0.653173, 0.634394, 0.615232, 0.595700, 0.575808, 0.555570, 0.534998, 0.514103, 0.492898, 0.471397, 0.449611, 0.427555, 0.405241,
 0.382683, 0.359895, 0.336889, 0.313681, 0.290284, 0.266712, 0.242979, 0.219100, 0.195089, 0.170961, 0.146729, 0.122410, 0.098016, 0.073563, 0.049066, 0.024540,
 -0.000002, -0.024543, -0.049069, -0.073566, -0.098019, -0.122413, -0.146732, -0.170964, -0.195092, -0.219103, -0.242982, -0.266715, -0.290287, -0.313684, -0.336892, -0.359897,
 -0.382686, -0.405244, -0.427558, -0.449614, -0.471399, -0.492901, -0.514105, -0.535000, -0.555573, -0.575811, -0.595702, -0.615234, -0.634396, -0.653176, -0.671562, -0.689543,
 -0.707109, -0.724250, -0.740954, -0.757211, -0.773013, -0.788349, -0.803210, -0.817587, -0.831472, -0.844856, -0.857731, -0.870089, -0.881923, -0.893226, -0.903991, -0.914212,
 -0.923881, -0.932994, -0.941546, -0.949530, -0.956942, -0.963777, -0.970032, -0.975703, -0.980786, -0.985279, -0.989177, -0.992480, -0.995185, -0.997291, -0.998796, -0.999699,
 -1.000000, -0.999699, -0.998795, -0.997290, -0.995184, -0.992479, -0.989176, -0.985277, -0.980784, -0.975701, -0.970030, -0.963774, -0.956938, -0.949526, -0.941542, -0.932990,
 -0.923877, -0.914207, -0.903986, -0.893221, -0.881918, -0.870083, -0.857725, -0.844850, -0.831465, -0.817581, -0.803203, -0.788342, -0.773006, -0.757204, -0.740946, -0.724242,
 -0.707101, -0.689535, -0.671553, -0.653167, -0.634387, -0.615225, -0.595693, -0.575801, -0.555563, -0.534990, -0.514095, -0.492891, -0.471389, -0.449603, -0.427547, -0.405233,
 -0.382675, -0.359886, -0.336881, -0.313673, -0.290276, -0.266704, -0.242971, -0.219092, -0.195081, -0.170952, -0.146721, -0.122401, -0.098007, -0.073555, -0.049058, -0.024531,
};

static float ftab_sin(float brads_f) {
  const uint_fast8_t brads = ((unsigned long)brads_f) & 0xFF;
  return FullSine[brads];
}

static float ftab_sin_int(float brads_f) {
  const uint_fast8_t brads = ((unsigned long)brads_f) & 0xFF;
  const float f = brads_f - (float)brads;

  // read two nearest values from the sin table and interpolate
  float a = FullSine[brads];
  float b = FullSine[(brads+1) & 0xFF];

  //return (1.0f-f)*a + f*b;
  return a + f * (b - a);
}
