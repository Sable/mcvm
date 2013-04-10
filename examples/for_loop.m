function for_loop()

	% Start the timer
	tic;

	sum = 0;

	for i = 1:2:5000000
		v = i;
		sum = sum + v;
	end

	% Stop the timer
	totalTime = toc;

	% Print the total computation time
	fprintf(1, 'Total time: %fs\n', totalTime);

	disp(sum);

end
