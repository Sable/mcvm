function H=Hamiltonian(N)
% function H=Hamiltonian(N)
% The Schroedinger equation reads i dPsi/dt = H Psi where (in 2 D)
% H = 1/2 (Dxx+ Dyy) + V(x,y) I is the Hamiltonian (I is the identity op.)
% 
% The function generates the finite difference approximation 
% of the Hamiltonian on a quadratic grid of 20x20 length units. 
% Note that the output is divided by i, so that the equation to solve is
%            DU/dt = H*U
% we make the following assumptions
%    - equally spaced grid with N points in each direction 
%      (i.e. N^2 grid points in total) indexed from 1 to N^2
%      covering the interior(!) of the domain [-10,10]x[-10,10] 
%    - Dirichlet boundary conditions are used with Psi=0 on the boundary
%    - harmonic potential (with constants set to 1): V(x,y)=1/2(x^2 + y^2)
%    - N is recommended to be odd (so that the center is used)
% input
%    - N is the number of gridpoints in x and y direction respectively
% output
%    - an N^2xN^2 matrix representing the discretized Hamiltonian 
%
% 09.04.08
% Dustin


%% CODE
% V=V(x,y)*I will be the potential times unity matrix
% V=1/2*diag(sparse((xval([1:N^2],N)).^2 + yval([1:N^2],N).^2)); %Harmonic Potential
% V=-1*diag(sparse((xval([1:N^2],N)))); % linear potential
V=-1*diag(((xval([1:N^2],N)))); % linear potential -- made non sparse
% h is the step/width in x- and y- direction
h=20/(N+1);

% Hamiltonian H = 1/i(-1/2 (Dxx + Dyy) + V)
% The functions xval and yval convert the index of the gridpoint into the 
% corresponding x or y coordinates respectively.
H=(1/i)*((1/3)*(1/(2*h^2))*laplacematrix(N)+V);



