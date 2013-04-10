function loop_test3()

err = 1;

for i = 1:10

      err = 0;
      
      for l = 2:10,
      
		for k = 2:10,
		      if (err <= abs(pi))
        	      err = abs(pi);
	      	end;
		end;
	  
		err;
	  
      end;
      
      err;
      
end;

err;