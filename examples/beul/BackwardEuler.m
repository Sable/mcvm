% function out = BackwardEuler(f,tspan,alpha,N,tol,n,o)
% Computes the solution of the system
%   y' = f(t,y)
%   y(a) = alpha
% in range tspan = [a b]. N is the number of steps. f,alpha can be vector
% valued (column vectors). The return will be a matrix whose first column
% is the vector of all used t (t0,t1,..,tN); the remaining columns will
% denote the values of y at ti. (y(t0),y(t1),..,y(N)).
% Optional arguments:
%  - tol (default = 1/1000): 
%      sets the tolerance for the Newton Root finding algorithm, this is
%      set in relation to h, (xi+1-xi = (b-a)/N), i.e. the actual value
%      passed to newton will be tol*h.
%  - n (default = 500):
%      sets the numbe of maximum iterations for the Newton rootfinding
%      algorithm
%  - o (default = 0):
%      options: 1 will set verbose mode (progress information will be printed using tic,toc)
%
% Anton Dubrau
% 10.03.08
function out = BackwardEuler(f,tspan,alpha,N,tol,n,o)
  % set up initial values
  a = tspan(1);
  b = tspan(2); % get interval
  h = (b-a)/N; % get step size
  W = zeros(length(alpha),N+1); % allocate memory for results
  W(:,1) = alpha; % set init value
  T = a:h:b; % set all T's
  % setting optional arguments and help values
  if (nargin < 5); tol = 1/1000; end;
  if (nargin < 6); n = 500; end;
  if (nargin < 7); o = 0; end;
  if (o & 1 ==1 ); tic; time = 0; end; % set variables for progress report

  % perform iterations
  i = 1;
  while(i <= N)
    % print information if needed
    if ((mod(i/N*100,1)==0) && (o & 1 == 1))
        time = time + toc; tic; % get time
        fprintf('BackwardEuler: %1.0f%%, elapsed: %1.1fsec., ',i/N*100,time); % print info
        fprintf('left: %1.1fsec. (%1.1fsec total).\n',time*N/i-time,time*N/i);
    end
    % actual iteration step
    wi1 = W(:,i); % set w_i
    ti2 = T(i+1); % set t_i+1
    z = @(wi2) (wi1 + h*f(ti2,wi2) - wi2); % set functon whose root is w_i+1 -> to column vector
    W(:,i+1) = Newton(z,wi1,h*tol,n); % solve to find w_i+1 -> to row vector
    i = i + 1;
  end
  out = [T' W'];
  if (o & 1 ==1 ); x=toc; end; % untic by toc 
end

