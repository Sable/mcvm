% function psi=SolveSchroedinger(InitialValue,It,h)
% solves the equation
% The Schroedinger equation reads i dPsi/dt = H Psi where (in 2 D)
%       H = 1/2 (Dxx+ Dyy) + V(x,y) I is the Hamiltonian (I is the identity op.)
% i.e. the system reduces to the system
%      y' = f(t,y)
% or 
%      y' = H*y
%      where H is given by the function 'Hamiltonian' in our case.
%
% inputs
%    - InitialValue is the N^2 vector denoting U at t=0, i.e. supplied by 'initval'
%    - [ta tb] = It is the time interval
%    - h is the step size of t used to integrate
%
% ...desparsed

function val=SolveSchroedinger(InitialValue,It,h)
  % get values from arguments
  ta = It(1);
  tb = It(2);
  tsteps = floor((tb-ta)/h); % get # of steps based on stepsize and interval
  h = (tb-ta)/tsteps; % since we rounded, we have to readjust h
  N = sqrt(length(InitialValue));
  fprintf('computing initial value\n');
  InitialValue = InitialValue; % make sure initial value is sparse
  fprintf('computing hamiltonian\n');
  % H = sparse(Hamiltonian(N));   % get the Hamiltonian approximation divided by i
  H = Hamiltonian(N);   % get the Hamiltonian approximation divided by i
  fprintf('computing psi\n');
  % f = @(x,y) H*sparse(y);  % define the function y' = f(t,y)
  f = @(x,y) H*y;  % define the function y' = f(t,y)
  [T,val] = RungeKutta4(f,It,InitialValue,tsteps);
end



