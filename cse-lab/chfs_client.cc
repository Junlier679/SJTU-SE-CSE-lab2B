
// chfs client.  implements FS operations using extent and lock server
#include "chfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

chfs_client::chfs_client(std::string extent_dst, std::string lock_dst)
{
    ec = new extent_client(extent_dst);
    lc = new lock_client(lock_dst);
    if (ec->put(1, "") != extent_protocol::OK)
    printf("error init root dir\n"); // XYB: init root dir
}

chfs_client::inum
chfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string
chfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
chfs_client::isfile(inum inum)
{
    extent_protocol::attr a;
    lc->acquire(inum);
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        lc->release(inum);
	return false;
    }
    lc->release(inum);
    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        return true;
    } 
    printf("isfile: %lld is a dir\n", inum);
    return false;
}
/** Your code here for Lab...
 * You may need to add routines such as
 * readlink, issymlink here to implement symbolic link.
 * 
 * */

bool
chfs_client::isdir(inum inum)
{
    // Oops! is this still correct when you implement symlink?
	extent_protocol::attr a;
	lc->acquire(inum);
	if(ec->getattr(inum,a)!=extent_protocol::OK){
		printf("error getting attr\n");
		lc->release(inum);
		return false;
	}
	lc->release(inum);
	if(a.type==extent_protocol::T_DIR)
                return true;

        return false;
}

int
chfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    lc->acquire(inum);
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

    release:
    lc->release(inum);
    return r;
}

int
chfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    lc->acquire(inum);
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

    release:
    lc->release(inum);
    return r;
}


#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)

// Only support set size of attr
int
chfs_client::setattr(inum ino, size_t size)
{
    lc->acquire(ino);
    ec->begin();
    int r = OK;

    /*
     * your code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */

    std::string buf;
    ec->get(ino,buf);
    buf.resize(size);
    ec->put(ino,buf);
    ec->commit();
    lc->release(ino);
    return r;
}

int
chfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    lc->acquire(parent);
    ec->begin();
    int r = OK;

    /*
     * your code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */
    std::string tmp;
    bool flag=false;
    lookup(parent,name,flag,ino_out);
    if(flag){
	    r=EXIST;
    }else{
	    ec->create(extent_protocol::T_FILE,ino_out);
	    /*if(ec->get(parent,tmp)!=extent_protocol::OK){
		    return IOERR;}*/
	    ec->get(parent,tmp);
            std::string newfile=std::string(name)+"&"+filename(ino_out)+"/";
	    tmp.append(newfile);
	    /* if(ec->put(parent,tmp)!=extent_protocol::OK){
		    return IOERR;}*/
	    ec->put(parent,tmp);
    }
    ec->commit();	    
    lc->release(parent);
    return r;
}

int
chfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    lc->acquire(parent);
    ec->begin();
    int r = OK;

    /*
     * your code goes here.
     * note: lookup is what you need to check if directory exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */

    bool found=false;
    lookup(parent,name,found,ino_out);
    if(found){
	    r=EXIST;
    }else{
	    ec->create(extent_protocol::T_DIR,ino_out);
	    std::string tmp;
	    ec->get(parent,tmp);
	    tmp.append(std::string(name)+"&"+filename(ino_out)+"/");
	    ec->put(parent,tmp);
    }
    ec->commit();
    lc->release(parent);
    return r;
}

int
chfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
    int r = OK;

    /*
     * your code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */
    std::list<dirent>list;
    readdir(parent,list);
    if(list.empty()){
	    found=false;
	    return r;
    }else{
	    for(std::list<dirent>::iterator it=list.begin();it!=list.end();it++){
		    if(it->name.compare(name)==0){
			    found=true;
			    ino_out=it->inum;
			    return r;
		    }
	    }
    }
    found=false;
    return r;
}

int
chfs_client::readdir(inum dir, std::list<dirent> &list)
{
    int r = OK;

    /*
     * your code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */
    std::string dirString;
    ec->get(dir,dirString);
    int start=0;
    int end=dirString.find('&');
    while(end!=std::string::npos){
	    std::string filename=dirString.substr(start,end-start);
           // int istart=end+1;
	   // int iend=dirString.find('/',istart);
	    start=end+1;
	    end=dirString.find('/',start);
	    std::string inum=dirString.substr(start,end-start);
	    struct dirent d;
	    d.name=filename;
	    d.inum=n2i(inum);
	    list.push_back(d);
	    start=end+1;
	    end=dirString.find('&',start);
    }
    return r;
}

int
chfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
    lc->acquire(ino);
    int r = OK;

    /*
     * your code goes here.
     * note: read using ec->get().
     */
    std::string buf;
    ec->get(ino,buf);
    if(off<0 || off>buf.size()){
	    lc->release(ino);
	    return r;
    }
    if(off+size > buf.size()){
	    data = buf.substr(off);
	    lc->release(ino);
	    return r;
    }
    data=buf.substr(off,size);
    lc->release(ino);
    return r;
}

int
chfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
    lc->acquire(ino);
    ec->begin();
    int r = OK;

    /*
     * your code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */
    std::string buf,buf2;
    ec->get(ino,buf);
    buf2.assign(data,size);
    if(off+size > buf.size())
	    buf.resize(off+size,'\0');
    buf.replace(off,size,buf2);
    bytes_written = size;
    ec->put(ino,buf);
    ec->commit();
    lc->release(ino);
    return r;
    /*if(off+size <= buf.size()){
	    buf.replace(off,size,buf2);
	    bytes_written = size;
	    ec->put(ino,buf);
	    return r;
    }else{
	    buf.resize(off+size,'\0');
	    buf.replace(off,size,buf2);
	    bytes_written = size;
	    ec->put(ino,buf);
            return r;
    }*/
}

int chfs_client::unlink(inum parent,const char *name)
{
    lc->acquire(parent);
    ec->begin();
    int r = OK;

    /*
     * your code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */
    bool found=false;
    inum i;
    lookup(parent,name,found,i);
    
    if(found){
	    std::string tmp;
	    ec->get(parent,tmp);
	    int start=tmp.find(name);
	    int end=tmp.find('/',start);
	    tmp.erase(start,end-start+1);
	    ec->put(parent,tmp);
	    ec->remove(i);
    }
    ec->commit();
    lc->release(parent);
    return r;
}
int chfs_client::symlink(inum parent,const char * name,const char *link,inum & ino_out){
	lc->acquire(parent);
	ec->begin();
	int r=OK;
	bool found=false;
	lookup(parent,name,found,ino_out);
        if(found){
		r=EXIST;
	}
	else{
		std::string buf;
		ec->get(parent,buf);
		ec->create(extent_protocol::T_SYMLINK,ino_out);
		ec->put(ino_out,std::string(link));
		buf.append(std::string(name)+"&"+filename(ino_out)+"/");
		ec->put(parent,buf);
	}
	ec->commit();
	lc->release(parent);
	return r;
}

int chfs_client::readlink(inum ino,std::string &data){
	lc->acquire(ino);
	int r=OK;
	std::string buf;
	ec->get(ino,buf);
	data=buf;
	lc->release(ino);
	return r;
}
	
