#include "kernel.h"

pcb_t pcb[ PROCESS_LIMIT ], *current = NULL;
pcb_t* rq[ PROCESS_LIMIT ]; uint32_t rq_size;

mqueue mq[ MSGCHAN_LIMIT ]; 

ofile_t of[ OFT_LIMIT ]; uint32_t of_size; // open file table
inode_t ai[ AIT_LIMIT ]; uint32_t ai_size; // available inodes table

fs_t fs;      // filesystem metadata
uint32_t cwd; // current working directory inode

// =================
// === PROCESSES ===
// =================

uint32_t fork( ctx_t* ctx ) {
  pid_t p; // next available pid
  for (p = 0; p < PROCESS_LIMIT; p++) {                          // lowest available pid
    if (pcb[ p ].pst == TERMINATED) {                // process block space available
      pcb[ p ].pid                  = p;
      pcb[ p ].prt                  = current->pid;
      memcpy( &pcb[ p ].ctx, ctx, sizeof( ctx_t ));
      pcb[ p ].ctx.gpr[ 0 ]         = 0; // return value of child process
      pcb[ p ].pst                  = EXECUTING;
      pcb[ p ].defp = pcb[ p ].prio = 100;

      rq_add(p);

      return p; // return value as child pid for parent process
    }
  }
  return -1;    // error: no available memory
}

int getProgramEntry( char *path ) {
  int ino = path_to_ino( path, ROOT_DIR );

  if (ino != -1 ) {
    inode_t inode;
    readInode( &inode, ino );
    uint32_t block[ BLOCK_SIZE/4 ];
    getDataBlock( (uint8_t*)block, &inode, 0 );
    return block[ 0 ];
  }

  return -1; // failed
}

int exec( ctx_t* ctx, char *path ) {
  int entry = getProgramEntry( path );
  if (entry == -1) 
    return -1;

  int pid = current->pid;
  int pst = current->pst;

  memset( current, 0, sizeof( pcb_t ) );
  current->pid      = pid;
  current->pst      = pst;
  current->ctx.cpsr = 0x50;
  current->ctx.pc   = entry;
  current->ctx.sp   = ( uint32_t )( (uint32_t)(&tos_init) - current->pid * 0x00010000 );
  current->pst      = EXECUTING;
  current->defp = current->prio = 2;

  memcpy( ctx, &current->ctx, sizeof( ctx_t ) );

  return 0;
}

void kill( pid_t pid, sig_t sig ) {
  if (0 <= pid && pid <= PROCESS_LIMIT) {
    switch (sig) {
      case SIGKILL: { // enforced immediately
        pcb[ pid ].pst = TERMINATED;
        rq_rm( pid );
        break; 
      }
      case SIGWAIT: { // enforced immediately
        pcb[ pid ].pst = WAITING;
        break;
      }
      case SIGCONT: { // enforced immediately
        if (pcb[ pid ].pst == WAITING)
          pcb[ pid ].pst = EXECUTING;
        break;
      }
      case SIGPRI0: { // enforced immediately
        pcb[ pid ].defp = 0;
        pcb[ pid ].prio = 0;
        break;
      }
    }
  }
  else if (pid == 9 && sig == SIGKILL) {
    for (pid_t p = 1; p < PROCESS_LIMIT; p++) {
      pcb[ p ].pst = TERMINATED;
    }
  }
}

// ==================
// === SCHEDULING ===
// ==================

int cmp_pcb( const void* p1, const void* p2 ) {
  // Empty entries go at back of list
  if      ( (*((pcb_t**)(p1))) == NULL && (*((pcb_t**)(p2))) != NULL ) return  1;
  else if ( (*((pcb_t**)(p1))) != NULL && (*((pcb_t**)(p2))) == NULL ) return -1;
  else if ( (*((pcb_t**)(p1))) == NULL && (*((pcb_t**)(p2))) == NULL ) return  0;

  // Terminated processes go behind waiting processes
  if      ( (*((pcb_t**)(p1)))->pst == TERMINATED && (*((pcb_t**)(p2)))->pst != TERMINATED ) return  1;
  else if ( (*((pcb_t**)(p1)))->pst != TERMINATED && (*((pcb_t**)(p2)))->pst == TERMINATED ) return -1;

  // Waiting processes go behind live processes
  if      ( (*((pcb_t**)(p1)))->pst == WAITING && (*((pcb_t**)(p2)))->pst != WAITING ) return  1;
  else if ( (*((pcb_t**)(p1)))->pst != WAITING && (*((pcb_t**)(p2)))->pst == WAITING ) return -1;

  // Zero priority processes go behind priority processes
  if      ( (*((pcb_t**)(p1)))->prio == 0 && (*((pcb_t**)(p2)))->prio  > 0 ) return  1;
  else if ( (*((pcb_t**)(p1)))->prio  > 0 && (*((pcb_t**)(p2)))->prio == 0 ) return -1;

  // Lower priority goes behind higher priority
  if      ( (*((pcb_t**)(p1)))->defp < (*((pcb_t**)(p2)))->defp ) return  1;
  else if ( (*((pcb_t**)(p1)))->defp > (*((pcb_t**)(p2)))->defp ) return -1;

  // Equal priority live processes ordered by pid
  if      ( (*((pcb_t**)(p1)))->pid > (*((pcb_t**)(p2)))->pid ) return  1;
  else if ( (*((pcb_t**)(p1)))->pid < (*((pcb_t**)(p2)))->pid ) return -1;
  
  return 0;
}

// For standard round robin
void rq_rotate() {
  pcb_t* head = rq[ 0 ];
  int end = head == NULL ? rq_size : rq_size-1; // TODO: ???

  for (int i = 0; i < end; i++) {
    rq[ i ] = rq[ i+1 ];
  }  

  rq[ end ] = head;

  if (rq[ 0 ]->pst != EXECUTING && rq[ 0 ] != NULL)
    rq_rotate(); // skip past waiting processing
}

void rq_add( pid_t pid ) {
  if (rq_size < PROCESS_LIMIT) { 
    rq[ rq_size ] = &pcb[ pid ];
    rq_size++;
  }
  // TODO: error signal
}

