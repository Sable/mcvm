a = 1;
b = 2;
c = 3;
d = 4;
e = 5;
f = 6;
g = 7;
v1 = a + b;
v2 = a + b * c;
v3 = -v2;
v4 = a + b + c + 2 * d * e + f * g;
disp(v4);

if a == v4 + c + d
	disp(a + b + c);
else
	disp(2);
end

v5 = (a || b) || c;