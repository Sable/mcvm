function out=yval(j,N)
% function out=yval(j,N)
% This function converts the index of grid-points of an equally spaced
% quadratic grid into the corresponding y value.
% we assume
%    - equally spaced grid with N points in each direction 
%      (i.e. N^2 grid points in total) indexed from 1 to N^2
%      covering the interior(!) of the domain [-10,10]x[-10,10] 
%    - the boundary is not covered by the grid, since we will work with
%      Dirichlet boundary conditions/  
% input: 
%    - N is the number of gridpoints in x and y direction respectively
%    - j (between 1 and N^2) is the number of the grid point 
% output:
%    - yval is the y coordinate of the grid point j 
%
% 09.04.08
% Dustin

out=-10+20/(N+1)*ceil(j/N);