/****************************************************************************
*
* Name: cordic.c
*
* Synopsis: Demonstration of the CORDIC algorithm
*
* Description:
*
*  This module demonstrates the CORDIC algorithm, which can calculate the
*  trigonometric functions of sin, cos, magnitude and phase using
*  multiplying factors which are powers of two.  Therefore, the multiplies
*  can be implemented in hardware as shift/adds.  (Since C doesn't let you
*  shift "doubles", the shift/add multiplies appear here as literal
*  multiplies, but you get the idea.)
*
*  This module is designed as an "object" since it requires a data table
*  which must be filled once prior to use.  Call the "cordic_construct"
*  function first to allocate and build the table, then call the CORDIC
*  functions, and finally call "cordic_destruct" to free the table.
*
*  This modules isn't highly "bulletproofed" and also is not completely
*  optimum because it is intended primarily to demonstrate the algorithm.
*
* Copyright 1999  Grant R. Griffin
*
*                          The Wide Open License (WOL)
*
* Permission to use, copy, modify, distribute and sell this software and its
* documentation for any purpose is hereby granted without fee, provided that
* the above copyright notice and this license appear in all source copies. 
* THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY OF
* ANY KIND. See http://www.dspguru.com/wol.htm for more information.
*
*****************************************************************************/

#include <stdlib.h>
#include <math.h>
#include "cordic.h"


/* define a couple of constants */
#ifndef NULL
  #define NULL ((void *) 0)
#endif

#ifndef PI
  #define PI       3.141592653589793
  #define HALF_PI  (PI * 0.5)
#endif

#define EVAL_CORDIC(l) K = mp_cordic_table[l].K; phase_rads = mp_cordic_table[l].phase_rads; tmp_I = I; if (Q >= 0.0) { I += Q * K; Q -= tmp_I * K; acc_phase_rads -= phase_rads; } else { I -= Q * K; Q += tmp_I * K; acc_phase_rads += phase_rads; }

#define INVALID_K -1

typedef struct tagCORDIC_TABLE {
  float K;
  double phase_rads;
} CORDIC_TABLE;

CORDIC_TABLE *mp_cordic_table = NULL;
int m_max_L = INVALID_K;
double m_mag_scale;


/****************************************************************************/
int cordic_construct(int max_L)
{
  double K = 0;
  int L = 0;

  cordic_destruct();        /* free previous table, if any */

  mp_cordic_table = (CORDIC_TABLE *) calloc(max_L + 1, sizeof(CORDIC_TABLE));
  if (!mp_cordic_table) {
    return 0;    /* failed to calloc table */
  }

  K = 1.0f;
  for (L = 0; L <= max_L; L++) {
    mp_cordic_table[L].K = K;
    mp_cordic_table[L].phase_rads = (double) atan(K);
    K *= 0.5f;
  }

  m_max_L = max_L;

  /* get m_mag_scale by getting the cordic magnitude with m_mag_scale = 1.0 */
  m_mag_scale = 1.0;
  cordic_get_mag_phase(1.0, 0.0, &m_mag_scale, NULL);
  m_mag_scale = 1.0 / m_mag_scale;

  return 1;    /* success */
}

/****************************************************************************/
void cordic_destruct(void)
{
  free(mp_cordic_table);
  mp_cordic_table = NULL;
  m_max_L = INVALID_K;
}

/****************************************************************************/
void cordic_get_mag_phase(float I, float Q, double *p_mag, double *p_phase_rads)
{
  register int L;
  register float tmp_I = 0;
  register float K = 0; 
  register double phase_rads = 0;
  register double acc_phase_rads = 0;

  if (I < 0) {
    /* rotate by an initial +/- 90 degrees */
    tmp_I = I;
    if (Q > 0.0f) {
       I = Q;       /* subtract 90 degrees */
       Q = -tmp_I;
       acc_phase_rads = -HALF_PI;
    } else {
       I = -Q;      /* add 90 degrees */
       Q = tmp_I;
       acc_phase_rads = HALF_PI;
    }
  } else {
    acc_phase_rads = 0.0;
  }

  /* rotate using "1 + jK" factors */
  for (L = 0; L <= m_max_L; L++) {
    /*EVAL_CORDIC(L);
    EVAL_CORDIC(L+1);
    EVAL_CORDIC(L+2);
    EVAL_CORDIC(L+3);*/
    K = mp_cordic_table[L].K;
    phase_rads = mp_cordic_table[L].phase_rads;
    tmp_I = I;
    if (Q >= 0.0) {
      // phase is positive: do negative roation
      I += Q * K;
      Q -= tmp_I * K;
      acc_phase_rads -= phase_rads;
    } else {
      // phase is negative: do positive rotation
      I -= Q * K;
      Q += tmp_I * K;
      acc_phase_rads += phase_rads;
    }
  }
  /*switch (m_max_L % 4)
  {
    case 3:
        EVAL_CORDIC(m_max_L - 3);
    case 2:
        EVAL_CORDIC(m_max_L - 2);
    case 1:
        EVAL_CORDIC(m_max_L - 1);
    case 0:
        EVAL_CORDIC(m_max_L);
  }*/

    if (p_phase_rads != NULL)
        *p_phase_rads = -acc_phase_rads;

    if (p_mag != NULL)
        *p_mag = I * m_mag_scale;
}

/****************************************************************************/
void cordic_get_cos_sin(double desired_phase_rads, double *p_cos, double *p_sin)
{
  double I, Q, tmp_I;
  double acc_phase_rads, phase_rads, K;
  int L;

  /* start with +90, -90, or 0 degrees */
  if (desired_phase_rads > HALF_PI) {
    I = 0.0;
    Q = 1.0;
    acc_phase_rads = HALF_PI;
  } else if (desired_phase_rads < -HALF_PI) {
    I = 0.0;
    Q = -1.0;
    acc_phase_rads = -HALF_PI;
  } else {
    I = 1.0;
    Q = 0.0;
    acc_phase_rads = 0.0;
  }

  /* rotate using "1 + jK" factors */
  for (L = 0; L <= m_max_L; L++) {
    K = mp_cordic_table[L].K;
    phase_rads = mp_cordic_table[L].phase_rads;
    tmp_I = I;
    if (desired_phase_rads - acc_phase_rads < 0.0) {
      /* do negative roation */
      I += Q * K;
      Q -= tmp_I * K;
      acc_phase_rads -= phase_rads;
    } else {
      /* do positive rotation */
      I -= Q * K;
      Q += tmp_I * K;
      acc_phase_rads += phase_rads;
    }
  }

  *p_cos = I * m_mag_scale;
  *p_sin = Q * m_mag_scale;
}
