%REMOVEVARIABLE A helper function that removes a given pair from a list of
%pairs.  Returns the original list of the pair is not found.
%
%Arguments:
% variable - the pair to remove.
% list - an Nx2 list of pairs from which the specified pair should be removed.
%
%Return:
% new_list - The original list without any occurrences of the specified pair.
function new_list = removevariable(variable, list)
    %separate list into a column of row numbers and a column of column numbers
    var_rows = list(:,1);
    var_cols = list(:,2);
    %a column of booleans indicating whether or not the specified row in the
    %list is a match for variable
    var_mask = var_rows ~= variable(1) | var_cols ~= variable(2);
    new_list = [var_rows(var_mask), var_cols(var_mask)];