// the extent server implementation

#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "extent_server.h"
#include "persister.h"

extent_server::extent_server() 
{
  im = new inode_manager();
  //_persister = new chfs_persister("log"); // DO NOT change the dir name here
  /*(_persister->restore_logdata();//fill log_entries
  std::vector<chfs_command> commands;
  std::vector<chfs_command>::iterator it=_persister->log_entries.begin();
  bool committed = false;
  for(;it!=_persister->log_entries.end();++it){
    //CMD_BEGIN ---> new transaction
    if(commands.size()>0 && (*it).type == CMD_BEGIN)commands.clear();
    
    commands.push_back(*it);
    
    if((*it).type == CMD_COMMIT)committed = true;
    if(committed){
      std::vector<chfs_command>::iterator tr = commands.begin();
      for(;tr!=commands.end();++tr){
      	if((*tr).type == CMD_CREATE){
	  im->alloc_inode((*tr).fileType);
	}else if((*tr).type == CMD_PUT){
	  extent_protocol::extentid_t inodeNum = (*tr).ino;
	  std::string con = (*tr).content;
	  inodeNum &= 0x7fffffff;
	  im->write_file(inodeNum,con.c_str(),con.size());
	}else if((*tr).type ==CMD_REMOVE){
	  im->remove_file((*tr).ino);
	}
      }
      committed = false;
      commands.clear();
    }
  }*/
  // Your code here for Lab2A: recover data on startup
}

int extent_server::create(uint32_t type, extent_protocol::extentid_t &id)
{
  // alloc a new inode and return inum
  //printf("extent_server: create inode\n");
  
  //create_log(type);
  id = im->alloc_inode(type);
  return extent_protocol::OK;
}

int extent_server::put(extent_protocol::extentid_t id, std::string buf, int &)
{
  //put_log(buf,id);
  id &= 0x7fffffff;
  im->write_file(id, buf.c_str(), buf.size());
  return extent_protocol::OK;
}

int extent_server::get(extent_protocol::extentid_t id, std::string &buf)
{
  //printf("extent_server: get %lld\n", id);

  id &= 0x7fffffff;
  int size = 0;
  char *cbuf = NULL;

  im->read_file(id, &cbuf, &size);
  if (size == 0)
    buf = "";
  else {
    buf.assign(cbuf, size);
    free(cbuf);
  }

  return extent_protocol::OK;
}

int extent_server::getattr(extent_protocol::extentid_t id, extent_protocol::attr &a)
{
  //printf("extent_server: getattr %lld\n", id);

  id &= 0x7fffffff;
  extent_protocol::attr attr;
  memset(&attr, 0, sizeof(attr));
  im->get_attr(id, attr);
  a = attr;

  return extent_protocol::OK;
}

int extent_server::remove(extent_protocol::extentid_t id, int &)
{
  //printf("extent_server: write %lld\n", id);
  //remove_log(id);
  id &= 0x7fffffff;
  im->remove_file(id);
  return extent_protocol::OK;
}

void extent_server::create_log(uint32_t type){
/*  chfs_command command;
  command.id = _persister->trNum;
  command.type = CMD_CREATE;
  command.fileType = type;
  _persister->append_log(command);
*/}

void extent_server::put_log(std::string buf,extent_protocol::extentid_t id){
/*  chfs_command command;
  command.id = _persister->trNum;
  command.type = CMD_PUT;
  command.content = buf;
  command.ino = id;
  _persister->append_log(command);
*/}

void extent_server::remove_log(extent_protocol::extentid_t id){
/*  chfs_command command;
  command.id = _persister->trNum;
  command.type = CMD_REMOVE;
  command.ino = id;
  _persister->append_log(command);
*/}

void extent_server::begin(){
  /*chfs_command command;
  _persister->trNum++;
  command.id = _persister->trNum;
  command.type = CMD_BEGIN;
  _persister->append_log(command);
*/
  }

void extent_server::commit(){
  /*chfs_command command;
  command.id = _persister->trNum;
  command.type = CMD_COMMIT;
  _persister->append_log(command);
*/
  }












