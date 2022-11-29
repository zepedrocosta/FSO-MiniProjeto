//Ficheiro para correr os testes (MAIN)

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include "disk_driver.h"
extern struct disk_operations disk_ops;

#include "ffs_super.h"
extern struct IMsuper ffs_IMsb;
extern struct super_operations super_ops;

#include "ffs_inode.h"
extern struct inode_operations inode_ops;

#include "ffs_bytemap.h"
extern struct bmapMData bmapMD[NBR_OF_BMAPS];
extern struct bytemap_operations bmap_ops;


/***
  Create or Open a disk. Do not use Open because we now Close the disk here
  args[0]= D, args[1]= disk name, args[2]= # of blocks (0 to open) 
***/
void run_D(int argc, char* args[]) {
  int ercode;
  int nblocks= 0;

  if (argc == 3) nblocks= atoi(args[2]);
  ercode= disk_ops.open(args[1], nblocks);
  if (ercode < 0) {
    if (argc == 3) printf("%s %s %s\tCreate ERROR\n",args[0],args[1],args[2]);
    else printf("%s %s\t\tOpen ERROR\n", args[0], args[1]);
    printf( "create():disk_ops.open %s\n", strerror(-ercode) );
    return;
  } else {
    if (argc == 3) printf("%s %s %s\tCreate OK\n",args[0],args[1],args[2]);
    else printf("%s %s\t\tOpen OK\n", args[0], args[1]);
  }

  // Create will now close the disk
  ercode= disk_ops.close();
  if (ercode < 0) {
    printf("%s\t\tUnexpected Close ERROR\n", args[0]);
    printf( "create():disk_ops.close %s\n", strerror(-ercode) );
  }
}


/***
  Helper: this will be the format function
***/
void format(char *diskname, unsigned int sizeInArea,\
			    unsigned int clusterSize) {
  int ercode;

  ercode= disk_ops.open(diskname, 0);
  if (ercode < 0) {
    printf( "format():disk_ops.open %s\n", strerror(-ercode) );
    return;
  }

  ercode= disk_ops.stat();
  if (ercode < 0) {
    printf( "format():disk_ops.stat %s\n", strerror(-ercode) );
    return;
  }

  unsigned int nblocks= ercode;  
  super_ops.create(&ffs_IMsb.sb, nblocks, sizeInArea, clusterSize);

  ercode= super_ops.write(&ffs_IMsb.sb);
  if (ercode < 0) {
    printf( "format():super_ops.write %s\n", strerror(-ercode) );
    return;
  }

  ercode= bmap_ops.clear(INODE_BMAP,&ffs_IMsb.sb);
  if (ercode < 0) {
    printf( "format():bmap_ops.clear INODE_BMAP %s\n", strerror(-ercode) );
    return;
  }
  ercode= bmap_ops.clear(DATA_BMAP,&ffs_IMsb.sb);
  if (ercode < 0) {
    printf( "format():bmap_ops.clear DATA_BMAP %s\n", strerror(-ercode) );
    return;
  }

  ercode= inode_ops.clear(&ffs_IMsb.sb);
  if (ercode < 0) {
    printf( "format():inode_ops.clear %s\n", strerror(-ercode) );
    return;
  }

/*
  	MISSING MISSING MISSING MISSING MISSING MISSING MISSING
	MISSING clean of ?data?
*/


  // Format will now close the disk
  ercode= disk_ops.close();
  if (ercode < 0) {
    printf( "format():disk_ops.close %s\n", strerror(-ercode) );
  }
}
/***
  Format a disk
  args[0]= F, args[1]= disk name, args[2]= sizeInArea, args[3]= clusterSize
***/
void run_F(int argc, char* args[]) {
  unsigned int sizeInArea, clusterSize;

  if (argc == 4) { sizeInArea= atoi(args[2]); clusterSize= atoi(args[3]); }
  else {
    printf("%s %s\t\tERROR: missing information\n", args[0], args[1]);
    return;
  }

  printf("%s %s\t%s\t%s\tFormatting...\n", args[0], args[1], args[2], args[3]);
  format( args[1], sizeInArea, clusterSize );
}


/***
  Mount: Read in the superblock and optionally debug. Should NOT be mounted
  args[0]= M, args[1]= disk name args[2]= debug if 1, not if 0
***/
void run_M(int argc, char* args[]) {
  int ercode;
  
  ercode= super_ops.mount(args[1], &ffs_IMsb, 1);
  if (ercode < 0) {
    printf("mount: %s %s\tERROR\n",args[0],args[1]);
    return;
  }

  bmap_ops.mount(&ffs_IMsb.sb);
}


/***
  Umount: Set mounted= 0 and write the superblock. Close the disk
  args[0]= U
***/
void run_U(int argc, char* args[]) {
  int ercode;

  ercode=super_ops.umount(&ffs_IMsb);
  if (ercode < 0) printf("umount: %s \tERROR\n",args[0]);
}

