function D=laplacematrix(N)
% function D=laplacematrix(N)
% This function generates a finite difference matrix for a Laplacian on 
% an evenly spaced 2 dimensional grid with Dirichlet boundary conditions
% u=0 on the boundary. 
% N is the number of grid points in each direction 
% i.e. NxN is the number of total grid points 
% Output is a N^2xN^2 matrix.
% FORMULA
% d/dt u_i = 1/h^2 (u_i+1 -4u_i +u_i-1 +u_i+N-u_i-N)
% for u_x not on the boundary. Boundary terms are zero. 
%
% 09.04.08
% Dustin

% (u_i+1 -4u_i +u_i-1) -term (*) 
% corresponding to one-dimensional finite difference matrix for Dxx
% with -4 in the diagonal, coming from the terms corresponding
% to derivatives in y direction. 

% On each 'line' of constant y we have the usual finite difference matrix
% for Dxx on N grid-points:
R=(zeros(1,N));
R(1,1)=-4;
R(1,2)=1;
%A=sparse(toeplitz(R));
A=(toeplitz(R));

% Dxx on the entire grid then corresponds to the blockdiagonal matrix 
% constructed from N copies of A:
D={A};
C=cell(1,N);
C(1,1:N)=D;
B=blkdiag(C{1,1:N});

disp('adding diag...');

% Now add the finite difference terms for y-derivatives
% (u_i-N) -term (for u_i not next to lower boundary)
B=B+diag(ones(1,N^2-N),-N);

% (u_i+N) -term (for u_i not next to upper boundary)
B=B+diag(ones(1,N^2-N),N);


%output
D=B;
