% function M=psiXYplane(psi,i)
% returns the XY plane at time index i of a psi object.
% input
%    psi  - structure as returned by solveSchroedinger or Bohmian
%    i    - time index of which to return plane
% output
%    a (N+2)x(N+2) matrix with all values of the grid, including boundary
%    points. meshgrid of this matrix corresponds to the values psi.xgrid
%    and psi.ygrid of the psi object. i.e. use
%       surf(psi.xgrid,psi.ygrid,psiXYplane(psi,10))
%   to plot the values of psi at time index 10.
%
%  12.04.08
%  Anton Dubrau
function M=psiXYplane(val,i)
   V = val(i,:)';
   N = sqrt(length(V)); % the number of internal elements in each dimension
   M = zeros(N+2,N+2);
   h = 20/(N+1); % distance between grid points

   for xi = 1:N % go across all x's
       for yi = 1:N % go across all y's
           Is = ival(-10+xi*h+h/3,-10+yi*h+h/3,N); % get indizes of coressponding point in plane
           i = Is(2,1); % get index xi,yi
           if (i == 0) % check whether this index is 0
               fprintf('!'); % this is not suppossed to happen and denotes a slight (corrected error)
           else
               c = V(Is(2,1)); % get value at xi,yi
               M(xi+1,yi+1) = c; % get value at bottom left index
           end
       end
    end
end



