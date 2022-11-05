#include "inode_manager.h"

// disk layer -----------------------------------------

disk::disk()
{
  bzero(blocks, sizeof(blocks));
}

void
disk::read_block(blockid_t id, char *buf)
{    if(id<0||id>=BLOCK_NUM){
	   return;
			}
     memcpy(buf,blocks[id],BLOCK_SIZE);

}

void
disk::write_block(blockid_t id, const char *buf)
{
	if(id<0||id>=BLOCK_NUM){
		return;}

	memcpy(blocks[id],buf,BLOCK_SIZE); 
}

// block layer -----------------------------------------

// Allocate a free disk block.
blockid_t
block_manager::alloc_block()
{
  /*
   * your code goes here.
   * note: you should mark the corresponding bit in block bitmap when alloc.
   * you need to think about which block you can start to be allocated.
   */
 int start=IBLOCK(INODE_NUM,sb.nblocks)+1;
 for(int i=start;i<BLOCK_NUM;i++){
	 if(using_blocks[i]==0){
		 using_blocks[i]=1;
		 return i;
	 }
 }
 


  return 0;
}

void
block_manager::free_block(uint32_t id)
{
  /* 
   * your code goes here.
   * note: you should unmark the corresponding bit in the block bitmap when free.
   */
  if(id<0||id>=BLOCK_NUM)
	  return;
  using_blocks[id]=0;


  return;
}

// The layout of disk should be like this:
// |<-sb->|<-free block bitmap->|<-inode table->|<-data->|
block_manager::block_manager()
{
  d = new disk();
  for(uint32_t i=0;i<IBLOCK(INODE_NUM,BLOCK_NUM);i++){
	  using_blocks[i]=0;
  }

  // format the disk
  sb.size = BLOCK_SIZE * BLOCK_NUM;
  sb.nblocks = BLOCK_NUM;
  sb.ninodes = INODE_NUM;

}

void
block_manager::read_block(uint32_t id, char *buf)
{
  d->read_block(id, buf);
}

void
block_manager::write_block(uint32_t id, const char *buf)
{
  d->write_block(id, buf);
}

// inode layer -----------------------------------------

inode_manager::inode_manager()
{
  bm = new block_manager();
  uint32_t root_dir = alloc_inode(extent_protocol::T_DIR);
  if (root_dir != 1) {
    printf("\tim: error! alloc first inode %d, should be 1\n", root_dir);
    exit(0);
  }
}

/* Create a new file.
 * Return its inum. */
uint32_t
inode_manager::alloc_inode(uint32_t type)
{
  /* 
   * your code goes here.
   * note: the normal inode block should begin from the 2nd inode block.
   * the 1st is used for root_dir, see inode_manager::inode_manager().
   */

uint32_t inodeNum=1;
for(int i=1;i<INODE_NUM;i++){
inode_t * inode=get_inode(inodeNum);
if(!inode){
	inode=(inode_t *)malloc(sizeof(inode_t));
	bzero(inode,sizeof(inode_t));
	inode->atime=time(NULL);
	inode->mtime=time(NULL);
	inode->ctime=time(NULL);
	inode->size=0;
	inode->type=type;
	put_inode(inodeNum,inode);
	break;
}
inodeNum++;
}
  return inodeNum;
}

void
inode_manager::free_inode(uint32_t inum)
{
  /* 
   * your code goes here.
   * note: you need to check if the inode is already a freed one;
   * if not, clear it, and remember to write back to disk.
   */

inode_t * inode=get_inode(inum);
if(inode!=NULL){
	if(inode->type!=0){
		inode->type=0;
		put_inode(inum,inode);
		free(inode);
	}
};






  return;
}


/* Return an inode structure by inum, NULL otherwise.
 * Caller should release the memory. */
struct inode* 
inode_manager::get_inode(uint32_t inum)
{
  struct inode *ino;
  /* 
   * your code goes here.
   */
  struct inode *tmpinode;
  char buf[BLOCK_SIZE];
  bm->read_block(IBLOCK(inum,bm->sb.nblocks),buf);
  tmpinode=(struct inode *)buf+inum%IPB;
  if(tmpinode->type==0){
	  return NULL;}
  ino=(struct inode *)malloc(sizeof(struct inode));
  *ino=*tmpinode;


  return ino;
}

