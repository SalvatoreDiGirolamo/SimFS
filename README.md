**Install:**

  1) Build DV 
    - `./build.sh` to install in current_dir/build
    - `sudo ./build.sh install` to install in /usr/bin
    
  2) Build DVLib
    - `./build_dvlib_x86.sh [--netcdf <netcdf path>] [--hdf5_108 <HDF5 1.8 path>] [--hdf5_110 <HDF5 1.10 path>] [--liblsb <liblsb path>]`
    - Note: need only netcdf for the heat equation example

  3) Initialize `heat` environment:
    - `cd examples/heatequation/simulator` 
    - `simfs init heat ../../../dv_config_files/heatequation.dv`
    - Note the heatequation.dv file is created by the DV build process.

  4) Run DV
    - `simfs start heat`
    - If you get a binding error, then specify different ports with: `simfs start heat -C <client_port> -S <server_port>

  5) Run heat equation visulizer 
    - `cd examples/heatequation/client`
    - `simfs run heat python reader2.py`


    
