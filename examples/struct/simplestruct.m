function simplestruct()
  a = simple_struct_impl(5) ;
  disp(a) ;
end

function [res] = simple_struct_impl(init)
	sum.a = init ;
  sum.offset = 5 ;
  sum.a = sum.a + sum.offset ;
  res = sum.a ;
end
