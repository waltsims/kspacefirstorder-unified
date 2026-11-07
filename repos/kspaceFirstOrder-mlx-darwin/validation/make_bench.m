% k-Wave MATLAB toolbox: set the KWAVE_MATLAB environment variable to its k-Wave folder, or add it to the path.
if ~isempty(getenv('KWAVE_MATLAB')); addpath(genpath(getenv('KWAVE_MATLAB'))); end
outDir = fullfile(pwd, 'bench'); if ~exist(outDir, 'dir'); mkdir(outDir); end
rng(1); dx = 1e-4;
common = {'PMLSize', 'auto', 'PMLInside', false, 'PlotSim', false, 'DataCast', 'single'};
for N = [256, 512]
  % Lossless homogeneous, initial pressure (PML outside -> FFT friendly size)
  kgrid = kWaveGrid(N, dx, N, dx, N, dx);
  medium = struct('sound_speed', 1500, 'density', 1000);
  kgrid.makeTime(medium.sound_speed, 0.3);
  source = struct('p0', makeBall(N, N, N, N/2, N/2, N/2, 8));
  sensor = struct('mask', zeros(N, N, N)); sensor.mask(N/2, :, :) = 1;
  kspaceFirstOrder3D(kgrid, medium, source, sensor, 'SaveToDisk', fullfile(outDir, sprintf('bench_%d_lossless.h5', N)), common{:}, 'Smooth', false);
  % Heterogeneous, nonlinear, power law absorption
  medium.sound_speed = 1500 * ones(N, N, N, 'single'); medium.sound_speed(:, :, N/2:end) = 1700;
  medium.density = 1000 * ones(N, N, N, 'single');  medium.density(:, N/2:end, :) = 1100;
  medium.alpha_coeff = 0.5 * ones(N, N, N, 'single'); medium.alpha_power = 1.5;
  medium.BonA = 6;
  kspaceFirstOrder3D(kgrid, medium, source, sensor, 'SaveToDisk', fullfile(outDir, sprintf('bench_%d_heterog.h5', N)), common{:}, 'Smooth', false);
end
disp('done');
