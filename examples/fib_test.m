function [] = fib_test()

% Fibonacci number to compute and expected output
FIB_NUM = 25;
FIB_OUT = 75025;

% Start the timer
tic;

% Compute the fibonacci number
val = fibonacci(FIB_NUM);

% Stop the timer
totalTime = toc;

% Print the total computation time
fprintf(1, 'Total time: %fs\n', totalTime);

% Display the result
disp(val);

% Display whether the result is correct or not
if val == FIB_OUT
    disp('Correct result');
else
    disp('INCORRECT RESULT');
end

end