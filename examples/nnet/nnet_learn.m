% function takes in a neural net, and a sample in and out to return an
% updated net, alhpa is a scalar denoting the learning rate
% in and out are matrizes whose columns are training samples, each denoting
% input and desired output
% denoting the learning rate
% algorithm from (with alterations): Russel,Norvig - 'A.I., modern approach' (2nd), p.746

function nnet=nnet_learn(nnet,ins,outs,alpha)
  layers = numel(nnet)+1; % get total number of layers
  for sample = 1:size(ins,2)
      in = ins(:,sample);
      out = outs(:,sample);
      % get activations
      activations{1} = in;
      for i = 1:(layers-1)
          activations{i+1} = 1./(1+exp(-nnet{i}*[activations{i};-1]));
      end
      % get output error
      errors{layers} = ((activations{layers}.*(1-activations{layers})).*(out-activations{layers})).';
      % update weights, and propagate errors backwards
      for i = (layers-1):-1:1 % for each node in output layer, starting with last but one
          E = errors{i+1}*nnet{i};
          errors{i} = (activations{i} .* (1-activations{i})).' .* E(1:end-1);
          nnet{i} = nnet{i} + alpha*(errors{i+1}.')*[activations{i};-1].';
      end
  end
end
