#!/bin/bash

op_name=$1

# Define paths and clean up previous files
op_dir="../op/${op_name}"
echo $op_dirs
rm -rf ${op_name}_zip ${op_name}.zip
mkdir ${op_name}_zip

# Copy op_host/ op_kernel/ build_out/custom_*.run files to the specified directory
cp -r ${op_dir}/op_host ${op_name}_zip
cp -r ${op_dir}/op_kernel ${op_name}_zip
cp -r ${op_dir}/build_out/custom_*.run ${op_name}_zip

# Package files
zip -r ${op_name}.zip ${op_name}_zip
