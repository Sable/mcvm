function [val] = fibonacci(n)

if n == 0 || n == 1
    val = n;
else
    val = fibonacci(n-1) + fibonacci(n-2);
end

end