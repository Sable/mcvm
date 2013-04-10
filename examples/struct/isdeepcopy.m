function [] = isdeepcopy()

x.a.b = 9 ;
x.a.c = [ 4 5 6 ] ;

foo(x) ;

if x.a.c == [ 4 5 6 ] 
  printf('Deep copy') ;
else
  printf('NO Deep copy') ;
end

end

function [] = foo(param)
param.a.d = 4 ;
param.a.c = 3 ;
end