void rq_rm( pid_t pid ) {
  // fill where process was
  for (int p = 0; p < rq_size; p++) {
    if (rq[ p ]->pid == pid) {

      for(int i = p; i < rq_size-1; i++) {
        rq[ i ] = rq[ i+1 ];
      }

      // Remove back of queue and decrease size
      rq[ rq_size-1 ] = NULL;
      rq_size--;

    }
  }
  // TODO: error finding pid
} 

void scheduler( ctx_t* ctx ) {
  qsort( rq, PROCESS_LIMIT, sizeof(pcb_t*), cmp_pcb );
  //rq_rotate();

  // There exists a live program in ready queue
  if (rq[ 0 ] != NULL) {
    // reset all priorities (should be in order by this point)
    if (rq[ 0 ]->prio == 0) {
      for (int i = 0; i < rq_size; i++) {
        rq[ i ]->prio = rq[ i ]->defp;
      }
    }

    // Current has not changed
    if (current == rq[ 0 ]) {
      current->prio--; 
    }
    // Current changed
    else {
      memcpy( &pcb[ current->pid ].ctx, ctx, sizeof( ctx_t ) );
      memcpy( ctx, &rq[ 0 ]->ctx, sizeof( ctx_t ) );
      current = rq[ 0 ];
    }
  }
}

// ======================
// === MESSAGE QUEUES ===
// ======================

int mq_open(int name) {
  // check to see if mqueue already open
  for (mqd_t i = 0; i < MSGCHAN_LIMIT; i++) {
    if (mq[ i ].msg_qname == name)
      return i;
  }

  // open new channel
  for (mqd_t i = 0; i < MSGCHAN_LIMIT; i++) {
    if (mq[ i ].msg_qname == 0) {
      mq[ i ].msg_qname = name;

      mq[ i ].msg_qnum = 0;
      mq[ i ].msg_qrec = 0;

      mq[ i ].msg_lspid = 0;
      mq[ i ].msg_lrpid = 0;

      return i;
    }
  }

  return -1; // no channels available
}

int mq_unlink(int m) {
  if (0 <= m && m <= MSGCHAN_LIMIT) { 
    mq[ m ].msg_qname = 0;
    return 0; 
  }
  return -1;
}

int mq_send(mqd_t mqd, uint8_t *msg_ptr, size_t msg_len) {
  mq[ mqd ].msg_lspid = current->pid;
  kill( current->pid, SIGWAIT );

  if (mq[ mqd ].msg_qnum == 0) { // space on mqueue
    mq[ mqd ].msg_qnum++;
    memcpy( mq[ mqd ].msg_qbuf, msg_ptr, msg_len );
    if (mq[ mqd ].msg_qrec == 1) {
      kill( mq[ mqd ].msg_lrpid, SIGCONT ); // wake receiver
      mq[ mqd ].msg_qrec = 0;
    }
    return 0;
  }

  return -1;
}

int mq_receive(mqd_t mqd, uint8_t *msg_ptr, size_t msg_len) {
  mq[ mqd ].msg_lrpid = current->pid;  
  
  // space on mqueue & last sender wasn't current receiver
  if (mq[ mqd ].msg_qnum > 0 && mq[ mqd ].msg_lspid != current->pid) { 
    mq[ mqd ].msg_qnum--;
    memcpy( msg_ptr, mq[ mqd ].msg_qbuf, msg_len );
    kill( mq[ mqd ].msg_lspid, SIGCONT ); // wake sender
    return msg_len; // just some success value (it doesn't matter so long as it's positive)
  }

  mq[ mqd ].msg_qrec = 1; // tells queue a process is waiting for data
  kill( current->pid, SIGWAIT );

  return -1;
}

// ==================
// === FILESYSTEM ===
// ==================

void createObjFiles() {
  int FILE;

  FILE = open( "P0", O_CREAT );
  fwrite( FILE, (uint8_t*)&entry_P0, 4 );
  close( FILE );

  FILE = open( "P1", O_CREAT );
  fwrite( FILE, (uint8_t*)&entry_P1, 4 );
  close( FILE );

  FILE = open( "P2", O_CREAT );
  fwrite( FILE, (uint8_t*)&entry_P2, 4 );
  close( FILE );

  FILE = open( "het", O_CREAT );
  fwrite( FILE, (uint8_t*)&entry_het, 4 );
  close( FILE );

  FILE = open( "ping", O_CREAT );
  fwrite( FILE, (uint8_t*)&entry_ping, 4 );
  close( FILE );

  FILE = open( "pong", O_CREAT );
  fwrite( FILE, (uint8_t*)&entry_pong, 4 );
  close( FILE );

  FILE = open( "MESSAGE", O_CREAT );
  fwrite( FILE, (uint8_t*)&entry_MESSAGE, 4 );
  close( FILE );
  
  FILE = open( "CABB", O_CREAT );
  fwrite( FILE, (uint8_t*)&entry_CABB, 4 );
  close( FILE );
}

// === BLOCK ALLOCATION FUNCTIONS ===

daddr32_t balloc() { // block allocation
	if (fs.fs_fdbhead > 0) {
		fs.fs_fdbhead--;
    disk_wr( fs.fs_sblkno, (uint8_t*)(&fs), sizeof( fs_t ) ); // update sb
		return fs.fs_fdb[ fs.fs_fdbhead+1 ];
	}
	
	// block addr 1 is addr of superblock - indicates no space remaining
	if (fs.fs_fdb[ fs.fs_fdbhead ] != 1) {
		daddr32_t addr = fs.fs_fdb[ fs.fs_fdbhead ];
    
    uint8_t block[ BLOCK_SIZE ];
    disk_rd( addr, block, BLOCK_SIZE );

    memcpy( fs.fs_fdb, block, 64 * sizeof( daddr32_t ) );
    fs.fs_fdbhead = 63;

    disk_wr( fs.fs_sblkno, (uint8_t*)(&fs), sizeof( fs_t ) ); // update sb

		return addr;
	}

	return -1; // no more available blocks
}

