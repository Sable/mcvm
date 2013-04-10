function drv_schroedinger(scale)
  if (scale < 5)
    SolveSchroedinger(initval(40),[0 .1],0.01);
  elseif (scale < 12)
    SolveSchroedinger(initval(50),[0 .01],0.001);
  else 
    SolveSchroedinger(initval(60),[0 .01*round(scale/16)],0.001);
  end      
end