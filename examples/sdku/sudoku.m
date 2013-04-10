%SUDOKU Solves a sudoku puzzle.
%
%Arguments:
% given_values - a rectangular array of given values.  Non-positive entries will
%   be assumed to indicate blank squares.
%	NB: input is NOT validated.
% block_size - the size of a sudoku sub-block.  A typical sudoku puzzle is
%   9x9 and, so, has block_size=3.
%	NB: not correlated with given_values.
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
% given - a square array (block_size^2 x block_size^2) giving the initial
%	assignment.  (i.e. the input interpreted as a sudoku problem).  Note that
%	this may not be the same as the input.  For example, a 10x13 input to a 9x9
%	problem will be cropped.
% solution - a square array (block_size^2 x block_size^2) giving a solution 
%	(i.e. a consistent assignment) to the sudoku problem if one is found, and
%	[] otherwise.
function [given, solution] = sudoku( ...
		given_values, ...
		block_size, ...
		enable_constraint_propagation, ...
		enable_forward_checking, ...
		enable_most_constrained_variable, ...
		enable_least_constraining_value)
		
    edge_size = block_size^2;
    
    constraints = buildconstraints(block_size);
    
    %initialized to a list of all variables: ((a,b):1<=a,b<=edge_size^2)
    unassigned_variables = ...
    		[floor(([1:edge_size^2] - 1)/edge_size); ...
    		   mod(([1:edge_size^2] - 1),edge_size)]' + 1;
    
	%initially, all values are possible for all variables, so mark all entries
	%as true
    domains = ones(edge_size,edge_size,edge_size);
    
    %iterate over the input and make the assignments indicated thereby
    %update unassigned_variables and domains as needed
    %note: all non-positive entries are converted to zeroes
    assignment = zeros(edge_size, edge_size);
    for r=1:min(edge_size, size(given_values,1))
        for c=1:min(edge_size, size(given_values,2))
            variable = [r c];
            value = given_values(r,c);
            if(value > 0)
                assignment(r,c) = value;
                unassigned_variables = removevariable(variable, ...
                		unassigned_variables);
                if enable_constraint_propagation
                    domains = updatedomains(variable, value, domains, ...
                    		constraints);
                end                
            end
        end
    end
    
    given = assignment;
    
    %if the initial input is not consistent, then return failure immediately
    if not(assignmentconsistent(assignment, constraints))
        solution = [];
    else
        solution = backtrackingsearch( ...
        		assignment, ...
        		unassigned_variables, ...
        		domains, ...
        		constraints, ...
        		enable_constraint_propagation, ...
        		enable_forward_checking, ...
        		enable_most_constrained_variable, ...
        		enable_least_constraining_value);
    end