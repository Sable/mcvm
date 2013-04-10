%FINDMOSTCONSTRAINED Given a list of variables, finds the one that has a domain
%of minimal size.
%
%Arguments:
% unassigned_variables - an Nx2 array which lists the unassigned variables as
%   row-column pairs.
% domains - a cubic array (usually 9x9x9).  domains(row, col, val) will be true
%   if variable (row, col) could be val and false otherwise.
%
%Return:
% variable - a row-column pair indicating a variable with a minimal domain.

function variable = findmostconstrained(unassigned_variables, domains)
    %a value larger than any possible domain size
    min_size = size(domains,3) + 1;
    %a 9x9 matrix where the r,c entry indicates the number of values left in
    %the domain of variable r,c
    domain_sizes = sum(domains,3);
    for i=1:size(unassigned_variables, 1)
        curr = unassigned_variables(i, :);
        currsize = domain_sizes(curr(1), curr(2));
        if currsize < min_size
            min_size = currsize;
            variable = curr;
        end
    end
    