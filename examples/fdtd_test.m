function fdtd_test()

%%
%% Driver for 3D FDTD of a hexahedral cavity with conducting walls.
%%

% Fix the scale
scale = 100;

t1 = clock;

% Parameter initialization.
Lx = .05; Ly = .04; Lz = .03; % Cavity dimensions in meters.
Nx = 25; Ny = 20; Nz = 15; % Number of cells in each direction.

% Because norm isn't currently supported,
% nrm = norm([Nx/Lx Ny/Ly Nz/Lz]) is plugged in.
nrm = 866.0254;

Nt = scale*200; % Number of time steps.

disp('Performing computation');

[Ex, Ey, Ez, Hx, Hy, Hz, Ets] = fdtd(Lx, Ly, Lz, Nx, Ny, Nz, nrm, Nt);

t2 = clock;

% Display result.
% disp(Ex), disp(Ey), disp(Ez);
% disp(Hx), disp(Hy), disp(Hz);
% disp(Ets);
disp(mean(Ex(:))), disp(mean(Ey(:))), disp(mean(Ez(:)));
disp(mean(Hx(:))), disp(mean(Hy(:))), disp(mean(Hz(:)));
disp(mean(Ets(:)));

% Display timings.
fprintf(1, 'FDTD: total = %f\n', (t2-t1)*[0 0 86400 3600 60 1]');


