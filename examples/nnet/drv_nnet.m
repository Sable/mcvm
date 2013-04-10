% neural network benchmark
% learns AND, OR, and XOR using a neural network
% Anton Dubrau
function drv_nnet(n)
  % learn AND, OR, and XOR
  nnet = nnet_create([2 2 3]);
  inputs = {[0;0];[0;1];[1;0];[1;1]};
  outputs = {[0;0;0], [0;1;1], [0;1;1], [1;1;0]};
  alpha = 0.5;
  numEpoch = round(sqrt(n)*70);
  samplesPerEpoch = round(sqrt(n)*100);
  % create matrizes with training samples
  in = zeros(2,samplesPerEpoch);
  out = zeros(3,samplesPerEpoch);
  for j=1:samplesPerEpoch
     i = mod(j,4)+1;
     in(:,j) = inputs{i};
     out(:,j) = outputs{i};
  end
  for x=1:numEpoch
     nnet = nnet_learn(nnet,in,out,alpha);
  end
  % test
  for i=1:4
      outs(i,:) = nnet_eval(nnet,inputs{i});
  end
  % outs % prints exact results
  round(outs) % prints rounded results - this should be an exact benchmark then
end



