% The original algorithim in C can be found in
% Numerical recipes : the art of scientific computing / William H. Press ... [et al.]

% Computes the discrete fourier transform for complex data type
% data: contains the complex data values whose real and
% imaginary parts are stored in contigous locations in the array data;
% size of data is 2 * n (the number of complex numbers in the data points).
% isign: is a flag whose value is either 1 or -1; if isign is 1, forward
% transform is computed and backward transform otherwise.

function result = loop_test2(data, n, isign)

  nn = n;
  mmax = 2;
  while (nn > mmax)

     istep = mmax * 2;
     theta = isign * (6.28318530717959/mmax);
     wtemp = sin(0.5*theta);
     wpr = -2.0*wtemp*wtemp;
     wpi=sin(theta);
     wr=1.0;
     wi=0.0;

     for m=2:2:mmax
         
         for i=m:istep:nn
             j=i+mmax;
             tempr=wr*data(j-1)-wi*data(j);
             tempi=wr*data(j)+wi*data(j-1);
             data(j-1)=data(i-1)-tempr;
             data(j)=data(i)-tempi;
             data(i-1) = data(i-1) + tempr;
             data(i) = data(i) + tempi;
         end

         wtemp = wr;
         wr = wtemp * wpr - wi*wpi+wr;
         wi = wi*wpr+wtemp*wpi+wi;
     end

     mmax = istep;
  end

  result = data;

end