int bfree( daddr32_t a ) { // block free
	if (fs.fs_fdbhead < 63) {
		fs.fs_fdb[ ++fs.fs_fdbhead ] = a;
    disk_wr( fs.fs_sblkno, (uint8_t*)(&fs), sizeof( fs_t ) ); // update sb
		return 0; // success
	}

	disk_wr( a, (uint8_t*)fs.fs_fdb, 64 * sizeof( daddr32_t ) );
	fs.fs_fdb[ 0 ] = a;
	fs.fs_fdbhead  = 0;
  disk_wr( fs.fs_sblkno, (uint8_t*)(&fs), sizeof( fs_t ) ); // update sb

	return 1; // sucess
}

// === SUPERBLOCK FUNCTIONS ===

void wipe() {
  // superblock
  fs.fs_sblkno  = 1;
  fs.fs_size    = 2048;
  fs.fs_iblkno  = 2;
  fs.fs_isize   = 64*8;
  fs.fs_dblkno  = 66;
  fs.fs_dsize   = 1982;
  fs.fs_fdbhead = (fs.fs_dsize%64)-1;

  daddr32_t fdb[ 64 ];

  for (int i = 0; i < 30; i++) {
    if (i == 29) 
      fdb[ 0 ] = fs.fs_sblkno;
    else
      fdb[ 0 ] = i + 1 + fs.fs_dblkno;

    for (int j = 0; j < 63; j++)
      fdb[ j+1 ] = 63*i + j + (fs.fs_dblkno + 30);

    disk_wr( i + fs.fs_dblkno, (uint8_t*)fdb, 64 * sizeof( daddr32_t ) );
  }

  fs.fs_fdb[ 0 ] = fs.fs_dblkno;
  for (int i = 1; i < 62; i++) {
    fs.fs_fdb[ i ] = i + 1986; // 66 + 30 * 64
  }

  disk_wr( fs.fs_sblkno, (uint8_t*)(&fs), sizeof( fs_t ) );

  icommon_t ic[ 8 ]; 
  for (int i = 0; i < 64; i++) {
    disk_rd( i + fs.fs_iblkno, (uint8_t*)ic, 8 * sizeof( icommon_t ) );
    for (int j = 0; j < 8; j++) {
      if (i == 0 && j == ROOT_DIR) {
        ic[ j ].ic_mode = IFDIR;
        ic[ j ].ic_size = 32;
        ic[ j ].ic_db[ 0 ] = balloc(); // note: this updates the disk

        dir_t dir[ 16 ];
        dir[ 0 ].d_ino = ROOT_DIR;
        dir[ 0 ].d_namlen = 1;
        dir[ 0 ].d_name[ 0 ] = '.';

        disk_wr( ic[ j ].ic_db[ 0 ], (uint8_t *)dir, 16 * sizeof( dir_t ) );        
      }
      else {
        ic[ j ].ic_mode = IFZERO;
        ic[ j ].ic_size = 0;
      }
    }
    disk_wr( i + fs.fs_iblkno, (uint8_t *)ic, 8 * sizeof( icommon_t ) );
  }  

  createObjFiles();

  return;
}

// === DATA BLOCK FUNCTIONS ===

int getDataBlockAddr( const inode_t *inode, uint32_t byte ) { 
  int blk = byte / BLOCK_SIZE;

  // Direct block
  if (blk < NDADDR) {
    return (int)inode->i_ic.ic_db[ blk ];
  }
  // Indirect blocks
  else {
    const int ls = BLOCK_SIZE/4;
    uint8_t block[ BLOCK_SIZE ];

    if      (blk < ls + NDADDR) {
      blk -= NDADDR; 
      disk_rd( inode->i_ic.ic_ib[ 0 ], block, BLOCK_SIZE );
      return ((uint32_t*)block)[ blk ];
    }
    else if (blk < ls*ls + ls + NDADDR) {
      blk -= ls + NDADDR;
      disk_rd( inode->i_ic.ic_ib[ 1 ], block, BLOCK_SIZE );
      disk_rd( ((uint32_t*)block)[ blk/ls ], block, BLOCK_SIZE );
      return ((uint32_t*)block)[ blk%ls ];
    }
    else if (blk < ls*ls*ls + ls*ls + ls + NDADDR) {
      blk -= ls*ls + ls + NDADDR;
      disk_rd( inode->i_ic.ic_ib[ 2 ], block, BLOCK_SIZE );
      disk_rd( ((uint32_t*)block)[ blk/(ls*ls) ], block, BLOCK_SIZE );
      disk_rd( ((uint32_t*)block)[ (blk%(ls*ls))/ls ], block, BLOCK_SIZE );
      return ((uint32_t*)block)[ blk%ls ];
    }
  } 

  return -1;
}

int allocateDataBlockAddr( inode_t *inode, uint32_t byte ) { 
  int blk = byte / BLOCK_SIZE;

  int addr = balloc();
  if (addr == -1)
    return -1;

  // Direct block
  if (blk < NDADDR) {
    inode->i_ic.ic_db[ blk ] = addr;
    writeInode( inode );
  }
  // Indirect blocks
  else {
    const int ls = BLOCK_SIZE/4;
    uint8_t block[ BLOCK_SIZE ];

    int taddr;

    if      (blk < ls + NDADDR) {
      blk -= NDADDR; 

      if (blk == 0) {
        taddr = balloc();
        if (taddr == -1)
          return -1;
        inode->i_ic.ic_ib[ 0 ] = taddr;
        writeInode( inode );
      }
      else {  
        taddr = inode->i_ic.ic_ib[ 0 ];
      }

      disk_rd( taddr, block, BLOCK_SIZE );

      ((uint32_t*)block)[ blk ] = addr;
      disk_wr( taddr, block, BLOCK_SIZE );
    }
    else if (blk < ls*ls + ls + NDADDR) {
      blk -= ls + NDADDR;

      if (blk == 0) {
        taddr = balloc(); if (taddr == -1) return -1;
        inode->i_ic.ic_ib[ 1 ] = taddr;
        writeInode( inode );
      }
      else {  
        taddr = inode->i_ic.ic_ib[ 1 ];
      }

      disk_rd( taddr, block, BLOCK_SIZE );

      if (blk % ls == 0) {
        taddr = balloc(); if (taddr == -1) return -1;
        ((uint32_t*)block)[ blk/ls ] = taddr;
        disk_wr( taddr, block, BLOCK_SIZE );
      }
      else {
        taddr = ((uint32_t*)block)[ blk/ls ];
      }
  
      disk_rd( taddr, block, BLOCK_SIZE );

      ((uint32_t*)block)[ blk%ls ] = addr;
      disk_wr( taddr, block, BLOCK_SIZE );
    }
    else if (blk < ls*ls*ls + ls*ls + ls + NDADDR) {
      blk -= ls*ls + ls + NDADDR;

      if (blk == 0) {
        taddr = balloc(); if (taddr == -1) return -1;
        inode->i_ic.ic_ib[ 2 ] = taddr;
        writeInode( inode );
      }
      else {  
        taddr = inode->i_ic.ic_ib[ 2 ];
      }

      disk_rd( taddr, block, BLOCK_SIZE );

      if (blk % (ls*ls) == 0) {
        taddr = balloc(); if (taddr == -1) return -1;
        ((uint32_t*)block)[ blk/(ls*ls) ] = taddr;
        disk_wr( taddr, block, BLOCK_SIZE );
      }
      else {
        taddr = ((uint32_t*)block)[ blk/(ls*ls) ];
      }
  
      disk_rd( taddr, block, BLOCK_SIZE );

      blk %= ls*ls;

      if (blk % ls == 0) {
        taddr = balloc(); if (taddr == -1) return -1;
        ((uint32_t*)block)[ blk/ls ] = taddr;
        disk_wr( taddr, block, BLOCK_SIZE );
      }
      else {
        taddr = ((uint32_t*)block)[ blk/ls ];
      }
  
      disk_rd( taddr, block, BLOCK_SIZE );

      ((uint32_t*)block)[ blk%ls ] = addr;
      disk_wr( taddr, block, BLOCK_SIZE );
    }
  } 

  return addr;
}

