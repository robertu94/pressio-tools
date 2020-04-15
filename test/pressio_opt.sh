#!/bin/bash

eval_script="local cr = metrics['size:compression_ratio'];
local se = metrics['spatial_error:percent'];
local threshold = .05;
local result = 0.0
if se > threshold then
  result = -math.huge;
else
  result = cr;
end
return 'objective', result;
"

mpiexec pressio \
  -a settings -a compress -a decompress \
  -d 1800 -d 3600 -t float \
  -i ${HOME}/git/datasets/cesm/PRECT_1_1800_3600.f32 \
  -M 'all' \
  -m 'size' \
  -m 'spatial_error' \
  -n "composite:scripts=${eval_script}" \
  -N 'all' \
  -O 'all'  \
  -b 'dist_gridsearch:search=fraz' \
  -b 'opt:compressor=sz' \
  -b 'opt:search_metrics=progress_printer' \
  -b 'opt:search=dist_gridsearch' \
  -o 'dist_gridsearch:num_bins=5' \
  -o 'dist_gridsearch:overlap_percentage=.1' \
  -o 'opt:inputs=sz:rel_err_bound' \
  -o 'opt:lower_bound=0' \
  -o 'opt:upper_bound=1e-6' \
  -o 'opt:local_rel_tolerance=0.1' \
  -o 'opt:global_rel_tolerance=0.01' \
  -o 'opt:max_iterations=100' \
  -o 'opt:output=composite:objective' \
  -o 'opt:output=size:compression_ratio' \
  -o 'opt:output=spatial_error:percent' \
  -o 'opt:prediction=1e-5' \
  -o 'opt:do_decompress=1' \
  -o 'opt:objective_mode_name=max' \
  -o 'sz:error_bound_mode=1' \
  opt
  
