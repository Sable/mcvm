% function out=ival(x,y,N)
% This function converts from a position in the 2d equally spaced
% quadratic grid into the corresponding index value.
% we assume
%    - equally spaced grid with N points in each direction 
%      (i.e. N^2 grid points in total) indexed from 1 to N^2
%      covering the interior(!) of the domain [-10,10]x[-10,10] 
%    - the boundary is not covered by the grid, since we will work with
%      Dirichlet boundary conditions
% input: 
%    - N is the number of gridpoints in x and y direction respectively
%    - x,y denotes a coordinate in the grid
% output:
%    - a 2x2 matrix denoting all the indizes of the points on the grid
%      which enclose x,y, in the same oriantation as the grid. Note that
%      the index 0 denotes values outside of the matrix, their value is 0.
%
% 09.05.08
% Anton

function out=ival(x,y,N)
  x = x+10; % coordinate relative to bottom left corner of grid
  y = y+10;
  rx = x/20; % coordinate of point relative to bottom in range [0 1]
  ry = y/20;
  nx = floor((N+1)*rx); % coordinates in grid, floored (rounded to -inf)
  ny = floor((N+1)*ry); 
  if (nx < 1 || ny < 1 || nx > N || ny > N) % set bottom left corner
      out(2,1) = 0;
  else
      out(2,1) = nx+(ny-1)*N;
  end;
  if ((nx+1) < 1 || ny < 1 || (nx+1) > N || ny > N) % set bottom right corner
      out(2,2) = 0;
  else
      out(2,2) = (nx+1)+(ny-1)*N;
  end;
  if (nx < 1 || (ny+1) < 1 || nx > N || (ny+1) > N) % set top left corner
      out(1,1) = 0;
  else
      out(1,1) = nx+ny*N;
  end;
  if ((nx+1) < 1 || (ny+1) < 1 || (nx+1) > N || (ny+1) > N) % set top right corner
      out(1,2) = 0;
  else
      out(1,2) = (nx+1)+ny*N;
  end;
end