void
inode_manager::put_inode(uint32_t inum, struct inode *ino)
{
  char buf[BLOCK_SIZE];
  struct inode *ino_disk;

  printf("\tim: put_inode %d\n", inum);
  if (ino == NULL)
    return;

  bm->read_block(IBLOCK(inum, bm->sb.nblocks), buf);
  ino_disk = (struct inode*)buf + inum%IPB;
  *ino_disk = *ino;
  bm->write_block(IBLOCK(inum, bm->sb.nblocks), buf);
}

#define MIN(a,b) ((a)<(b) ? (a) : (b))

/* Get all the data of a file by inum. 
 * Return alloced data, should be freed by caller. */
void
inode_manager::read_file(uint32_t inum, char **buf_out, int *size)
{
  /*
   * your code goes here.
   * note: read blocks related to inode number inum,
   * and copy them to buf_out

*/
int pos=0;	
struct inode *ino=get_inode(inum);
*buf_out=(char *)malloc(ino->size);
if(ino->size==0){
	*size=0;
	ino->atime=(unsigned int)time(NULL);
	put_inode(inum,ino);
	return;
}

if(ino->size>NDIRECT*BLOCK_SIZE){
	for(int i=0;i<NDIRECT;i++){
		char buf[BLOCK_SIZE];
		bm->read_block(ino->blocks[i],buf);
                memcpy(*buf_out+pos,buf,BLOCK_SIZE);
		pos+=BLOCK_SIZE;
	}
	char indirectBlock[BLOCK_SIZE];
	bm->read_block(ino->blocks[NDIRECT],indirectBlock);

for(int i=0;i<(ino->size-NDIRECT*BLOCK_SIZE-1)/BLOCK_SIZE+1;i++){
  char buf[BLOCK_SIZE];
  bm->read_block(*((uint32_t*)indirectBlock+i),buf);
  if( i!=(ino->size-NDIRECT*BLOCK_SIZE-1)/BLOCK_SIZE){
	memcpy(*buf_out+pos,buf,BLOCK_SIZE);
	pos+=BLOCK_SIZE;
     }
  else{
	memcpy(*buf_out+pos,buf,(ino->size-1)%BLOCK_SIZE+1);
        pos+=(ino->size-1)%BLOCK_SIZE+1;}
}}
else{
	for(int i=0;i<(ino->size-1)/BLOCK_SIZE+1;i++){
		char buf[BLOCK_SIZE];
		bm->read_block(ino->blocks[i],buf);
		if(i!=(ino->size-1)/BLOCK_SIZE){
	        	memcpy(*buf_out+pos,buf,BLOCK_SIZE);
	        	pos+=BLOCK_SIZE;}
	        else{
			memcpy(*buf_out+pos,buf,(ino->size-1)%BLOCK_SIZE+1);
			pos+=(ino->size-1)%BLOCK_SIZE+1;}
	}
}
 
ino->atime=(unsigned int )time(NULL);
put_inode(inum,ino);
*size=pos;
                


  
  return;
}

