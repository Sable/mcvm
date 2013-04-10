%Driver for fast fourier transform
%input is n randomly generated complex numbers stored in an array of size 2*n
%n must be a power of 2
% transf = fft_four1(data,n,1) computes forward transform
% 1/n * fft_four1(transf,n,-1) computes backward tranform


function [] = loop_test2_drv(scale)
	if scale > 0
	 
		n = 1024*1024*4*2^round(log2(scale));
		data = rand(1,2*n);

		t1 = clock;
		out = loop_test2(data,n,1);
		t2 = clock;
	   
		% Display timings.
		fprintf(1, 'FFT: total = %f\n', (t2-t1)*[0 0 86400 3600 60 1]');
	   
	end
end


