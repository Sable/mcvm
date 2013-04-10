% returns all child states, i.e. all possible states the player can reach
% with moves he ('me') can make vs the opponent ('her')
% the rules are "connect four"

function states = play_getChildStates(me,her)
  states = {};
  for i=1:size(me,2) % go through all columns
    if (me(1,i) == 0 && her(1,i) == 0) % can drop down ith row
        j = 1;

		disp(size(me));

        while ((j < size(me,1)) && me(j+1,i) == 0 && her(j+1,i) == 0)
			disp('iterating');
           j = j+1;
        end

        me(j,i) = 1;
        states{end+1} = me; % put child state
        me(j,i) = 0;
    end
  end
end
