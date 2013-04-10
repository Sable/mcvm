% checks whether in the matrix M there n true elements in a row, column or
% diagonal - uses some matrix math instead of scalar checks

function out = play_checkWin(M,n)
  M = (M ~= 0); % make sure all elements are logical
  % check rows
  if checkRows(M,n)
  	disp('row win');
      out = true;
      return
  end
  % check columns
  if checkRows(M.',n)
  	disp('col win');
      out = true;
      return
  end
  % check diagonals down right
  if checkDownRight(M,n)
  	disp('down right win');
      out = true;
      return
  end
  % check diagonals up right
  if checkDownRight(M.',n)
  	disp('up right win');
      out = true;
      return
  end  
  out = false;
end


% check whether there are any n true in a any row - assuming M is 0/1 only
function out = checkRows(M,n)
  rows = size(M,2); % get number of rows (i.e. column size)
  if (rows < n)
     out = false;
     return;
  end
  
  check = toeplitz([ones(n,1); zeros(rows-n,1)],[1, zeros(1,rows-n)]); % get checker matrix


  val1 = M*check;

  
  out = sum(sum(val1 >= n)) > 0; % return whether there are elements bigger than n

  
end

% check whether there are any n true in any down right diagonal
% assumes M consists of only 1/0 but not checked
function out = checkDownRight(M,n)
   for i=2:n
      M=M(1:end-1,1:end-1)+M(2:end,2:end);
   end
   
   disp('M:');
   disp(M);
   
   out = sum(sum(M >= 2^(n-1))) > 0;
end




