% function Z = Newton(F,x0,tol,N,D,o)
% finds a root of the non linear system F(x) = 0 using Newton's
% Method for Systems 
% Refer to Numerical Analysis, Burden, Faires, 8th, Algorithm 10.1 p610
% possible calling sequence:
% Newton(F,x0,tol,N)
% Newton(F,x0,tol,N,D)
% Newton(F,x0,tol,N,D,o)
% With the following arguments:
%   - F:   a n to m dimensional function
%   - x0:  a n dimensional vector denoting the initial guess
%   - tol: the tolerance to which to approximate, i.e. stop when |F(xi)| < tol
%   - N:   the maximum number of iterations before stopping the iteration
%   - D:   a function calculating the derivative of f at x,
%          of the form (f,x,h)->(y'), where
%            - f denotes a f:x->y function
%            - x denotes the point where to evaluate the derivate
%            - the (tolerance?)
%          if D is not supplied, Derive from Derive.m is used.
%   - o:   the following options (default = 0), or them together to apply multiple:
%            - 1 verbose mode: prints warnings etc
% returns a n dimensional vector x s.t. |F(xi)| < tol, or the last
% iterate if the iteration did not produce a close result in N iterations
%
% Anton Dubrau, group  6
% 12.03.08 (last update)
function [Z,n] = Newton(F,x0,tol,N,D,o)

  disp('Newton func');

  switch nargin
    case 4
      [Z,n] = intNewton(F,x0,tol,N,@intDerive,0);
    case 5
      [Z,n] = intNewton(F,x0,tol,N,D,0);
    case 6
      [Z,n] = intNewton(F,x0,tol,N,D,o);
    otherwise
      fprintf('wrong number of arguments, type <help Newton>');
  end

  disp('Done Newton func');


% the private function with a fixed number of arguments
function [Z,n] = intNewton(F,x0,tol,N,D,o)

  disp('Int newton func');

  k = 1;  % iteration step
  jo = 0; % jacobian options are 0
  x = x0; % current iterate
  while(k <= N)
  
    disp('Iterating');
  
    fx= F(x);
    
    disp('Done F');
    
    h = tol; % get h for the derivative operations (todo)
    
    disp('Calling Jacobian func');
    
    J = Jacobian(F,x,D,h,jo);
    
    disp('Done Jacobian call');
    
    y = -J\fx; % solve J*y = -F(x)
    x = x+y; % set new iterate - making sure it's a column vector
    if ((dot(fx,fx)<tol^2) && (dot(y,y)<tol^2)) % f(x) and the current correction have to be within h
      Z = x;
      n = k;
      return 
    end
    k = k+1; % next ietration step
  end
  if ((o & 1) == 1)
    fprintf('Newton method didn''t converge');
  end
  Z = x;
  n = k;

  disp('Done Int newton func');

  
% an internal derivative function
function d = intDerive(f,x,h)
  d = (f(x+h/2)-f(x-h/2))/h; % simple approximation
