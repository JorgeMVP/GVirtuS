# A GPGPU Transparent Virtualization Component for High Performance Computing Clouds #

The GPU Virtualization Service (gVirtuS) presented in this work tries to fill the gap between in-house hosted computing clusters, equipped with GPGPUs devices, and pay-for-use high performance virtual clusters deployed via public or private computing clouds. gVirtuS allows an instanced virtual machine to access GPGPUs in a transparent and hypervisor independent way, with an overhead slightly greater than a real machine/GPGPU setup. The performance of the components of gVirtuS is assessed through a suite of tests in different deployment scenarios, such as providing GPGPU power to cloud computing based HPC clusters and sharing remotely hosted GPGPUs among HPC nodes.

    https://link.springer.com/chapter/10.1007/978-3-642-15277-1_37

## How to cite GVirtuS in your scientific papers ##

* Montella, R., Coviello, G., Giunta, G., Laccetti, G., Isaila, F., & Blas, J. G. (2011, September). A general-purpose virtualization service for HPC on cloud computing: an application to GPUs. In International Conference on Parallel Processing and Applied Mathematics (pp. 740-749). Springer, Berlin, Heidelberg.

* Montella, R., Kosta, S., Oro, D., Vera, J., Fernández, C., Palmieri, C., ... & Laccetti, G. (2017). Accelerating Linux and Android applications on low‐power devices through remote GPGPU offloading. Concurrency and Computation: Practice and Experience, 29(24), e4286.

* Montella, R., Ferraro, C., Kosta, S., Pelliccia, V., & Giunta, G. (2016, December). Enabling android-based devices to high-end gpgpus. In International Conference on Algorithms and Architectures for Parallel Processing (pp. 118-125). Springer, Cham.

* Montella, R., Giunta, G., & Laccetti, G. (2014). Virtualizing high-end GPGPUs on ARM clusters for the next generation of high performance cloud computing. Cluster computing, 17(1), 139-152.

# How To install GVirtuS framework and plugins#
## Prerequisites: ##
GCC, G++ with C++17 extension (minmum version: 8)

OS: CentOS 7.3 (tested)

CUDA Toolkit: up to Version 9.0

This package are required:
    build-essential
    autotools-dev
    automake
    git
    libtool
    libxmu-dev
    libxi-dev
    libgl-dev
    libosmesa-dev
    liblog4cplus-dev
    libzmq-dev

Ubuntu:
    sudo apt-get install build-essential libxmu-dev libxi-dev libgl-dev libosmesa-dev git liblog4cplus-dev libzmq-dev

CentOS:
    sudo yum install ...

    sudo yum install centos-release-scl

    sudo yum install devtoolset-8-gcc

    scl enable devtoolset-8 bash

## Installation: ##
1) Clone the GVirtuS main repository

   git clone https://github.com/gvirtus/GVirtuS.git 

In the directory “GVirtuS” there are three directories named “gvirtus”, “gvirtus.cudart” and "gvirtus.cudadr".

“gvirtus” contains the framework.

“gvirtus.cudart” and "gvirtus.cudadr" contains the cuda runtime plugin and the cuda driver plugin.


2) Launch the installer script indicating the destination folder of the installation (es. "$HOME/opt/gvirtus"):

    export CUDA_HOME=/usr/local/cuda-9.0 

    export LD_LIBRARY_PATH=$CUDA_HOME/lib64:$LD_LIBRARY_PATH

    export PATH=$CUDA_HOME/bin:$PATH

    export EXTRA_NVCCFLAGS="--cudart=shared"

    export LDFLAGS="-L$CUDA_HOME/lib64"

    export CPPFLAGS="-I$CUDA_HOME/include"

    export GVIRTUS_HOME=$HOME/opt/gvirtus-2019

    mkdir -p $GVIRTUS_HOME

    ./gvirtus-installer $VIRTUS_HOME

To check your installation please check the following directories:

Check $GVIRTUS\_HOME/lib for frontend and backend directories

**NOTE: Be sure to use a toolchain enabled with C++17 features.**

## Testing ##

