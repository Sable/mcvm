% function solveHeatEquation(L,N,Ta,Tb,a,A,Tspan,tsteps)
% solves the heat equation
%    Ut = a*Uxx + A,    x ? [0, 2pi], t > 0
% with boundary counditions being a rod of length T, which has a initially
% a temperature of Ta on the left half, and Tb on the right half. These
% temperatures are maintained at the right and left end. note that A heats
% up the rod.
%
% uses a rod of length L, with N subdivisions, whose initial temperature is
% 0 on the left and T on the right (split in middle). a and A as in above
% equation. tsteps denote how many time steps to take within the time
% interval Tspan

function works=solveHeatEquation(L,N,Ta,Tb,a,A,Tspan,tsteps)
  N = max(N,2); % we need at least two subdivisions, because end points are ommitted
  h = L/N; % step size in x
  X = [h:h:L-h].'; % create x axis with subidivisions
  U0 = X; % create some u0
  U0(1:round(end/2)) = Ta; % set left to Ta
  U0(round(end/2):end) = Tb; % set right to Tb
  [D,c] = Dxx(N+1,Ta,Tb,h); % set derivative matrix
  F = @(t,u) a*(D*u + c) + A; % right hand side of ODE
  out = BackwardEuler(F,Tspan,U0,tsteps);  
  works = abs(max(max(out(:,2:end)))-4.6368) < 0.1;
end
  

% creates a centered-in-space finite difference differentiation matrix,
% for the second spatial derivative.
% uses fixed boundary conditions, i.e. u(0)=a (end)=b.
% takes N, the length of the vector, returns a (N-2)x(N-2) matrix Dxx,
% with the boundary conditions omitted.
% note that 
%     Uxxi = (Ui-1 - 2*Ui + Ui+1)/h^2 for i=2..N-1
%
function [D,c]=Dxx(N,a,b,h)
   % create matrix with -2 as diagonal, 1 above and below diagonal
   D= toeplitz([-2;1;zeros(N-4,1)],[-2,1,zeros(1,N-4)])./h^2;
   % create the constant vector enforcing the boundary conditions
   c = zeros(N-2,1);
   c(1) = a/h^2;
   c(end) = b/h^2;
end


