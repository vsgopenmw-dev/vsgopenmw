#define RAIN_RIPPLE_DETAIL 1

const float rainRippleGaps = 10.0;
const float rainRippleRadius = 0.2;
const float rainRingTimeOffset = 1.0/6.0;

float scramble(float x, float z)
{
    return fract(pow(fract(x)*3.0+1.0, z));
}

vec2 randOffset(vec2 c, float time)
{
  time = fract(time/1000.0);
  c = vec2(c.x * c.y /  8.0 + c.y * 0.3 + c.x * 0.2,
           c.x * c.y / 14.0 + c.y * 0.5 + c.x * 0.7);
  c.x *= scramble(scramble(time + c.x/1000.0, 4.0), 3.0) + 1.0;
  c.y *= scramble(scramble(time + c.y/1000.0, 3.5), 3.0) + 1.0;
  return fract(c);
}

float randPhase(vec2 c)
{
  return fract((c.x * c.y) /  (c.x + c.y + 0.1));
}

float blip(float x)
{
  x = max(0.0, 1.0-x*x);
  return x*x*x;
}

float blipDerivative(float x)
{
  x = clamp(x, -1.0, 1.0);
  float n = x*x-1.0;
  return -6.0*x*n*n;
}

vec3 circle(vec2 coords, vec2 corner, float adjusted_time)
{
  vec2 center = vec2(0.5,0.5) + (0.5 - rainRippleRadius) * (2.0 * randOffset(corner, floor(adjusted_time)) - 1.0);
  float phase = fract(adjusted_time);
  vec2 toCenter = coords - center;

  float r = rainRippleRadius;
  float d = length(toCenter);
  float ringfollower = (phase-d/r)/rainRingTimeOffset-1.0; // -1.0 ~ +1.0 cover the breadth of the ripple's ring

// normal mapped ripples
  if(ringfollower < -1.0 || ringfollower > 1.0)
    return vec3(0.0);

  if(d > 1.0) // normalize center direction vector, but not for near-center ripples
    toCenter /= d;

  float height = blip(ringfollower*2.0+0.5); // brighten up outer edge of ring; for fake specularity
  float range_limit = blip(min(0.0, ringfollower));
  float energy = 1.0-phase;

  vec2 normal2d = -toCenter*blipDerivative(ringfollower)*5.0;
  vec3 normal = vec3(normal2d, 0.5);
  vec3 ret = normal;
  ret.xy *= energy*energy;
  // do energy adjustment here rather than later, so that [we can use the w component for fake specularity]
  ret = normalize(ret) * energy*range_limit;
  ret.z *= range_limit;
  return ret;
}
vec3 rain(vec2 uv, float time)
{
  uv *= rainRippleGaps;
  vec2 f_part = fract(uv);
  vec2 i_part = floor(uv);
  float adjusted_time = time * 1.2 + randPhase(i_part);
#if RAIN_RIPPLE_DETAIL > 0
  vec3 a = circle(f_part, i_part, adjusted_time);
  vec3 b = circle(f_part, i_part, adjusted_time - rainRingTimeOffset);
  vec3 c = circle(f_part, i_part, adjusted_time - rainRingTimeOffset*2.0);
  vec3 d = circle(f_part, i_part, adjusted_time - rainRingTimeOffset*3.0);
  vec3 ret;
  ret.xy = a.xy - b.xy/2.0 + c.xy/4.0 - d.xy/8.0;
  // z should always point up
  ret.z  = a.z  + b.z /2.0 + c.z /4.0 + d.z /8.0;
  //ret *= 1.5;
  return ret;
#else
  return circle(f_part, i_part, adjusted_time) * 1.5;
#endif
}

vec2 complex_mult(vec2 a, vec2 b)
{
    return vec2(a.x*b.x - a.y*b.y, a.x*b.y + a.y*b.x);
}
vec3 rainCombined(vec2 uv, float time) // returns ripple normal
{
  return
    rain(uv, time)
  + rain(complex_mult(uv, vec2(0.4, 0.7)) + vec2(1.2, 3.0),time)
    #if RAIN_RIPPLE_DETAIL == 2
      + rain(uv * 0.75 + vec2( 3.7,18.9),time)
      + rain(uv * 0.9  + vec2( 5.7,30.1),time)
      + rain(uv * 1.0  + vec2(10.5 ,5.7),time)
    #endif
  ;
}
