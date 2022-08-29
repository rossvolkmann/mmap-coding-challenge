# mmap-coding-challenge

This was a coding challenge I did in Summer 2022.  I've posted it here for my own reference or to assist anyone who wants an example of how to dynamically unmap and remap pages using the Unix mmap()/munmap() system call interface.

# The Problem
The program sets up an artificially small virtual address space (to simulate limited physical memory).  It then performs a sequence of 50,000 arbitrary square root calculations.  Eventually, this series of calculations will run into the end of available memory and a page fault will occur.

# The Solution
If you would like to attempt this yourself, delete lines 24 and 38-67.  If you would also like to practice registering a signal handler, 

 * My solution was to add a signal handler function which is registered to handle the SIGSEGV (page fault) signal.  The address of the segfault is extracted from the siginfo_t parameter.  

 * A page is then unmapped using munmap().  The first time an unmapping occurs NULL will be passed into munmap() allowing the OS to choose an appropriate page.  All subsequent times a the handler is called the previously remapped page is unmapped.  

 * Then, a new page is mapped using mmap().  By passing mmap() the fault address, mmap returns the start address of a newly mapped page which begins at the page boundary which was nearest the fault.  

 * The return value from this mapping is assigned to a pointer.  This pointer is used to calculate the offset in virtual memory since the start of calculations, and pass the index of the next calculation into the calculate function.  

# How to run this code

mmap is a single file C solution compiled for Ubuntu 22.04.1 LTS.  You may want to recompile it using the provided Makefile.  If you are running this on a Mac or Windows machine you will need to manually compile mmap.c with the following gcc command:

$ gcc mmap.c -lm -o mmap
