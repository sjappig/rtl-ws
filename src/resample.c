#include <stdint.h>
#include "resample.h"

#ifdef SHOW_TRACE
#define TRACE(x) printf(x); 
#else
#define TRACE(x)
#endif


int cic_decimate(int R, int filter_order, cmplx* src, int src_len, cmplx_s32* dst, int dst_len)
{
   int src_idx, dst_idx = 0;
   cmplx_s32 integrator_curr_in = {0,0};
   cmplx_s32 integrator_curr_out = {0,0};
   cmplx_s32 integrator_prev_out = {0,0};
   cmplx_s32 comb_prev_in = {0,0};
  
   if (dst_len * R != src_len)
     return -1;
   
   for (src_idx = 0; src_idx < src_len; src_idx++) 
   {
      // integrator y(n) = y(n-1) + x(n)
      set_cmplx_s32_cmplx(&integrator_curr_in, &src[src_idx], 127);
      add(&integrator_curr_in, &integrator_prev_out, &integrator_curr_out);
      
      // resample
      if ((src_idx + 1) % R == 0) 
      {
	 // comb y(n) = x(n) - x(n-1)
	 if (dst_idx >= dst_len)
	 {
	    return -2;
	 }
	 sub(&integrator_curr_out, &comb_prev_in, &dst[dst_idx]);
	 set_cmplx_s32(&comb_prev_in, &integrator_curr_out);
	 dst_idx++;	 
      }
      set_cmplx_s32(&integrator_prev_out, &integrator_curr_out);
   }
      
   return 0;
}

#ifdef TEST

#include <stdio.h>
#include <string.h>

#define CIC_DATA_LEN        24
#define CIC_RESAMPLE_FACTOR 3
#define CIC_RESULT_LEN      (CIC_DATA_LEN/CIC_RESAMPLE_FACTOR)    

int test_cic() 
{
   int i = 0;
   int r = 0;
   cmplx data[CIC_DATA_LEN];
   cmplx_s32 result[CIC_RESULT_LEN];
   memset(result, 0, CIC_RESULT_LEN*sizeof(cmplx_s32));
   for (i = 0; i < CIC_DATA_LEN/2; i++) 
   {
      set_cmplx(&data[i], 127+i, 127+i);
   }
   for (i = CIC_DATA_LEN/2; i < CIC_DATA_LEN; i++)
   {
      set_cmplx(&data[i], data[CIC_DATA_LEN/2-1].re, data[CIC_DATA_LEN/2-1].im);
   }
   
   r = cic_decimate(CIC_RESAMPLE_FACTOR, 1, data, CIC_DATA_LEN, result, CIC_RESULT_LEN);
   
   if (r < 0)
     return r;
   
   for (i = 0; i < CIC_RESULT_LEN; i++) 
   {
      printf("result[%d] == (%d, %d)\n", i, result[i].re, result[i].im); 
   }
      
   return 0;
}


int main(int argc, char** argv) 
{
   int r = 0;
   printf("Testing cic-implementation...\n");
   r = test_cic();
   if (r)
   {
      printf("test FAILED; return value %d\n", r);
   } else 
   {
      printf("test succeeded\n");
   }
   
}

#endif
