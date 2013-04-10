%UPDATEDOMAINS Given a new value-to-variable assignment, updates the domains
%of variables in alldiff constraints with the modified variable.
%
%Arguments:
% variable - a pair indicating which variable has been modified.
% value - the new value of the variable.
% domains - a cubic array (usually 9x9x9).  domains(row, col, val) will be true
%   if variable (row, col) could be val and false otherwise.
% constraints - a 3D array (usually 9x2x27).  constraints(:, :, x) is a (9x2) 
%   list of (9) variables which must satisfy an alldiff constraint.
%
%Return:
% new_domains - domains with newly illegal values set to false.
% num_affected - the number of domains in which a value was set to false.
%  (useful for heuristics)
function [new_domains, num_affected] = updatedomains(variable, value, domains, constraints)
    new_domains = domains;
    num_affected = 0;
    %each face in the constraints array is a list of alldiff variables
    
    disp('start updatedomains');
    
    for face=1:size(constraints,3)
    
        %extract the alldiff list from the array
        diff_vars = constraints(:,:,face);
     
        
        %find variables other than the one being assigned to
        affected_vars = removevariable(variable, diff_vars);

        n = size(affected_vars, 1);
        
        if n ~= size(diff_vars, 1) %i.e. if variable is in the list
            for i=1:n %remove from the other domains

                row = affected_vars(i, 1);
                
                col = affected_vars(i, 2);
            
                %if the value is not already false, set it to false and
                %increment the counter
                
                fprintf('value = %f\n', value);
                
                if new_domains(row, col, value) ~= 0
          
                    num_affected = num_affected + 1;
                                        
                    new_domains(row, col, value) = 0;
                    
                end
              
                
            end
        end
        
        
    end
    