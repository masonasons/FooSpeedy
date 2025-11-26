/*
 * speedy_wrapper.h - Wrapper header for Speedy/Sonic integration
 *
 * This header provides the interface to Google's Speedy library using KISS FFT
 * instead of FFTW for better portability.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Use KISS FFT instead of FFTW
#define KISS_FFT 1

// Include the Sonic2 header (which wraps Speedy and Sonic)
#include "sonic2.h"

#ifdef __cplusplus
}
#endif
