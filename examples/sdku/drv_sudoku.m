% sudoku solver benchmark

function out=drv_sudoku(scale)

% load the puzzles
for i = 1:9
    name = sprintf('puzzle%d.in',i-1);
    puzzle{i} = load(name);
    sizes{i} = sqrt(sqrt(numel(puzzle{i})));
end
% load the results
for i = 1:9
    name = sprintf('puzzle%d.solution',i-1);
    solution{i} = load(name);
end


% run the puzzles
puzzles =  7+(scale>=3)+(scale > 4); % leave out last puzzles if we need quick
for time = 1:ceil(scale/6)
    for i = 1:puzzles
        if (sudoku(puzzle{i},sizes{i},1,1,1,1)~=solution{i})
            out = false;
            return
        end
    end
end
out = 1;
end

