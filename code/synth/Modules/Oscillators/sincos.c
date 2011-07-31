#include <stdint.h>
#include <math.h>
#include "sincos.h"

int16_t Sin=0, Cos=0x7000, dphase=3361;

// take one euler integral step of a simple harmonic oscillator
// d/dt sin(omega t) =  omega * cos(omega t)
// d/dt cos(omega t) = -omega * sin(omega t)
// To integrate, determine phase advance = omega*dt, this is the
// same for each step so it can be pre-computed
// on the first step, sin and cos should be initialized based on the
// desired initial phase and amplitude
inline void sincos_step(int16_t *sin, int16_t *cos, int16_t d_phase)
{
  //int32_t savesin;

  //savesin = *sin;

  *sin += (((int32_t)(*cos))*((int32_t)d_phase))>>16;

  //*sin += __builtin_bfin_multr_fr1x16(*cos,d_phase);

  //*cos -= (savesin*((int32_t)d_phase))>>16;
  *cos -= (((int32_t)(*sin))*((int32_t)d_phase))>>16;

 //*cos -= __builtin_bfin_multr_fr1x16(*sin,d_phase);

}

// Pre-compute phase advance for each sample
// The scaling I chose allows a maximum frequency of 32768Hz
// The frequency is scaled by 1<<7 and 2*pi is scaled by
// 1<<6, giving a maximum value of ~80% the maximum possible
// in a 32-bit number. This leaves a factor of 1<<3 to turn the
// result into a frac16. This factor is incorporated into
// the sample rate before division
int16_t compute_dphase(uint32_t frequency, uint16_t sample_rate)
{
  return (frequency*TWOPI_32)/(sample_rate>>3);
}

// fill a buffer of length n with a sinusoid
// sin,cos initial values set initial phase and amplitude
// dphase computed from frequency and sample rate as above
// example call:
// int32_t sin=0, cos=0x3FFF;
// int16_t dphase;
// int16_t buffer[128];
// dphase = compute_dphase(MAKE_FREQUENCY(493.883), 48000);
// fill_buffer(&sin, &cos, dphase, buffer, 128);
void fill_buffer(int16_t *sin, int16_t *cos, int16_t dphase, int16_t *sinValues, int n)
{
//  int i;
//  for(i=0;i<n;i++) {
    sinValues[0] = *sin;
    sincos_step(sin, cos, dphase);
//  }

}

#ifdef TEST
#include <stdio.h>
#include <stdlib.h>
// simple test, which prints a table of
// values of sin and cosine
// gcc -DTEST sincos.c -o sincos
// ./sincos 493.883 > sinetable
// gnuplot -p -e "plot './sinetable' using 1:2 title 'sine', './sinetable' using 1:3 title 'cosine';"
#define SAMPLE_RATE ((uint16_t)48000)
int main(int argc, char *argv[])
{
  int32_t frequency = MAKE_FREQUENCY(atof(argv[1]));
  int16_t sin=0, cos=0x7000, dphase;
  int32_t sample;

  dphase = compute_dphase(frequency, SAMPLE_RATE);

  printf("# frequency: %d dphase: %d\n", frequency, dphase);

  for(sample=0; sample<1000;sample++) {
    printf("%d %d %d\n", sample, sin, cos);
    sincos_step(&sin, &cos, dphase);
  }
  return 0;
}
#endif