int allocateDataBlocks( inode_t *inode, uint32_t bytes ) {
  const int b = inode->i_ic.ic_size / BLOCK_SIZE;
  const int n = (inode->i_ic.ic_size + bytes) / BLOCK_SIZE;

  for (int i = b+1; i <= n; i++) { 
    if (allocateDataBlockAddr( inode, i * BLOCK_SIZE ) == -1) 
      return -1;
  }

  return inode->i_ic.ic_size += bytes;
}

int freeDataBlocks( inode_t *inode ) {
  for (int i = 0; i < inode->i_ic.ic_size; i += BLOCK_SIZE) { 
    // Removing direct or leaf blocks    
    bfree( getDataBlockAddr( inode, i ) );

    // Removing indirect blocks
    int blk = i / BLOCK_SIZE;
    if (blk >= NDADDR) {
      const int ls = BLOCK_SIZE/4;
      uint8_t block[ BLOCK_SIZE ];

      if      (blk < ls + NDADDR) {
        blk -= NDADDR;
        if (blk == 0) {
          bfree( inode->i_ic.ic_ib[ 0 ] );
        }
      }
      else if (blk < ls*ls + ls + NDADDR) {
        blk -= ls + NDADDR;

        disk_rd( inode->i_ic.ic_ib[ 1 ], block, BLOCK_SIZE );

        if (blk % ls == 0) {
          bfree( ((uint32_t*)block)[ blk/ls ] );
        }

        if (blk == 0) {
          bfree( inode->i_ic.ic_ib[ 1 ] );
        }
      }
      else if (blk < ls*ls*ls + ls*ls + ls + NDADDR) {
        blk -= ls*ls + ls + NDADDR;

        disk_rd( inode->i_ic.ic_ib[ 2 ], block, BLOCK_SIZE );
        disk_rd( ((uint32_t*)block)[ blk / (ls*ls) ], block, BLOCK_SIZE );

        if ((blk % (ls*ls)) % ls == 0) {
          bfree( ((uint32_t*)block)[ (blk % (ls*ls))/ls ] );
        }

        disk_rd( inode->i_ic.ic_ib[ 2 ], block, BLOCK_SIZE );

        if (blk % (ls*ls) == 0) {
          bfree( ((uint32_t*)block)[ blk/(ls*ls) ] );
        }

        if (blk == 0) {
          bfree( inode->i_ic.ic_ib[ 2 ] );
        }
      }
    }
  }

  return 0;
}

int getDataBlock( uint8_t *block, const inode_t *inode, uint32_t byte ) { 
  int addr = getDataBlockAddr( inode, byte );
  disk_rd( addr, block, BLOCK_SIZE );
  return addr;
}

// === INODE FUNCTIONS ===

inode_t *copyInode( inode_t *copy, inode_t *inode ) {
  if (getFreeInode( copy ) == NULL)
    return NULL;
  allocateDataBlocks( copy, (inode->i_ic.ic_size / BLOCK_SIZE) + 1 );

  uint8_t block[ BLOCK_SIZE ];

  for (int i = 0; i < inode->i_ic.ic_size; i += BLOCK_SIZE) { 
    getDataBlock( block, inode, i );
    int n = inode->i_ic.ic_size - i > BLOCK_SIZE ? BLOCK_SIZE : inode->i_ic.ic_size - i;
    disk_wr( getDataBlockAddr( copy, i ), block, n );
  }

  return copy;
}

inode_t *readInode( inode_t *in, int ino ) {
	// TODO: validation in case ino is invalid

	int q = ino / 8;
	int r = ino % 8;
	
	icommon_t ic[ 8 ];
	disk_rd( fs.fs_iblkno + q, (uint8_t*)ic, 8 * sizeof( icommon_t ) );

	in->i_number = ino;
	in->i_ic		 = ic[ r ];

	return in;
}

inode_t *writeInode( inode_t *inode ) {
  const int q = inode->i_number / 8;
	const int r = inode->i_number % 8;
	
	icommon_t ic[ 8 ];
	disk_rd( fs.fs_iblkno + q, (uint8_t*)ic, BLOCK_SIZE );

  ic[ r ] = inode->i_ic;
  disk_wr( fs.fs_iblkno + q, (uint8_t*)ic, BLOCK_SIZE );
	
	return inode;
} 

