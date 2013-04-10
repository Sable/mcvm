function switch_test(A)

switch A
	case 1
		disp('c1');
	case {2,3,4}
		disp('c2');
	otherwise
		disp('ot');
end