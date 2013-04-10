% minimax: uses play_getChildStates and play_checkWin to find the best 
% possible move according to the minimax strategy going n levels down
% returns the best possible move, given as index of the child states
% returned by play_getChildStates, and min and max values.
% returns 0 if there are no legal moves.
% value denotes minimax's possibility for winning; 1 represents winning,
% -1 represents loosing, 0 is a tie.
% win denotes how many elements in a row,column,diagonal are necessary to
% win.
function [move,value]=play_MiniMax(me,her,n,win)

   states = play_getChildStates(me,her);
   if (numel(states) == 0)
      move = 0;
      value = -1;
   end

   % go through all child states checking for winning
   for i = 1:numel(states)
      if play_checkWin(states{i},win)
          value = 1;
          move = i;
          return
      end
   end
   
   if (n > 0) % if we have recursive levels left
		disp('recursion');
       % go through all child states calling minmax recursively - the
       % opponent is trying to make me loose
       maxvalue = -1; % start with assuming we will loose
       move = 1;
         % if any result is -1, the opponent looses -- pick that one -- we can win
         % if any result is 1, the opponent wins
       for i = 1:numel(states)
           [dummy,value] = play_MiniMax(her,states{i},n-1,win); % call minimax recursively
           if (-value > maxvalue) % found better move - note that the opponents good value is my bad value
              move = i;
              maxvalue = -value;
           end
           if (value == -1) % stop if winning move is found
              states{i}+her*2
              break;
           end
       end
   else % base case - we know nothing
       value = 0;
       move = round(numel(states)/2); % choose 'middle' move
		disp('base case');
   end
   fprintf('level %d move %d value %d\n',n,move,value);
end