/* alloc/free blocks if needed */
void
inode_manager::write_file(uint32_t inum, const char *buf, int size)
{
  /*
   * your code goes here.
   * note: write buf to blocks of inode inum.
   * you need to consider the situation when the size of buf 
   * is larger or smaller than the size of original inode
   */
   	struct inode *inode = get_inode(inum);
	unsigned int old_blocknum = (inode->size+BLOCK_SIZE-1)/BLOCK_SIZE;
	unsigned int new_blocknum = (size+BLOCK_SIZE-1)/BLOCK_SIZE;
	char indirect[BLOCK_SIZE];

	if (old_blocknum > new_blocknum)//need free
	{
		if(new_blocknum > NDIRECT)
		{
		 	bm->read_block(inode->blocks[NDIRECT],indirect);
			for(unsigned int i = new_blocknum;i<old_blocknum;++i)
				bm->free_block(*((blockid_t *)indirect+(i-NDIRECT)));
		}else if(new_blocknum<=NDIRECT && old_blocknum>NDIRECT)
		{
			bm->read_block(inode->blocks[NDIRECT],indirect);
			for(unsigned int i=NDIRECT;i<old_blocknum;++i)
				bm->free_block(*((blockid_t *)indirect+(i-NDIRECT)));
			bm->free_block(inode->blocks[NDIRECT]);
			for (unsigned int i = new_blocknum; i < NDIRECT; i++)
				bm->free_block(inode->blocks[i]);
		}else{
			for (unsigned int i = new_blocknum; i < old_blocknum; i++)
				bm->free_block(inode->blocks[i]);
		}
	}else{
		if(new_blocknum <= NDIRECT){
			for (unsigned int i = old_blocknum; i < new_blocknum; i++)
				inode->blocks[i] = bm->alloc_block();
		}else if(new_blocknum>NDIRECT && old_blocknum<=NDIRECT){
			for (unsigned int i = old_blocknum; i < NDIRECT; i++)
				inode->blocks[i] = bm->alloc_block();
			inode->blocks[NDIRECT] = bm->alloc_block();
			bzero(indirect,BLOCK_SIZE);
			for (unsigned int i = NDIRECT; i < new_blocknum; i++)
				*((blockid_t *)indirect + (i - NDIRECT)) = bm->alloc_block();
			bm->write_block(inode->blocks[NDIRECT], indirect);
		}else{
			bm->read_block(inode->blocks[NDIRECT], indirect);
			for (unsigned int i = old_blocknum; i < new_blocknum; i++)
				*((blockid_t *)indirect + (i - NDIRECT)) = bm->alloc_block();
			bm->write_block(inode->blocks[NDIRECT], indirect);
		}
	}
	char tail_block[BLOCK_SIZE];
	int pos = 0;
	for (int i = 0; i < NDIRECT && pos < size; i++) {  
		if (size - pos > BLOCK_SIZE) {
		    	bm->write_block(inode->blocks[i], buf+pos); 
		        pos += BLOCK_SIZE;
		}else{
			int left_len = size - pos;
			memcpy(tail_block, buf + pos, left_len);
			bm->write_block(inode->blocks[i], tail_block);
			pos += left_len;
		}
	}
	if(pos < size){
		bm->read_block(inode->blocks[NDIRECT], indirect);
		for (unsigned int i = 0; i < NINDIRECT && pos < size; i++) {
			blockid_t blockid = *((blockid_t *)indirect + i);
			if (size - pos > BLOCK_SIZE){
			        bm->write_block(blockid, buf + pos);
			        pos += BLOCK_SIZE;
			}else{
				int left_len = size - pos;
				memcpy(tail_block, buf + pos, left_len);
				bm->write_block(blockid, tail_block);
				pos += left_len;
			}		
		}
	}
	inode->size = size;
	inode->atime = (unsigned int)time(NULL);
	inode->mtime = (unsigned int)time(NULL);
	inode->ctime = (unsigned int)time(NULL);
	put_inode(inum,inode);
	free(inode);
}

void
inode_manager::get_attr(uint32_t inum, extent_protocol::attr &a)
{
  /*
   * your code goes here.
   * note: get the attributes of inode inum.
   * you can refer to "struct attr" in extent_protocol.h
   */

 struct inode * ino=get_inode(inum);
if(ino==NULL){
       a.type=0;
       return;}
a.size=ino->size;
a.type=ino->type;
a.atime=ino->atime;
a.mtime=ino->mtime;
a.ctime=ino->ctime;
  
  return;
}

void
inode_manager::remove_file(uint32_t inum)
{
  /*
   * your code goes here
   * note: you need to consider about both the data block and inode of the file
   */
  struct inode *ino=get_inode(inum);
  if(ino->size>0){
	  if(ino->size<=NDIRECT*BLOCK_SIZE){
		  for(int i=0;i<(ino->size-1)/BLOCK_SIZE+1;i++){
			  bm->free_block(ino->blocks[i]);}
	  }
	  else{
		  for(int i=0;i<NDIRECT;i++){
			  bm->free_block(ino->blocks[i]);}
		  char indirectBlock[BLOCK_SIZE];
		  bm->read_block(ino->blocks[NDIRECT],indirectBlock);
		  for(int i=0;i<(ino->size-1-NDIRECT*BLOCK_SIZE)/BLOCK_SIZE+1;i++){
                        bm->free_block(*((uint32_t *)indirectBlock+i));
                     }
		  bm->free_block(ino->blocks[NDIRECT]);
	  }}

free_inode(inum);
  return;
}
