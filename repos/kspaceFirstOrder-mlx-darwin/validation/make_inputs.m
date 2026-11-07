% Generate k-Wave input files covering the main code paths of the C++ solvers.
% Usage: matlab -batch "run('make_inputs.m')"   (from the validation folder)
% k-Wave MATLAB toolbox: set the KWAVE_MATLAB environment variable to its k-Wave folder, or add it to the path.
if ~isempty(getenv('KWAVE_MATLAB')); addpath(genpath(getenv('KWAVE_MATLAB'))); end
outDir = fullfile(pwd, 'inputs');
if ~exist(outDir, 'dir'); mkdir(outDir); end
rng(1);
common = {'PMLSize', 10, 'PlotSim', false, 'DataCast', 'single'};

%% Case 1: 3D, homogeneous, lossless, initial pressure, index sensor mask
Nx = 64; Ny = 64; Nz = 64; dx = 1e-4;
kgrid = kWaveGrid(Nx, dx, Ny, dx, Nz, dx);
medium = struct('sound_speed', 1500, 'density', 1000);
kgrid.makeTime(medium.sound_speed, 0.3, 4e-6);
source = struct('p0', 5 * makeBall(Nx, Ny, Nz, 32, 32, 32, 6));
sensor = struct('mask', zeros(Nx, Ny, Nz));
sensor.mask(randperm(Nx*Ny*Nz, 300)) = 1;
kspaceFirstOrder3D(kgrid, medium, source, sensor, 'SaveToDisk', fullfile(outDir, 'case1_3d_homog_p0.h5'), common{:});

%% Case 2: 3D, heterogeneous, nonlinear, power-law absorption, additive pressure source (many signals),
%%         non power-of-two grid
Nx = 60; Ny = 54; Nz = 48;
kgrid = kWaveGrid(Nx, dx, Ny, dx, Nz, dx);
medium = struct();
medium.sound_speed = 1500 * ones(Nx, Ny, Nz); medium.sound_speed(:, :, 25:end) = 1800;
medium.density     = 1000 * ones(Nx, Ny, Nz); medium.density(:, 30:end, :)    = 1200;
medium.alpha_coeff = 0.75 * ones(Nx, Ny, Nz); medium.alpha_coeff(30:end, :, :) = 2.0;
medium.alpha_power = 1.5;
medium.BonA        = 6 * ones(Nx, Ny, Nz);    medium.BonA(:, :, 1:20) = 9;
kgrid.makeTime(medium.sound_speed, 0.3, 3e-6);
source = struct();
source.p_mask = zeros(Nx, Ny, Nz); source.p_mask(20:24, 20:24, 15) = 1;
nSrc = sum(source.p_mask(:));
t = kgrid.t_array;
source.p = 2e5 * sin(2 * pi * 1e6 * t) .* (1:nSrc)' / nSrc;
source.p_mode = 'additive';
sensor = struct('mask', zeros(Nx, Ny, Nz));
sensor.mask(randperm(Nx*Ny*Nz, 500)) = 1;
kspaceFirstOrder3D(kgrid, medium, source, sensor, 'SaveToDisk', fullfile(outDir, 'case2_3d_heterog_nonlinear_powerlaw.h5'), common{:});

