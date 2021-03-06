#ifndef __SINCOS_H__
#define __SINCOS_H__

// Scale a floating point frequency to a 32-bit fixed point value
// This scaling is a but funny, see compute_dphase for a full
// exmplanation
#define MAKE_FREQUENCY(frequency) ((int32_t)(frequency*(((int32_t)1)<<7)))

// compute the next value of sin and cos after moving one
// d_phase step
void sincos_step(int16_t *sin, int16_t *cos, int16_t d_phase);
// compute the phase step for one sample at the specified frequency
int16_t compute_dphase(int32_t frequency, int32_t sample_rate);
// fill a buffer of values using an initial amplitude and phase
void fill_buffer(int16_t *sin, int16_t *cos, int16_t dphase, int16_t *sinValues, int n);

#endif // __SINCOS_H__
