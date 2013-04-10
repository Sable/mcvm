% function [T,W]=RungeKutta4(f,tspan,alpha,N)
% This is an implementation of the Runge-Kutta 4 algorithm for solving the system
%   - y'=f(t,y) 
%   - y(a)=alpha
% over tspan=[a,b].
% f is a function, alpha is a vector (or scalar).
% N=number of steps to be taken to integrate.
% Note that y may be a vector valued function (column vectors).
% The ourputs are
%    - T a vector of all t's used (i.e. N+1 values from tspan)
%    - The approximations, as a matrix whose rows correspond to each yi
%
%
% Kathryn Pelham, Anton Dubrau, group6
% 08.03.08, adapted 10.04.08 to follow Matlab's ode23s syntax
function [T,W]=RungeKutta4(f,tspan,alpha,N)

   % initial set up
   a=tspan(1); % this is the initial time
   b=tspan(2); % this is the final time
   h=(b-a)/N; % step size
   W = zeros(N+1,length(alpha)); % set output matrix
   W(1,:)=alpha'; % set initial value
   T = (a:h:b)'; % set times
   
   % iteration
   j = 1;
   while (j <= N)

       % set k1, k2, k3 and k4
       k1 = h*(feval(f, T(j), W(j,:)'));
       k2 = h*(feval(f, (T(j) + (h/2)), (W(j,:)' + ((1/2)*(k1)))));
       k3 = h*(feval(f, (T(j) + (h/2)), (W(j,:)' + ((1/2)*(k2)))));
       k4 = h*(feval(f, (T(j)+h), (W(j,:)' + k3)));

       % Runge-Kutta 4 - step
       W(j+1,:) = W(j,:)+ ((1/6)*(k1 + 2*k2 + 2*k3 + k4))';

       j = j+1;

   end

end


