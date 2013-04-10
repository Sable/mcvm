%ASSIGNMENTCONSISTENT Checks whether a given assignment satisfies a given list
%of alldiff constraints.
%
%Arguments:
% assignment - a square array (usually 9x9) which indicates the current
%   assignment of values to variables.  Variables with non-positive values are
%   considered unassigned.
% constraints - a 3D array (usually 9x2x27).  constraints(:, :, x) is a (9x2) 
%   list of (9) variables which must satisfy an alldiff constraint.
%
%Return:
% result - true if the assignment is consisten with the results and false
%   otherwise.
function result = assignmentconsistent(assignment, constraints)
    for i=1:size(constraints, 3)
        if not(alldiff(assignment, constraints(:,:,i)))
            result = 0;
            return;
        end
    end
    result = 1;