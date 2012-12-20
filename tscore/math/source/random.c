/*
 * Copyright 2011 Andrew Gottemoller.
 *
 * This software is a copyrighted work licensed under the terms of the
 * Trigger Script license.  Please consult the file "TS_LICENSE" for
 * details.
 */

#include <math/random.h>

#include <tsffi/error.h>

#include <math.h>
#include <stdlib.h>


int Math_UniformRandom (
                        struct tsffi_invocation_data* invocation_data,
                        void*                         group_data,
                        union tsffi_value*            output,
                        union tsffi_value*            input
                       )
{
    double uniform_random;

    uniform_random = (double)(rand()+1)/(double)(RAND_MAX+1);

    output->real_data = (tsffi_real)uniform_random;

    return TSFFI_ERROR_NONE;
}

int Math_GaussianRandom (
                         struct tsffi_invocation_data* invocation_data,
                         void*                         group_data,
                         union tsffi_value*            output,
                         union tsffi_value*            input
                        )
{
   double uniform_random1;
   double uniform_random2;
   double x;
   double y;
   double magnitude;
   double random_normal;
   double mean;
   double standard_deviation;

   mean               = (double)input[0].real_data;
   standard_deviation = (double)input[1].real_data;

   do
   {
       uniform_random1 = (double)(rand()+1)/(double)(RAND_MAX+1);
       uniform_random2 = (double)(rand()+1)/(double)(RAND_MAX+1);
       x = (double)2.0*uniform_random1-(double)1.0;
       y = (double)2.0*uniform_random2-(double)1.0;

       magnitude = x*x+y*y;
   }while(magnitude >= (double)1.0);

   random_normal = x*sqrt(((tsffi_real)-(double)2.0*log(magnitude))/magnitude);
   random_normal = mean+random_normal*standard_deviation;

   output->real_data = (tsffi_real)random_normal;

   return TSFFI_ERROR_NONE;
}

