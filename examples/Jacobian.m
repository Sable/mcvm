% function M = Jacobian(F,x,A,B,o)
% calculates the Jacobian Matrix of a vector valued function F at a given x
% possible calling sequence:
% - Jacobian(F,x)
% - Jacobian(F,x,h)
% - Jacobian(F,x,D,h)
% - Jacobian(F,x,D,h,o)
% With the following arguments:
% - F: a n to m dimensional function, represented by a function taking
%      an n dimensional vector and returning and m dimensional one.
%      F should return a real result in a shpere around x with radius h
% - x: the n dimensional column vector where the Jacobian is computed
%      if x is not n dimensional, the behavior of the function is  undefined
% - h: the h used for the derivative (default = 10^(-6)).
% - D: a function calculating the derivative of f at x, 
%      of the form (f,x,h)->(y'), where 
%        - f denotes a f:x->y function, y is a scalar or  a vector
%        - x denotes the point  where to evaluate the derivate
%        - h denotes the tolerance
%      Note that D should work component wise, that is, for a for a
%      function returning a vector, it should return the derivative
%      for every component. The input of f is a sclar. This means
%      D evaulates apprixmately lim((f(x+h)-f(x))*(1/h),h->0).
%      if D is not supplied, f' = [f(x+.5*h)-f(x-0.5*h)]/2 is used.
% - o: the following options (default = 0):
% The Result is returned as a n x m matrix
%
% Anton Dubrau, group6
% 12.03.08 (last update)
function M = Jacobian(F,x,A,B,o)

  disp('Jacobian');

  switch nargin
    case 2
      h = 10^(-6);
      M = intJacobian(F,x,@intDerive,h,0);
    case 3
      M = intJacobian(F,x,@intDerive,A,0);
    case 4
      M = intJacobian(F,x,A,B,0);
    case 5
      M = intJacobian(F,x,A,B,o);
  end

  disp('Done Jacobian');