inode_t * getFreeInode( inode_t *in ) {
	icommon_t ic[ 8 ];
	for (int i = 0; i < fs.fs_isize/8; i++) {
		disk_rd( fs.fs_iblkno + i, (uint8_t*)ic, 8 * sizeof( icommon_t ) );
		for (int j = 0; j < 8; j++) {
			if (ic[ j ].ic_mode == IFZERO) { // inode unused
				in->i_number     = 8*i + j;
        in->i_ic.ic_mode = IFREG;
        in->i_ic.ic_size = 0;
				return in;
			}
		}
	}

	return NULL;
}

dir_t *getLastDir( dir_t *d, const inode_t *inode ) {
  const int r = ((inode->i_ic.ic_size / 32) - 1) % 16; // offset of last dir, assumes directory non-empty
  
  dir_t dir[ 16 ];
  getDataBlock( (uint8_t*)dir, inode, inode->i_ic.ic_size - 32);

  memcpy( d, &dir[ r ], sizeof( dir_t ) );
  return d;
}

int remove_inode( dir_t *child, inode_t *parent, const char *name ) {
	const int bytes = parent->i_ic.ic_size; // number of bytes
	const int dirs  = bytes / 32;       // number of directories

  if (dirs > 0) {
	  const int blks  = ((dirs-1) / 16) + 1;	// number of blocks
	  const int r		  = dirs % 16;			      // offset in last block
	
	  dir_t dir[ 16 ];
    daddr32_t addr;

    // TODO: check file exists in ai, and prevent removal if so : "error, do not have permission to remove."

	  for (int i = 0; i < blks-1; i++) {
      addr = getDataBlock( (uint8_t*)dir, parent, BLOCK_SIZE * i );
		  for (int j = 0; j < 16; j++) {
			  if (strncmp( name, dir[j].d_name, dir[j].d_namlen >= strlen(name) ? dir[j].d_namlen : strlen(name) ) == 0) {
          memcpy( child, &dir[ j ], sizeof( dir_t ) );        

          getLastDir( &dir[ j ], parent ); 
          disk_wr( addr, (uint8_t*)dir, BLOCK_SIZE );

          parent->i_ic.ic_size -= 32;
          writeInode( parent );

				  return child->d_ino;
        }
		  }
	  }

    addr = getDataBlock( (uint8_t*)dir, parent, BLOCK_SIZE * (blks-1) );
	  for (int j = 0; j < r; j++) {
		  if (strncmp( name, dir[j].d_name, dir[j].d_namlen >= strlen(name) ? dir[j].d_namlen : strlen(name) ) == 0) {
        memcpy( child, &dir[ j ], sizeof( dir_t ) );       

        getLastDir( &dir[ j ], parent ); 
        disk_wr( addr, (uint8_t*)dir, BLOCK_SIZE );
        
        parent->i_ic.ic_size -= 32;
        writeInode( parent );  

			  return child->d_ino;
      }
	  }
  }

	return -1; // does not exist in directory
}

int name_to_ino( const char *name, const inode_t *in ) {
	const int bytes = in->i_ic.ic_size; // number of bytes
	const int dirs  = bytes / 32;       // number of directories

  if (dirs > 0) {
	  const int blks  = ((dirs-1) / 16) + 1;	// number of blocks
	  const int r		  = dirs % 16;			      // offset in last block
	
	  dir_t dir[ 16 ];

	  for (int i = 0; i < blks-1; i++) {
      getDataBlock( (uint8_t*)dir, in, BLOCK_SIZE * i );
		  for (int j = 0; j < 16; j++) {
			  if (strncmp( name, dir[j].d_name, dir[j].d_namlen >= strlen(name) ? dir[j].d_namlen : strlen(name) ) == 0)
				  return dir[j].d_ino;
		  }
	  }

    getDataBlock( (uint8_t*)dir, in, BLOCK_SIZE * (blks-1) );
	  for (int j = 0; j < r; j++) {
		  if (strncmp( name, dir[j].d_name, dir[j].d_namlen >= strlen(name) ? dir[j].d_namlen : strlen(name) ) == 0)
			  return dir[j].d_ino;
	  }
  }

	return -1; // does not exist in directory
}

int path_to_ino( char *path, const int dir ) {
  inode_t  inode;
  int 		 ino = dir;
  char 	  *tok;

  for (tok = strtok( path, "/" ); 
	     tok != NULL && ino != -1; 
	     tok = strtok( NULL, "/" ) ) 
  {
    readInode( &inode, ino );
    ino = name_to_ino( tok, &inode );
  }

  return ino;
}

// this version also deals with creating new files
int path_to_ino2( char *path, const int dir ) {
	inode_t  inode;
	int 		 ino0,  ino = dir;
	char 	  *tok0, *tok;

	for (tok0 = tok = strtok( (char*)path, "/" ); 
			 tok != NULL && ino != -1; 
			 tok = strtok( NULL, "/" ) ) 
  {
		readInode( &inode, ino ); 			     // first iteration will give us root directory

		ino0 = ino;
		ino  = name_to_ino( tok, &inode ); // if ino is valid, then there must exist a corresponding valid inode

		tok0 = tok;                        // tok0 will be name of file upon termination, if path is valid
	} 
	
	if (tok != NULL && ino == -1) {
		return -1; // path doesn't exist
	}
	else if (tok == NULL && ino == -1) {
		// create new inode and return corresponding ino
		if (getFreeInode( &inode ) == NULL) // check assigned inode correctly
			return -2;

    // give inode its first data block
    inode.i_ic.ic_db[ 0 ] = balloc();

    // write new inode to disk
    writeInode( &inode );

		// add new inode to parent directory (assumes new inode is not a directory)
		inode_t parent;
		readInode( &parent, ino0 );
		addInodeToDirectory( &parent, inode.i_number, tok0 ); // TODO: parent will get written to 
                                                          // - thus assumes parent is not in AIT (since is a dir.)

		return inode.i_number;
	}

  return ino; 
}

// === DIRECTORIES FUNCTIONS ===

