# mmap-coding-challenge

This was a coding challenge I did in Summer 2022.  I've posted it here for my own reference or to assist anyone who wants an example of how to dynamically unmap and remap pages using the Unix mmap()/munmap() system call interface.

The Problem:
The program sets up an artificially small virtual address space (to simulate limited physical memory).  It then performs a sequence of 50,000 arbitrary square root calculations.  Eventually, this series of calculations will run into the end of available memory and a page fault will occur.

The Solution:
If you would like to attempt this yourself, delete lines 24 and 38-67.  If you would also like to practice registering a signal handler, 

