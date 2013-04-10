function nested_test()

	a = 2;

	function foo()
		disp(a);
	end
	
	fh = @foo;
	
	fh();
	
	a = 4;
	
	fh();
	
end