void addInodeToDirectory( inode_t* par, uint32_t ino, const char *name ) {
  const int r	= (par->i_ic.ic_size / 32) % 16; // offset in block
  if (r == 0 && par->i_ic.ic_size > 0) {
    allocateDataBlockAddr( par, par->i_ic.ic_size ); // TODO: validate
  }
  
  dir_t dir[ 16 ];
  daddr32_t addr = getDataBlock( (uint8_t*)dir, par, par->i_ic.ic_size );

  dir[ r ].d_ino    = ino;
  dir[ r ].d_namlen = strlen( name ); // TODO: pass in string length (it's safer that way)
  strncpy( dir[ r ].d_name, name, strlen( name ) ); 
	disk_wr( addr, (uint8_t*)dir, BLOCK_SIZE );

  // WARNING: this may need to go above getDataBlock?
  par->i_ic.ic_size += 32; // add new directory
  writeInode( par );       // update inode status on disk
}

// === TABLE FUNCTIONS ===

int getFD() {
  // find valid file descriptor table entry
  for (int fd = 0; fd < FDT_LIMIT; fd++) {
    if (current->fd[ fd ] == NULL)
      return fd;
  }

  return -1; // table full 
}

ofile_t * getOFT() {
  // find valid OFT entry and grab the pointer to it
	for (int i = 0; i < OFT_LIMIT; i++) {
		if (of[ i ].o_inptr == NULL)
			return &of[ i ];
	}

  return NULL; // this probably can't happen
}

inode_t * getAIT( int ino ) {
  // inode already in AIT
	for (int i = 0; i < AIT_LIMIT; i++) {
		if (ai[ i ].i_number == ino && ai[ i ].i_ic.ic_mode != IFZERO)
			return &ai[ i ];
	}	

  // add inode to empty AIT entry (2nd pass)
  for (int i = 0; i < AIT_LIMIT; i++) {
    if (ai[ i ].i_ic.ic_mode == IFZERO)
      return readInode( &ai[ i ], ino );
	}	

	return NULL; // table full
}

// === POSIX FUNCTIONS ===

int open( char *path, int oflag) {
  int fd = getFD();                          if (fd == -1)      return -1;
	int ino = 0;
  if (oflag == O_CREAT) ino = path_to_ino2( path, ROOT_DIR );  
  else                  ino = path_to_ino( path, ROOT_DIR );
  if (ino < 0) return -1;
  inode_t *inode = getAIT( ino );            if (inode == NULL) return -1; 
  ofile_t *ofile = getOFT();                 if (ofile == NULL) return -1;

  // increment number of linked OFT entries to the inode
  inode->i_links++;	

	// link FDT entry to OFT entry
	current->fd[ fd ] = ofile;

	// link OFT entry to AIT entry
	ofile->o_inptr = inode;	

  return fd;
}

int close( const int fd ) {
  // validation
  if      (fd < 0 || fd >= FDT_LIMIT) return -1;
  else if (current->fd[ fd ] == NULL) return -1;
  
  // decrement i_link THEN check it is 0 
  if (--current->fd[ fd ]->o_inptr->i_links == 0) {
    // no longer need to keep inode in memory
    writeInode( current->fd[ fd ]->o_inptr );
    current->fd[ fd ]->o_inptr->i_ic.ic_mode = IFZERO;
  }

  // clear OFT entry
  current->fd[ fd ]->o_inptr = NULL;
  current->fd[ fd ]->o_head  = 0;

  // clear FDT entry
  current->fd[ fd ] = NULL;

  return 0;
}

int fwrite( const int fd, const uint8_t *data, const int n ) {
  // validation
  if      (fd < 0 || fd >= FDT_LIMIT) return -1;
  else if (current->fd[ fd ] == NULL) return -1;

  ofile_t *ofile = current->fd[ fd ];
  inode_t *inode = ofile->o_inptr;

  // if necessary, allocate new blocks to file
  if (ofile->o_head + n > inode->i_ic.ic_size)
    allocateDataBlocks( inode, ofile->o_head + n - inode->i_ic.ic_size );

  uint8_t buf[ BLOCK_SIZE ]; // block buffer
  uint32_t addr = getDataBlock( buf, inode, ofile->o_head ); // maintain current block addr

  // write each byte to disk
  for (int i = 0; i < n; i++) {
    buf[ (ofile->o_head + i) % BLOCK_SIZE ] = data[ i ];
  
    // check not last byte and next addr goes over block boundary
    if (i != n-1 && (ofile->o_head + i + 1) % BLOCK_SIZE == 0) {
      disk_wr( addr, buf, BLOCK_SIZE );                         // store
      addr = getDataBlock( buf, inode, ofile->o_head + i + 1 ); // load                       
    }
  }

  disk_wr( addr, buf, BLOCK_SIZE );
  ofile->o_head += n;

  return 0;
}

int fread( const int fd, uint8_t *data, const int n ) {
  // validation
  if      (fd < 0 || fd >= FDT_LIMIT) return -1;
  else if (current->fd[ fd ] == NULL) return -1;

  ofile_t *ofile = current->fd[ fd ];
  inode_t *inode = ofile->o_inptr;

  // Not enough data in file to be read
  if (ofile->o_head + n > inode->i_ic.ic_size)
    return -1;

  uint8_t buf[ BLOCK_SIZE ]; // block buffer
  getDataBlock( buf, inode, ofile->o_head );

  // read each byte to from
  for (int i = 0; i < n; i++) {
    data[ i ] = buf[ (ofile->o_head + i) % BLOCK_SIZE ];
  
    // check not last byte and next addr goes over block boundary
    if (i != n-1 && (ofile->o_head + i + 1) % BLOCK_SIZE == 0)
      getDataBlock( buf, inode, (ofile->o_head + i + 1) );                
  }

  ofile->o_head += n;

  return 0;
}

int lseek( const int fd, uint32_t offset, const int whence ) {
  // validate file descriptor
  if (fd < 0 || fd >= FDT_LIMIT) return -1;
  if (current->fd[ fd ] == NULL) return -1;

  switch (whence) {
    case SEEK_SET : {
      current->fd[ fd ]->o_head = offset;
      break;
    }
    case SEEK_CUR : {
      current->fd[ fd ]->o_head += offset;
      break;
    }
    case SEEK_END : {
      current->fd[ fd ]->o_head = current->fd[ fd ]->o_inptr->i_ic.ic_size + offset;
      break;
    }
    default: return -1;
  }
  return current->fd[ fd ]->o_head;
}

int tell( const int fd ) {
  // validate file descriptor
  if (fd < 0 || fd >= FDT_LIMIT) return -1;
  if (current->fd[ fd ] == NULL) return -1;

  return current->fd[ fd ]->o_head;
}