%% Case 3: 2D, heterogeneous, velocity sources (Dirichlet), cuboid (corner) sensor mask
Nx = 128; Ny = 96;
kgrid = kWaveGrid(Nx, dx, Ny, dx);
medium = struct();
medium.sound_speed = 1500 * ones(Nx, Ny); medium.sound_speed(70:end, :) = 2000;
medium.density     = 1000 * ones(Nx, Ny); medium.density(:, 60:end)    = 1500;
kgrid.makeTime(medium.sound_speed, 0.3, 6e-6);
source = struct();
source.u_mask = zeros(Nx, Ny); source.u_mask(30, 30:60) = 1;
source.ux = 1e-2 * sin(2 * pi * 1e6 * kgrid.t_array);
source.uy = 5e-3 * cos(2 * pi * 1e6 * kgrid.t_array);
source.u_mode = 'dirichlet';
sensor = struct('mask', [40, 20, 90, 70; 100, 10, 110, 80]');
kspaceFirstOrder2D(kgrid, medium, source, sensor, 'SaveToDisk', fullfile(outDir, 'case3_2d_velocity_cuboid.h5'), common{:});

%% Case 4: 3D, Stokes absorption, linear, heterogeneous sound speed, single-signal pressure source
%%         (additive-no-correction), cuboid sensor, non-staggered velocity
Nx = 48; Ny = 64; Nz = 40;
kgrid = kWaveGrid(Nx, dx, Ny, dx, Nz, dx);
medium = struct();
medium.sound_speed = 1500 * ones(Nx, Ny, Nz); medium.sound_speed(25:end, :, :) = 1650;
medium.density     = 1000;
medium.alpha_coeff = 0.5;
medium.alpha_power = 2;
medium.alpha_mode  = 'stokes';
kgrid.makeTime(medium.sound_speed, 0.3, 3e-6);
source = struct();
source.p_mask = zeros(Nx, Ny, Nz); source.p_mask(24, 20:40, 20) = 1;
source.p = 1e5 * sin(2 * pi * 1.5e6 * kgrid.t_array);
source.p_mode = 'additive-no-correction';
sensor = struct('mask', [10, 10, 10, 38, 50, 30]');
kspaceFirstOrder3D(kgrid, medium, source, sensor, 'SaveToDisk', fullfile(outDir, 'case4_3d_stokes_cuboid.h5'), common{:});

%% Case 5: 3D, homogeneous, power-law absorption, additive velocity source (uz, many signals)
Nx = 56; Ny = 56; Nz = 56;
kgrid = kWaveGrid(Nx, dx, Ny, dx, Nz, dx);
medium = struct('sound_speed', 1540, 'density', 1050, 'alpha_coeff', 0.5, 'alpha_power', 1.1);
kgrid.makeTime(medium.sound_speed, 0.3, 3e-6);
source = struct();
source.u_mask = zeros(Nx, Ny, Nz); source.u_mask(20:36, 20:36, 12) = 1;
nSrc = sum(source.u_mask(:));
source.uz = 1e-2 * sin(2 * pi * 1e6 * kgrid.t_array) .* linspace(0.5, 1, nSrc)';
source.u_mode = 'additive';
sensor = struct('mask', zeros(Nx, Ny, Nz));
sensor.mask(:, :, 40) = 1;
kspaceFirstOrder3D(kgrid, medium, source, sensor, 'SaveToDisk', fullfile(outDir, 'case5_3d_powerlaw_velocity_additive.h5'), common{:});

%% Case 6: 3D, transducer source
Nx = 64; Ny = 64; Nz = 48;
kgrid = kWaveGrid(Nx, dx, Ny, dx, Nz, dx);
medium = struct('sound_speed', 1540, 'density', 1000);
kgrid.makeTime(medium.sound_speed, 0.3, 4e-6);
tr.number_elements = 16; tr.element_width = 1; tr.element_length = 8; tr.element_spacing = 0;
tr.radius = inf; tr.position = round([1, Ny/2 - 8, Nz/2 - 4]);
tr.sound_speed = 1540; tr.focus_distance = 2e-3; tr.elevation_focus_distance = Inf;
tr.steering_angle = 10; tr.transmit_apodization = 'Rectangular'; tr.receive_apodization = 'Rectangular';
tr.active_elements = ones(tr.number_elements, 1);
tr.input_signal = toneBurst(1 / kgrid.dt, 1.5e6, 3);
transducer = kWaveTransducer(kgrid, tr);
sensor = struct('mask', zeros(Nx, Ny, Nz));
sensor.mask(40, :, Nz/2) = 1;
kspaceFirstOrder3D(kgrid, medium, transducer, sensor, 'SaveToDisk', fullfile(outDir, 'case6_3d_transducer.h5'), common{:});

disp('done');