/***
  Bytemap print table
  args[0]= B, args[1]= 0 INODE_BMAP 1 DATA_BMAP
***/
void run_B(int argc, char* args[]) {
  int ercode;

  ercode= bytemap_print_table(atoi(args[1]));
  if (ercode < 0) printf("ERROR bytemap_print_table\n");
}

/***
  Bytemap get free test
  args[0]= g, args[1]= 0 INODE_BMAP 1 DATA_BMAP
***/
void run_g(int argc, char* args[]) {

  printf("Bmap get free: %d\n", bmap_ops.getfree(atoi(args[1])) );
}


/***
  Bytemap set test
  args[0]= s, args[1]= 0 INODE_BMAP 1 DATA_BMAP
  args[2]= entry args[3]= value2set
***/
void run_s(int argc, char* args[]) {
  int ercode;

  ercode= bmap_ops.set(atoi(args[1]), atoi(args[2]), atoi(args[3]) );
  if (ercode < 0) printf("ERROR bytemap_set\n");
}


/***
  Allocate i-node: Write an inode
  args[0]= a, args[1]= abs number, args[2]= type
***/
void run_a(int argc, char* args[]) {
  int ercode;
  struct inode in;

  inode_ops.init(&in);
  in.nlinks= 1;
  in.type= (unsigned char)atoi(args[2]);
  ercode= inode_ops.update(atoi(args[1]), &in);
  if (ercode < 0) {
    printf("inode write: %s %s %s\tERROR\n",args[0],args[1],args[2]);
    return;
  }
}


/***
  Deallocate i-node: Write a clean inode in-place
  args[0]= d, args[1]= abs number
***/
void run_d(int argc, char* args[]) {
  int ercode;
  struct inode in;

  inode_ops.init(&in);
  ercode= inode_ops.update(atoi(args[1]), &in);
  if (ercode < 0) {
    printf("inode write: %s %s %s\tERROR\n",args[0],args[1],args[2]);
    return;
  }
}


/***
   Inode print table
   args[0]= I, args[1]= 0 all entries 1 valid only
***/
void run_I(int argc, char* args[]) {
  inode_print_table(atoi(args[1]));
}


void runcommand(int argc, char* args[]){

  switch (args[0][0]) {
    case 'D': // Disk open
      run_D(argc, args);
      break;
    case 'F': // Format disk
      run_F(argc, args);
      break;
    case 'M': // Mount and optionally debug, must be unmounted
      run_M(argc, args);
      break;
    case 'U': // Umount
      run_U(argc, args);
      break;
    case 'B': // Print Bytemap tables
      run_B(argc, args);
      break;
    case 'g': // Bytemap getfree
      run_g(argc, args);
      break;
    case 's': // Bytemap set
      run_s(argc, args);
      break;
    case 'a': // Inode allocate (write)
      run_a(argc, args);
      break;
    case 'd': // Inode deallocate (write clean inode in-place)
      run_d(argc, args);
      break;
    case 'I': // Print the Inode table
      run_I(argc, args);
      break;

    case '#': // Comment line
      break;

    default:
      printf("WRONG SPEC FILE?\n");
      break;
  }
}

/************************* MAIN & related code ***************************/

#define LINESIZE	64
#define ARGVMAX		32

int makeargv(char *s, char* argv[ARGVMAX+1]) {
  // in: s points a text string with words
  // out: argv[] points to all words in the string s (*s is modified!)
  // pre: argv is predefined as char *argv[ARGVMAX+1]
  // return: number of words pointed to by the elements in argv (or -1 in case of error)

  int ntokens;

  if ( s == NULL || argv == NULL || ARGVMAX == 0)
    return -1;

  ntokens = 0;
  argv[ntokens] = strtok(s, " \t\n");
  while ( (argv[ntokens] != NULL) && (ntokens < ARGVMAX) ) {
    ntokens++;
    argv[ntokens] = strtok(NULL, " \t\n");
  }
  argv[ntokens] = NULL; // it must terminate with NULL
  return ntokens;
}

void interp(FILE *fdesc) {
  char line[LINESIZE];
  char* av[ARGVMAX];
  int argc;

  while ( fgets( line, LINESIZE, fdesc ) != NULL ) {
    if ( (argc= makeargv(line, av) ) > 0 )
    runcommand(argc, av);
  }
}

int main (int argc, char *argv[]) {
  FILE *fdesc;

  // Util para correr no debugger ou alguns IDE
  if ( (argc == 3) && (strcmp(argv[1], "<")==0) ) {
    fdesc= fopen(argv[2], "r");
    if (fdesc == NULL) abort();
  } else
    fdesc= stdin;

  interp(fdesc);

  return 0;
}