How GVirtuS works? Could my application work with GVirtuS?

The only way to answer is tring and testing.

A good starting poin is testing GVirtuS aganist the NVIDIA CUDA SAMPLES using the gvirtus-test script.

    ./gvitus-test NVIDA_CUDA_SAMPLES_PATH output_file [avoid_file]

Where:

    NVIDA_CUDA_SAMPLES_PATH is the absolute path of the sample files shipped with the NVIDIA CUDA SdK

    output_file is the CSV file where the results will be saved

    avoid_file is a text file containing the samples that have to be skipped in order to avoid the script stucks.

The script produces a CSV files with reluts.

On the standard output the script prints the error message generated by the sample. This could be used for debugging or in order to improve features.

Example:

    ./gvirtus-test $HOME/dev/cuda-samples/NVIDIA_CUDA-9.0_Samples/ $HOME/dev/GVirtuS-9.0/saturn.saturn.9.0.csv $HOME/dev/GVirtuS-9.0/avoid_file.txt 2>&1 | tee result.txt

## EXAMPLE cuda application ##

### Backend machine (physical GPU and Cuda required) ###

On the remote machine where the cuda executables will be executed

Modify the GVirtuS configuration file backend if the default port 9999 is occuped or the machine is remote:

$GVIRTUS\_HOME/etc/properties.json

    {
        "communicator": [
        {
             "endpoint": {
                 "suite": "tcp/ip",
                 "protocol": "oldtcp",
                 "server_address": "127.0.0.1",
                 "port": "9999"
        },
        "plugins": [
            "cudart",
            "cudadr",
            "cufft",
            "cublas",
            "curand"
            ]
        }
        ],
        "secure\_application": false
    }

Export the dynamic CUDA library:(typically /usr/local/cuda/lib64)


    export GVIRTUS_HOME=$HOME/opt/gvirtus-2019

    export CUDA_HOME=/usr/local/cuda-9.0

    export LD_LIBRARY_PATH=$CUDA/lib64:$GVIRTUS_HOME/lib:$GVIRTUS_HOME/lib/communicator:$GVIRTUS_HOME/lib/backend:$GVIRTUS_HOME/external/lib:$LD_LIBRARY_PATH

    export PATH=$CUDA_HOME/bin:$PATH

Execute application server gvirtus-backend with follow command:

    $GVIRTUS_HOME/bin/gvirtus-backend

### Frontend machine (No GPU or Cuda required) ###

Modify the Gvirtus configuration file frontend:

$GVIRTUS\_HOME/etc/properties.json

    {
        "communicator": [
        {
             "endpoint": {
                 "suite": "tcp/ip",
                 "protocol": "oldtcp",
                 "server_address": "127.0.0.1",
                 "port": "9999"
        },
        "plugins": [
            "cudart",
            "cudadr",
            "cufft",
            "cublas",
            "curand"
            ]
        }
        ],
        "secure\_application": false
    }


**NOTE: In the local configuration GVirtuS Backend and Frontend share the same configuration files.**

Export the dynamic GVirtuS library:

    export  LD_LIBRARY_PATH=GVIRTUS_PATH/gvirtus/lib/frontend

Optionally set a different configuration file

    export CONFIG_FILE=$HOME/dev/properties.json

execute the cuda application compiled with cuda dynamic library (with -lcuda -lcudart)

    ./example

If you are using nvcc be sure you are compiling using shared libraries:

    export EXTRA_NVCCFLAGS="--cudart=shared"


## Logging ##

In order to change the loging level, define the GVIRTUS\_LOGLEVEL environment variable:

    export GVIRTUS_LOGLEVEL=<loglevel>

The <loglevel> value is defined as follows:

    OFF_LOG_LEVEL     = 60000

    FATAL_LOG_LEVEL   = 50000

    ERROR_LOG_LEVEL   = 40000

    WARN_LOG_LEVEL    = 30000

    INFO_LOG_LEVEL    = 20000

    DEBUG_LOG_LEVEL   = 10000

    TRACE_LOG_LEVEL   = 0
