%ALLDIFF Confirms that the values assigned to a given list of variables are
%distinct.
%
%Arguments:
% assignment - a square array (usually 9x9) which indicates the current
%   assignment of values to variables.  Variables with non-positive values are
%   considered unassigned.
% variables - an Nx2 array which lists the variables whose values are to be
%   checked as row-column pairs.
%
%Return:
% result - true if all of the assigned variables being considered have distinct
%   values and false otherwise.
function result = alldiff(assignment, variables)
    %build a list of the values of the specified variables
    N=size(variables,1);
    values=zeros(N,1);
    for i = 1:N
        var = variables(i, :);
        val = assignment(var(1), var(2));
        values(i) = val;
    end
    values = values(values > 0); %only consider positive values
    %return true if the number of values is equal to the number of unique values
    result = isequal(size(values), size(unique(values)));