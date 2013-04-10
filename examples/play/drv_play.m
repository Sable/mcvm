% this program starts with the empty matrix and tries to find the best
% possible move for the first player.
% the game is 4 in a row.
% -- the rules were adjusted so that the benchmark runs faster,
%    it is now 3 in a row, and the board has size 4x4

function out=drv_play(scale)
  % set scaling of problem --> depth of computation
  n = 5 + (scale > 2) +(scale > 6) +(scale > 12) +(scale > 25) +(scale > 50) +(scale > 100) +(scale > 150);
  factor = 1;
  if (scale > 200) % run multiple times for large scales
     factor = round(scale/170); 
  end
  
  
  % set expected output results
  move = 1;
  value = n > 5;
  if n == 8
      move = 3;
  end

  % find first move in game
  me = zeros(4,4); % start with empty play field
  her = me;
  for i = 1:factor
    [outmove,outvalue] = play_MiniMax(me,her,n,3);
  end
    
  % check results
  out = (outmove == move) && (outvalue == value);

end





