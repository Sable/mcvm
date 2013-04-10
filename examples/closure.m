
function [t2, B] = closure(N)
%-----------------------------------------------------------------------
%
%	This function M-file computes the transitive closure of a
%	directed graph. An adjacency matrix representation of the
%	directed graph is used, which is initialized arbitrarily.
%
%	Invocation:
%		>> [t2, B] = closure(N)
%
%		where
%
%		i. N is the number of nodes in the directed graph,
%
%		o. t2 is the elapsed time in seconds after
%		   initialization,
%
%		o. B is the transitive closure.
%
%	Requirements:
%		N is a positive integer.
%
%	Examples:
%		>> [t2, B] = closure(128)
%
%	Source:
%		Quinn's "Otter" project.
%
%	Author:
%		Yan Zhao (zhao@cs.orst.edu).
%
%	Date:
%		July 1997.
%
%-----------------------------------------------------------------------

% Initialization.
A = zeros(N, N);
for ii = 1:N,
    for jj = 1:N,
	if ii*jj < N/2,
	   A(N-ii, ii+jj) = 1;
	   A(ii, N-ii-jj) = 1;
	end;
	if (ii == jj),
	   A(ii, jj) = 1;
	end;
    end;
end;

B = A;

% Initialization time.
t2 = clock;

% Perform actual work.
ii = N/2;
while ii >= 1,
      B = B*B;
      ii = ii/2;
end;

B = B > 0;


