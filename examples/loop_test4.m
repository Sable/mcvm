%n=integer?
%nmax=parameter
%x,c are complex
%ya,yb,xa,xb,dx,dy
% computes mandelbrot set with N elements and Nmax iterations

function loop_test4()
  N = 19;
  Nmax = 3;
  side = round(sqrt(N));
  ya = -1;
  yb = 1;
  xa = -1.5;
  xb = .5;
  dx = (xb-xa)/(side-1);
  dy = (yb-ya)/(side-1);

     for y=0:side-1

		v0 = y*dy;
		v1 = ya+v0;
		%v1 = v0+ya;
		v2 = i*v1

     end

end

