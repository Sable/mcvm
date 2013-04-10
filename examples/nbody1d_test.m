function nbody1d_test()

%
% Driver for the N-body problem coded using 1d arrays for the
% displacement vectors.
%

% Benchmark execution time scale
scale = 5;

seed = 1;
t1 = clock;

n = round(scale^.4*30); %floor(28*rand);
dT = (.5)*0.0833;
T = (.5)*32.4362*sqrt(scale);

Rx = rand(n, 1,.1)*1000.23;
Ry = rand(n, 1,.4)*1000.23;
Rz = rand(n, 1,.9)*1000.23;

m = rand(n, 1,-.4)*345;

[Fx, Fy, Fz, Vx, Vy, Vz] = nbody1d(n, Rx, Ry, Rz, m, dT, T);

t2 = clock;

% Display result.
% disp(Fx), disp(Fy), disp(Fz);
% disp(Vx), disp(Vy), disp(Vz);
disp(mean(Fx(:))), disp(mean(Fy(:))), disp(mean(Fz(:)));
disp(mean(Vx(:))), disp(mean(Vy(:))), disp(mean(Vz(:)));

% Compute the total
total = (t2-t1)*[0 0 86400 3600 60 1]';

% Display timings.
fprintf(1, 'NBODY1D: n=%f, total = %f\n', n, total);

end

% making random deterministic
function M = rand(m,n,seed)
    seed = seed+m*n;
    M = zeros(m,n);
    for i = 1:m
        for j = 1:n
        	val = mod(seed,1);
            M(i,j) = val;
            newSeed = seed+M(i,j)*sqrt(100)+sqrt(2);
            seed = newSeed;
        end 
    end
end
    