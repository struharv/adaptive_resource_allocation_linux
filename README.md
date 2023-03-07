# Adaptive Resource Allocation Framework for Real-Time Containers

This is a installation guide for an Adaptive Resource Allocation Framework for Real-Time Containers.

The installation steps are the following
1. Install and configure Linux kernel
2. Make a custom container that supports resource adaptation
3. Define policies to adapt the resources

Dependencies:
- docker

The work is based on the HCBS (Hierarchical Constant Bandwidh Server) patch by Abeni et al. (Link: https://github.com/lucabe72/LinuxPatches/tree/HCBS).   



## 1. Install and Configure Linux Kernel 
The tested Linux Kernel that supports the framework is Linux Kernel v5.2.8:

1. Clone the Linux Kernel 
> git clone --depth 1 --single-branch --branch v5.2.8 git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
2. Install build tools needed for kernel compilation. 
> sudo apt install git build-essential kernel-package fakeroot libncurses5-dev libssl-dev ccache bison flex
3. Create a kernel config based on our current kernel config and loaded modules.
> make localmodconfig
4. Run kernel configurator.
> make menuconfig
5. Go to General setup ─> Control Group Support ─> CPU controller ─> Group scheduling for SCHED_RR/FIFO
6. Go to General setup ─> Kernel .config support and enable access to .config through /proc/config.gz
7. Apply the patch in linx_patch directory 
8. Compile & Install the kernel
> make -j20
> sudo make modules_install -j20
 
> sudo make install -j20
9. Reboot the system


## Adaptive Containers
The source codea of an adaptive rt container can be found in the directory  'rt_container'. 

### Build the container
> make
 
### create a docker container
> make docker
>



CREDITS:
Using HCBS patch 
