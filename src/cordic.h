/****************************************************************************
*
* Name: cordic.h
*
* Synopsis: header file for cordic module, in cordic.c
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
/* object construction/destruction */

int cordic_construct(int largest_k);
  /* construct the CORDIC table which will be of size "largest_k + 1".  this
     function must be called before any other. */

void cordic_destruct(void);
  /* call this last, to free the CORDIC table's memory. */


/* CORDIC functions */

void cordic_get_mag_phase(float I, float Q, double *p_mag, double *p_phase_rads);
  /* calculate the magnitude and phase of "I + jQ".  phase is in radians */

void cordic_get_cos_sin(double desired_phase_rads, double *p_cos, double *p_sin);
  /* calculate the cosine and sine of the desired phase in radians */

