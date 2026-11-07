% End-to-end test: MATLAB k-Wave calling kspaceFirstOrder-MLX through kspaceFirstOrder3DG/2DG,
% compared with the native MATLAB solvers kspaceFirstOrder3D/2D.
% k-Wave MATLAB toolbox: set the KWAVE_MATLAB environment variable to its k-Wave folder, or add it to the path.
if ~isempty(getenv('KWAVE_MATLAB')); addpath(genpath(getenv('KWAVE_MATLAB'))); end
binPath = [fullfile(fileparts(mfilename('fullpath')), '..', 'build') filesep];
binName = 'kspaceFirstOrder-MLX';

%% 3D heterogeneous absorbing nonlinear medium, time varying source, record p and p_max
Nx = 64; Ny = 64; Nz = 64; dx = 1e-4;
kgrid = kWaveGrid(Nx, dx, Ny, dx, Nz, dx);
medium.sound_speed = 1500 * ones(Nx, Ny, Nz); medium.sound_speed(:, :, 33:end) = 1700;
medium.density = 1000 * ones(Nx, Ny, Nz); medium.density(33:end, :, :) = 1150;
medium.alpha_coeff = 0.75; medium.alpha_power = 1.5; medium.BonA = 6;
kgrid.makeTime(medium.sound_speed, 0.3, 5e-6);
source.p_mask = zeros(Nx, Ny, Nz); source.p_mask(16:48, 16:48, 12) = 1;
source.p = 1e5 * toneBurst(1 / kgrid.dt, 1e6, 3, 'SignalLength', kgrid.Nt);
sensor.mask = zeros(Nx, Ny, Nz); sensor.mask(:, :, 50) = 1;
sensor.record = {'p', 'p_max', 'p_final'};
args = {'PMLSize', 10, 'PlotSim', false, 'DataCast', 'single'};

refData = kspaceFirstOrder3D(kgrid, medium, source, sensor, args{:});
mlxData = kspaceFirstOrder3DG(kgrid, medium, source, sensor, args{:}, 'BinaryPath', binPath, 'BinaryName', binName);

relErr = @(a, b) max(abs(a(:) - b(:))) / max(abs(a(:)));
fprintf('3D  p       max rel error vs MATLAB: %.3e\n', relErr(refData.p, mlxData.p));
fprintf('3D  p_max   max rel error vs MATLAB: %.3e\n', relErr(refData.p_max, mlxData.p_max));
fprintf('3D  p_final max rel error vs MATLAB: %.3e\n', relErr(refData.p_final, mlxData.p_final));

%% 2D homogeneous initial value problem
clear medium source sensor;
Nx = 256; Ny = 256;
kgrid = kWaveGrid(Nx, dx, Ny, dx);
medium.sound_speed = 1500; medium.density = 1000;
kgrid.makeTime(medium.sound_speed, 0.3);
source.p0 = makeDisc(Nx, Ny, 128, 128, 10);
sensor.mask = makeCircle(Nx, Ny, 128, 128, 100);
sensor.record = {'p', 'u'};
refData = kspaceFirstOrder2D(kgrid, medium, source, sensor, args{:});
mlxData = kspaceFirstOrder2DG(kgrid, medium, source, sensor, args{:}, 'BinaryPath', binPath, 'BinaryName', binName);
fprintf('2D  p       max rel error vs MATLAB: %.3e\n', relErr(refData.p, mlxData.p));
fprintf('2D  ux      max rel error vs MATLAB: %.3e\n', relErr(refData.ux, mlxData.ux));
fprintf('2D  uy      max rel error vs MATLAB: %.3e\n', relErr(refData.uy, mlxData.uy));
