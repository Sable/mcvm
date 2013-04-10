function works = drv_EulerSolverDriver(scale)
 works = solveHeatEquation(10,70,3,4,.2,0.05,[0 20],round(scale*10));

