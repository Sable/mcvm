function caller_test()

disp('calling function');

out = callee_test();

disp('returned from call');

disp('output is:');
disp(out);

end
