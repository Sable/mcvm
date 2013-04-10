%matrix = [1 2; 3 4];
%matrixInv = [-2 1; 1.5 -0.5];

%matrix(1, 1:2);
%matrix(:);

%disp('This should be identity:');
%disp(matrix * matrixInv);

%val = ones(3);
%disp(val);
%disp(val(2));

%disp('Scalar multiplication:');
%disp(3 * matrix);

%matrixRec = [
%	1	2	3	4
%];

%disp(matrixRec);
%disp(matrixRec');
%disp(matrixRec'');

A = ones(255, 255);
B = ones(255, 255);

for i = 1:100

	A(:) = B(:);

end