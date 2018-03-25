# arm-kernel
- Supports synchronous channels (inspired by MPI, via msgsend and msgreceive calls). Programs waiting for channel responses will
  not be scheduled, thus no time is wasted (i.e. receivers do not have to poll for senders to push
  data to a channel, and vice versa).
- No type restriction on data that can be pushed onto channel (there is a size limit that is easily configurable
  by adjusting the specified amount in the mqueue.h source file).

- Supports inode based directories / files (can open using paths).
- No limit to the depth of directory trees.
- Supports various command line instructions (cd, ls, mkdir, rm, cp, mv, cat, run <path> (fork/exec), kill 
  <pid> (terminates program), wipe (formats the disc), setp <priority> <path> (sets priority of program
  permanently), stats <path> (displays size of file on disc)).
- Files resize as necessary - can use posix file functions (open, close, write, read, lseek, unlink)
- Supports direct and indirect blocks (filesize limit of ~1GB, however, has only been tested to around 400kB).

- I particularly like how data blocks are allocated to inodes / deallocated from inodes:
  - linked list of available data block addresses maintained in superblock. Head of list
    points to a data block containing another list of available data block addresses etc.
  - When allocating a block: if list size >1 then remove address at back of list, else copy 
    list of addresses pointed to by head of superblock list to superblock list and return the old head.
    - If list only contains pointer to superblock (block address 1), no free data blocks exist (disk full).
  - When deallocating a block: if list not full then add freed address to end of list, else
    copy superblock list to freed address and set freed address to head of superblock list (list size now 1).
  - Particularly fast when superblock list is maintained in memory (done on boot).
  Enables fast allocation of data blocks without issues of fragmentation.
  - All blocks will be added to list of available blocks when wipe (format) occurs.

- Programs are statically loaded but program information is read from a file (i.e. program id / priority, 
  adjustable using setp). Generally, the "object" files shouldn't be tampered with, however they are 
  safe to mv, cp, and cat. Programs are executed by calling run on the "object" file's path (e.g. run P0).

- There is enough validation to prevent the system from breaking (as far as I'm aware) but for most cases the 
  system does not provide error messages.
