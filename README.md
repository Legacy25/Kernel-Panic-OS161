# Kernel-Panic-OS161

This is an implementation of kernel synchronization primitives, basic system calls and virtual memory on top of OS161, a toy operating system, done as assignments in a graduate operating systems class at the University at Buffalo. Go to [ops-class.org](http://www.ops-class.org/asst/0) for more details about the assignments.

# How to get it to run

1. Download the Virtual Machine from [here](http://www.ops-class.org/download/ops-class.org.ova) or alternatively, using a BitTorrent file, [torrent](http://www.ops-class.org/download/ops-class.ova.torrent). 

2. Boot up the VM and launch a terminal. Clone the repository locally to ```~/src``` folder.

3. ```cd``` into ```src/``` and run ```./confinstall```. This will install the kernel. 

4. Now ```cd``` to ```~/root/``` and run ```sys161 kernel```. You will have booted up my modded OS161.

# What was implemented

OS161 on its own, in its unmodded state can just run one user process. It has no system calls. Memory management is almost non-existent, appropriately named ```dumbvm```. Memory that is alocated to user programs is not reclaimed when its terminated, meaning you cannot keep the simulator running indefinitely, it will just run out of memory at some point. More importantly, there is no swapping, meaning you cannot execute programs that need more memory than the amount of RAM physically available to the simulator, which is based on the MIPS 3000 architecture, which means RAM is usually something like 8K or 16K.

We implemented higher level kernel level synchronization primitives like locks, condition variables and reader-writer locks.

System calls for process management (```fork(), exec(), wait(), exit()```) and file system operations (```open(), close(), read(), write(), lseek()```)

At this point you could run several user processes concurrently as kernel threads.

We then put in an entirely revamped memory management system, that uses address spaces. It reclaims freed memory properly, and can swap pages in from and out to disk, meaning that the final modded OS can run programs requiring more memory than physically available.

# Quick Note

If you are looking at this repository, chances are you are taking a class on Operating Systems, or specifically, CSE521 at the University at Buffalo. Hello future OpsClassers, this message is for you!

Please do not consider looking at the code here to help with the assignments. Not even just for pointers, exuse the pun. The path to doing the assignments may seem hopeless at times but it is a lot simpler than it looks. Do not be intimidated by the existing codebase and the assembly bombs strewn about everywhere. You can do it on your own (with your team-mate off course)! Ask for help from legitimate sources, the TAs and ninjas! When you finish it, you'll consider yourself a rockstar.

Do not violate the academic integrity guidelines. You <i>will</i> be caught! Its not worth it. :)
