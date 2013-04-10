function output = nested_func()

	disp('Calling nested function');
	nested();
	disp('Nest call over');
	
	function [] = nested()
		for i = 1:25
			disp(i);
		end
	end
	
end