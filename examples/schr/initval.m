function u0=initval(N)
% function u0=initval(N)
% We have specified the initial wave-function Psi_0 in the description of 
% the problem. This function will compute Psi_0 at the grid points, 
% yielding the initial value for the linearized problem.
% Input is N, the number of grid points in x and y direction respectively.
% Output is a vector u0 with N^2 entries, representing Psi_0 on the grid
%
% 09.04.08
% Dustin

%% CODE
% C is the normalization factor (see wiki) computed to 12 digits accuracy 
% using composite Simpson's rule with 1 million subintervals
C=0.9820089116592;
% G contains all grid points
G=[1:N^2];
% X contains the x coordinates of the grid points
X=xval(G,N);
% Y contains the y coordinates of the grid points
Y=yval(G,N);

% E evaluates the numerator of Psi_o at the grid points 
E=exp(-1/2*((X).^2+(Y).^2)).*exp(-1./(4-(X).^2)-1./(4-(Y).^2));

% CUTOFF realizes the cutoff of Psi_o outside the square max{|x-1|,|y-1|}<1
% which is the support of Psi_0. 
% The entry will be one iff the corresponding gridpoint lies inside the
% square and zero otherwise
CUTOFF=(abs(X)<2)'.*(abs(Y)<2)';

% u0=1/C*E'.*CUTOFF
% u0=sparse(1/C*E'.*CUTOFF);
u0=(1/C*E'.*CUTOFF);