// =====================================
// === INTERRUPTS / SUPERVISOR CALLS ===
// =====================================

void kernel_handler_rst( ctx_t* ctx ) { 
  memset( &pcb[ 0 ], 0, sizeof( pcb_t ) );
  pcb[ 0 ].pid      = 0;
  pcb[ 0 ].prt      = 0;
  pcb[ 0 ].ctx.cpsr = 0x50; // processor switched into USR mode, w/ IRQ interrupts enabled
  pcb[ 0 ].ctx.pc   = ( uint32_t )( entry_init );
  pcb[ 0 ].ctx.sp   = ( uint32_t )(  &tos_init );
  pcb[ 0 ].pst      = EXECUTING;
  pcb[ 0 ].defp = pcb[ 0 ].prio = 1;

  current = &pcb[ 0 ];
  memcpy( ctx, &current->ctx, sizeof( ctx_t ) );

  rq_size = 0;
  rq_add( 0 );

	// superblock defined at block address 1
	disk_rd( 1, (uint8_t*)(&fs), sizeof( fs_t ) ); // TODO: investigate padding

  // set up default working directory
  cwd       = ROOT_DIR;

  UART0->IMSC           |= 0x00000010; // enable UART    (Rx) interrupt
  UART0->CR              = 0x00000301; // enable UART (Tx+Rx)

  TIMER0->Timer1Load     = 0x00001000; // select period = 2^20 ticks ~= 1 sec
  TIMER0->Timer1Ctrl     = 0x00000002; // select 32-bit   timer
  TIMER0->Timer1Ctrl    |= 0x00000040; // select periodic timer
  TIMER0->Timer1Ctrl    |= 0x00000020; // enable          timer interrupt
  TIMER0->Timer1Ctrl    |= 0x00000080; // enable          timer

  GICC0->PMR             = 0x000000F0; // unmask all            interrupts
  GICD0->ISENABLER[ 1 ] |= 0x00000010; // enable timer          interrupt
  GICD0->ISENABLER[ 1 ] |= 0x00001000; // enable UART    (Rx) interrupt
  GICC0->CTLR            = 0x00000001; // enable GIC interface
  GICD0->CTLR            = 0x00000001; // enable GIC distributor

  irq_enable();

  return;
}

void kernel_handler_irq( ctx_t* ctx ) {
  uint32_t id = GICC0->IAR; //read  the interrupt identifier so we know the source

  // handle the interrupt, then clear (or reset) the source.
  if( id == GIC_SOURCE_TIMER0 ) {
    scheduler( ctx );
    TIMER0->Timer1IntClr = 0x01;
  }
  else if( id == GIC_SOURCE_UART0 ) {
    if (pcb[ 0 ].defp == 0) {
      pcb[ 0 ].defp = 1000;
      pcb[ 0 ].prio = 1000;
      scheduler( ctx );
    }
    UART0->ICR = 0x10;
  }

  GICC0->EOIR = id; // write the interrupt identifier to signal we're done

  return;
}

