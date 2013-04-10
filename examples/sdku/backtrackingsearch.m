%BACKTRACKINGSEARCH Runs a recursive backtracking search on the given
%assignment.
%
%Arguments:
% assignment - a square array (usually 9x9) which indicates the current
%   assignment of values to variables.  Variables with non-positive values are
%   considered unassigned.
% unassigned_variables - an Nx2 array which lists the unassigned variables as
%   row-column pairs.
% domains - a cubic array (usually 9x9x9).  domains(row, col, val) will be true
%   if variable (row, col) could be val and false otherwise.
% constraints - a 3D array (usually 9x2x27).  constraints(:, :, x) is a (9x2) 
%   list of (9) variables which must satisfy an alldiff constraint.
% enable_constraint_propagation - true if domains should be updated after each
%   assignment and false otherwise.
% enable_forward_checking - true if each call should initially check for empty
%   domains and fail if there are any.
%   NB: this flag is useless without constraint propagation.
% enable_most_constrained_variable - true if the most constrained variable
%   heuristic should be used and false otherwise.  If the flag is set, then
%   the (a) variable with the smallest domain will be the next to be assigned.
%   If the flag is not set, then the variables will be assigned sequentially.
%   NB: this flag is useless without constraint propagation.
% enable_least_constraining_value - true if the least constraining value
%   heuristic should be used and false otherwise.  If the flag is set, then
%   the values will be tried in increasing order of the number of domains that
%   they shrink.  If the flag is not set, then the values will be tried
%   sequentially.
%   NB: this flag is useless without constraint propagation.
%
%Return:
% result - a consistent assignment if one exists and [] otherwise.
function result = backtrackingsearch( ...
        assignment, ...
        unassigned_variables, ...
        domains, ...
        constraints, ...
        enable_constraint_propagation, ...
        enable_forward_checking, ...
        enable_most_constrained_variable, ...
        enable_least_constraining_value)

    if isempty(unassigned_variables)
        result = assignment; %success
        return;
    end
    
    if enable_forward_checking
        
        %sum(domains, 3) results in a 9x9 matrix where the r,c entry indicates
        %the number of values left in the domain of variable r,c.
        if any(any(sum(domains, 3) == 0))
            result = []; %failure
            return
        end
        
    end
    
    if enable_most_constrained_variable
        var = findmostconstrained(unassigned_variables, domains);
    else
        var = unassigned_variables(1, :);
    end
    
    unassigned_variables = removevariable(var, unassigned_variables);

    %the domain of the selected variable as a string of bits
    dom = squeeze(domains(var(1), var(2), :));
    
    %convert the bit string domain into a list of possible values
    possible_values = 1:length(dom);
    possible_values = possible_values(dom == 1);
    
    if enable_least_constraining_value
        %for each possible value, determine how many domains it will shrink
        %by calling updatedomains
        N = length(possible_values);
        numdomains = zeros(N, 1);
        disp(possible_values);
        for i=1:N
            %note that the new domain is discarded
            value = possible_values(i)
            fprintf('bts1 value = %f\n', value);
            [newdomains numdomains(i)] = updatedomains(var, value, domains, constraints);
        end
                
        %sort the list of numbers of affected domains
        [sorted indices] = sort(numdomains);        
        
        %discard the sorted list and use the indices to sort the list of
        %possible values
        if isempty(indices)
            %For some reason, matlab has a 0x1 array that's not the same as
            %[].  Convert it to [] because otherwise there are problems with
            %the for-loop.
            possible_values = [];
        else 
        	disp('numdomains:');
        	disp(numdomains);
        	disp('indices:');
        	disp(indices);
        	disp('possible vals:');
        	disp(possible_values);
        
            possible_values = possible_values(indices);
        end
    end
    
    for value = possible_values
        %don't need to copy the assignment to test this value - it will be
        %overwritten next iteration if it doesn't work
        assignment(var(1), var(2)) = value;
        if assignmentconsistent(assignment, constraints)
            if enable_constraint_propagation
                %always update the ORIGINAL domains, never the new domains
                fprintf('bts2 value = %f\n', value);
                newdomains = updatedomains(var, value, domains, constraints);
            else
                newdomains = domains;
            end
            %the recursive call receives the same values as the original except
            %that there will be one new assignment and the domains will have
            %changed
            recursive_result = backtrackingsearch( ...
            		assignment, ...
            		unassigned_variables, ...
            		newdomains, ...
            		constraints, ...
            		enable_constraint_propagation, ...
            		enable_forward_checking, ...
            		enable_most_constrained_variable, ...
            		enable_least_constraining_value);
            %if the recursive call succeeded, then return
            %otherwise, try the next possible value
            if not(isempty(recursive_result))
                result = recursive_result; %success
                return;
            end
        end
    end 

    result = []; %failure