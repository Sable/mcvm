%BUILDCONSTRAINTS A helper function that constructs the list of alldiff
%constraints used throughout the sudoku solving algorithm.
%
%Arguments:
% block_size - the size of a sudoku sub-block.  A typical sudoku puzzle is
%   9x9 and, so, has block_size=3.
% assignment - a square array (usually 9x9) which indicates the current
%   assignment of values to variables.  Variables with non-positive values are
%   considered unassigned.
%
%Return:
% constraints - a 3D array (usually 9x2x27).  constraints(:, :, x) is a (9x2) 
%   list of (9) variables which must satisfy an alldiff constraint.
function constraints = buildconstraints(block_size)
    edge_size = block_size ^ 2;
    constraints = zeros(edge_size,2,3*edge_size);

    %base values for the block alldiff constraints (see below)
    %e.g.   1   1
    %       1   2
    %       1   3
    %       2   1
    %       2   2
    %       2   3
    %       3   1
    %       3   2
    %       3   3
    block_template = ...
            [floor(([1:edge_size] - 1)/block_size); ...
               mod(([1:edge_size] - 1),block_size)]' + 1;
    for i=1:edge_size
        %rows of variables
        %e.g.   7   1
        %       7   2
        %       7   3
        %       7   4
        %       7   5
        %       7   6
        %       7   7
        %       7   8
        %       7   9
        constraints(:,:,i) = [i*ones(1,edge_size); 1:edge_size]';
        
        %columns of variables
        %e.g.   1   7
        %       2   7
        %       3   7
        %       4   7
        %       5   7
        %       6   7
        %       7   7
        %       8   7
        %       9   7
        constraints(:,:,i + edge_size) = [1:edge_size; i*ones(1,edge_size)]';
        
        %blocks of variables
        %e.g.   4   7
        %       4   7
        %       4   7
        %       5   8
        %       5   8
        %       5   8
        %       6   9
        %       6   9
        %       6   9
        %block_row and block_col are zero-based indices into the 
        %block_size x block_size list of sub-blocks in the edge_size x edge_size
        %sudoku board
        block_row = mod(i - 1, block_size);
        block_col = floor((i - 1) / block_size);
        constraints(:,:,i + 2*edge_size) = ...
                [block_template(:,1) + block_row*block_size, ...
                 block_template(:,2) + block_col*block_size];
    end