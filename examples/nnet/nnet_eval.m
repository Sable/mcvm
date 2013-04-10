% evaluates a neural net at a given input
% the input has to be a column vector

function out = nnet_eval(nnet,in)
  for i = 1:numel(nnet)
      in = 1./(1+exp(-nnet{i}*[in;-1]));
  end
  out=in;
end