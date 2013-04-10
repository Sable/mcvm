% function creates a neural net, given the layers
% the neural net consists of the weights, a cell array of matrices

function nnet = nnet_create(structure)
    for i=1:(numel(structure)-1) % for all layers
        A = structure(i+1);
        B = structure(i)+1; % treshold
        W = zeros(A,B); % matrix denoting weight between two layers - also threshold
        for j=1:A
           for k=1:B
             W(j,k) = sin(j+sqrt(2)*10*k+A+B)*0.01; % assign 'random' weight
           end
        end
        nnet{i} = W;
    end
end