void kernel_handler_svc( ctx_t* ctx, uint32_t id ) { 
  switch( id ) {
    case 0x00 : { // yield()
      scheduler( ctx );
      break;
    }
    case 0x01 : { // write( fd, x, n )
      int   fd = ( int   )( ctx->gpr[ 0 ] );       

      if (fd == STDIO) {
        char*  x = ( char* )( ctx->gpr[ 1 ] );  
        int    n = ( int   )( ctx->gpr[ 2 ] );

        for( int i = 0; i < n; i++ ) {
          PL011_putc( UART0, *x++ );
        }
        
        ctx->gpr[ 0 ] = n;
        break;
      }
      ctx->gpr[ 0 ] = fwrite( fd, (uint8_t*)ctx->gpr[ 1 ], ctx->gpr[ 2 ]);
      break;
    }
    case 0x02 : { // read( fd, x, n ) - non-silent
      int   fd = ( int   )( ctx->gpr[ 0 ] );  
      
      if (fd == STDIO) {
        char*  x = ( char* )( ctx->gpr[ 1 ] );  
        int    n = ( int   )( ctx->gpr[ 2 ] );

        for( int i = 0; i < n; i++ ) {
          x[i] = PL011_getc( UART0 );
          if (x[i] == 13) { // ASCII carriage return
            PL011_putc( UART0, '\n' );
            break; 
          }
          PL011_putc( UART0, x[i] );
        }

        ctx->gpr[ 0 ] = n;
        break;
      }
      ctx->gpr[ 0 ] = fread( fd, (uint8_t*)ctx->gpr[ 1 ], ctx->gpr[ 2 ]);
      break;
    }
    case 0x03 : { // fork
      ctx->gpr[ 0 ] = fork( ctx );
      break;
    }
    case 0x04 : { // exec
      ctx->gpr[ 0 ] = exec( ctx, (char*)ctx->gpr[ 0 ] );
      break;
    }
    case 0x05 : { // lseek
      ctx->gpr[ 0 ] = lseek( ctx->gpr[ 0 ], ctx->gpr[ 1 ], ctx->gpr[ 2 ] );
      break;
    }
    case 0x06 : { // kill
      kill( ctx->gpr[ 0 ], ctx->gpr[ 1 ] );
      break;
    }
    case 0x07 : { // raise
      kill( current->pid, ctx->gpr[ 0 ] );
      scheduler( ctx ); // for now - raise immediately invokes scheduler
      break;
    }
    case 0x08 : { // mqueue open
      ctx->gpr[ 0 ] = mq_open( ctx->gpr[ 0 ] );
      break;
    }
    case 0x09 : { // channel send
      ctx->gpr[ 0 ] = mq_send( ctx->gpr[ 0 ], (uint8_t*)ctx->gpr[ 1 ], (size_t)ctx->gpr[ 2 ] );
      if (ctx->gpr[ 0 ] == -1)
        scheduler( ctx );
      break;
    }
    case 0x0a : { // channel receive
      ctx->gpr[ 0 ] = mq_receive( ctx->gpr[ 0 ], (uint8_t*)ctx->gpr[ 1 ], (size_t)ctx->gpr[ 2 ] );
      if (ctx->gpr[ 0 ] == -1)
        scheduler( ctx );
      break;
    }
    case 0x0b : { // reformat
      wipe();
      break;
    }
    case 0x0c : { // open file
      char* path = ( char* )( ctx->gpr[ 0 ] );  
      ctx->gpr[ 0 ] = open( path, ctx->gpr[ 1 ] );
      break;
    }
    case 0x0d : { // close file
      ctx->gpr[ 0 ] = close( ctx->gpr[ 0 ] );
      break;
    }
    case 0x0e : { // pwd
      PL011_putc( UART0, '/' );
      break;
    }
    case 0x0f : { // ls
      inode_t inode;
      readInode( &inode, cwd );
      
      const int dirs = inode.i_ic.ic_size / 32;
      const int blks = ((dirs-1) / 16) + 1;	// number of blocks
	    const int r		 = dirs % 16;			      // offset in last block
	
	    dir_t dir[ 16 ];

	    for (int i = 0; i < blks-1; i++) {
		    disk_rd( inode.i_ic.ic_db[ i ], (uint8_t*)dir, 16 * sizeof( dir_t ) );
		    for (int j = 0; j < 16; j++) {
			    for( int k = 0; k < dir[ j ].d_namlen; k++ )
            PL011_putc( UART0, dir[ j ].d_name[ k ] );
          PL011_putc( UART0, '\n' );
		    }
	    }

	    disk_rd( inode.i_ic.ic_db[ blks-1 ], (uint8_t*)dir, 16 * sizeof( dir_t ) );
	    for (int j = 0; j < r; j++) {
		    for( int k = 0; k < dir[ j ].d_namlen; k++ )
          PL011_putc( UART0, dir[ j ].d_name[ k ] );
        PL011_putc( UART0, '\n' );
	    }

      break;
    }
    case 0x10 : { // mkdir
      inode_t child;
      if ( getFreeInode( &child ) == NULL ) {
        ctx->gpr[ 0 ] = -1; // failed        
        break;
      }

      inode_t parent;
      readInode( &parent, cwd );

      /* TODO:
       *   validate name: check parent doesn't already contain directory entry of same name.
       */

      child.i_ic.ic_mode = IFDIR;
      child.i_ic.ic_size = 64;
      child.i_ic.ic_db[ 0 ] = balloc(); // note: this updates the disk

      dir_t dir[ 16 ];

      dir[ 0 ].d_ino = child.i_number;
      dir[ 0 ].d_namlen = 1;
      dir[ 0 ].d_name[ 0 ] = '.';

      dir[ 1 ].d_ino = parent.i_number; // aka cwd
      dir[ 1 ].d_namlen = 2;
      dir[ 1 ].d_name[ 0 ] = '.'; dir[ 1 ].d_name[ 1 ] = '.';

      disk_wr( child.i_ic.ic_db[ 0 ], (uint8_t *)dir, BLOCK_SIZE );
      writeInode( &child );
      addInodeToDirectory( &parent, child.i_number, (char *)ctx->gpr[ 0 ] );  

      ctx->gpr[ 0 ] = 0; // success

      break;
    }
    case 0x11 : { // cd
      inode_t  inode;
	    int 		 ino = cwd;
	    char 	  *tok;

      for (tok = strtok( (char*)ctx->gpr[ 0 ], "/" ); 
			     tok != NULL && ino != -1; 
			     tok = strtok( NULL, "/" ) ) 
      {
		    readInode( &inode, ino );
		    ino  = name_to_ino( tok, &inode );
	    }

      if (ino == -1) {
        ctx->gpr[ 0 ] = -1; // failure
        break;
      }

      cwd = ino;

      ctx->gpr[ 0 ] = 0; // success

      break;
    }
    case 0x12 : { // rm
      if ( ((char*)ctx->gpr[ 0 ])[0] == '.' ) {
        ctx->gpr[ 0 ] = -1;
        break;
      }

      inode_t inode;
      dir_t dir;
      readInode( &inode, cwd );
      if (remove_inode( &dir, &inode, (char*)ctx->gpr[ 0 ] ) != -1) { 
        readInode( &inode, dir.d_ino );
        freeDataBlocks( &inode );
        inode.i_ic.ic_mode = IFZERO;
        inode.i_ic.ic_size = 0;
        writeInode( &inode );  
      }

      break;
    }
    case 0x13 : { // mv   
      if ( ((char*)ctx->gpr[ 0 ])[0] == '.' ) {
        ctx->gpr[ 0 ] = -1;
        break;
      }

      int ino = path_to_ino( (char*)ctx->gpr[ 1 ], ROOT_DIR ); 
      if (ino == -1) break;
      
      inode_t dest; readInode( &dest, ino );  
      if (dest.i_ic.ic_mode != IFDIR) break;

      dir_t dir;
      inode_t inode; readInode( &inode, cwd );
      if (remove_inode( &dir, &inode, (char*)ctx->gpr[ 0 ] ) != -1) { 
        addInodeToDirectory( &dest, dir.d_ino, dir.d_name );        
      }

      disk_wr( fs.fs_sblkno, (uint8_t*)(&fs), sizeof( fs_t ) ); // update sb
  
      break;
    }
    case 0x14 : { // cp
      if ( ((char*)ctx->gpr[ 0 ])[0] == '.' ) {
        ctx->gpr[ 0 ] = -1;
        break;
      }

      int ino = path_to_ino( (char*)ctx->gpr[ 0 ], cwd ); 
      if (ino == -1) break;
      
      inode_t src; readInode( &src, ino );  
      if (src.i_ic.ic_mode == IFZERO) break; 

      inode_t dest;
      copyInode( &dest, &src );

      addInodeToDirectory( readInode( &src, cwd ), dest.i_number, (char*)ctx->gpr[ 1 ] );        

      disk_wr( fs.fs_sblkno, (uint8_t*)(&fs), sizeof( fs_t ) ); // update sb

      break;
    }
    case 0x15 : { // tell
      ctx->gpr[ 0 ] = tell( ctx->gpr[ 0 ] );
      break;
    }
    case 0x16 : {
      ctx->gpr[ 0 ] = mq_unlink( ctx->gpr[ 0 ] );
    }
    default: {
      break;
    }
  }

  return;
}
