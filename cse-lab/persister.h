#ifndef persister_h
#define persister_h

#include <fcntl.h>
#include <mutex>
#include <iostream>
#include <fstream>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fstream>
#include "rpc.h"

#define MAX_LOG_SZ 131072

/*
 * Your code here for Lab2A:
 * Implement class chfs_command, you may need to add command types such as
 * 'create', 'put' here to represent different commands a transaction requires. 
 * 
 * Here are some tips:
 * 1. each transaction in ChFS consists of several chfs_commands.
 * 2. each transaction in ChFS MUST contain a BEGIN command and a COMMIT command.
 * 3. each chfs_commands contains transaction ID, command type, and other information.
 * 4. you can treat a chfs_command as a log entry.
 */

typedef unsigned long long txid_t;
enum cmd_type{
	CMD_BEGIN = 0,
	CMD_COMMIT,
	CMD_CREATE,
	CMD_PUT,
	CMD_REMOVE
};

class chfs_command {
public:
    cmd_type type = CMD_BEGIN;
    txid_t id = 0;
    std::string content = "";
    uint32_t fileType;
    unsigned long long ino = 0;
    // constructor
    chfs_command() {}

    uint64_t size() const {
        uint64_t s = sizeof(cmd_type) + sizeof(txid_t);
        return s;
    }
};

/*
 * Your code here for Lab2A:
 * Implement class persister. A persister directly interacts with log files.
 * Remember it should not contain any transaction logic, its only job is to 
 * persist and recover data.
 * 
 * P.S. When and how to do checkpoint is up to you. Just keep your logfile size
 *      under MAX_LOG_SZ and checkpoint file size under DISK_SIZE.
 */
template<typename command>
class persister {

public:
    persister(const std::string& file_dir);
    ~persister();

    // persist data into solid binary file
    // You may modify parameters in these functions
    void append_log(const command& log);
    void checkpoint();

    // restore data from solid binary file
    // You may modify parameters in these functions
    void restore_logdata();
    void restore_checkpoint();
    std::vector<command> log_entries;
    unsigned long long trNum = 0;

private:
    std::mutex mtx;
    std::string file_dir;
    std::string file_path_checkpoint;
    std::string file_path_logfile;
    int fd;
    FILE *fpLog = NULL;
    FILE *fpCht = NULL;
    // restored log data
    //std::vector<command> log_entries;
};

template<typename command>
persister<command>::persister(const std::string& dir){
    // DO NOT change the file names here
    file_dir = dir;
    file_path_checkpoint = file_dir + "/checkpoint.bin";
    file_path_logfile = file_dir + "/logdata.bin";

    fpLog = fopen("log/logdata.bin","a+");//open or new logdata.bin
    fpCht = fopen("log/checkpoint.bin","a+");//open or new checkpoint.bin
}

template<typename command>
persister<command>::~persister() {
    // Your code here for lab2A

}

template<typename command>
void persister<command>::append_log(const command& log) {
    // Your code here for lab2A

    log_entries.push_back(log);
    
    std::string logType = "";//type of log
    std::string logDetail = "";//detail og log
    if(log.type == CMD_CREATE){
    	logType = "CMD_CREATE";
	logDetail = logType+" "+std::to_string(log.id)+" "+std::to_string(log.fileType)+" "+"\n";
    	//CREATE id fileType
    }else if(log.type == CMD_PUT){
    	logType = "CMD_PUT";
	logDetail = logType+" "+std::to_string(log.id)+" "+std::to_string(log.ino)+" "+log.content+"\n"+"put over"+"\n";
	//PUT id inodeNum content (put over)
    }else if(log.type == CMD_BEGIN){
    	logType = "CMD_BEGIN";
	logDetail = logType+" "+std::to_string(log.id)+" "+"\n";
	//BEGIN id
    }else if(log.type == CMD_COMMIT){
    	logType = "CMD_COMMIT";
	logDetail = logType+" "+std::to_string(log.id)+" "+"\n";
	//COMMIT id
    }else if(log.type == CMD_REMOVE){
    	logType = "CMD_REMOVE";
	logDetail = logType+" "+std::to_string(log.id)+" "+std::to_string(log.ino)+" "+"\n";
	//REMOVE id inodeNum
    }
    int fdLog = open("log/logdata.bin",O_RDWR + O_APPEND);
    int fdCht = open("log/checkpoint.bin",O_RDWR + O_APPEND);
    write(fdLog,logDetail.c_str(),logDetail.size());
    write(fdCht,logDetail.c_str(),logDetail.size());
    if(logType == "CMD_COMMIT")ftruncate(fdLog,0);
    sync();
    close(fdLog);
    close(fdCht);
}

template<typename command>
void persister<command>::checkpoint() {
    // Your code here for lab2A

}

template<typename command>
void persister<command>::restore_logdata() {
    // Your code here for lab2A
    std::ifstream fp("log/checkpoint.bin");
    std::string logLine;
    int left,right;
    std::string logType,ID,tmp;
    while(getline(fp,logLine)){//get a line from *fp to logLine
    	
	//get logType
	left = 0;
	right = logLine.find(" ");
	logType = logLine.substr(left,right-left);
	printf("%s\n",logType);
	//get id
	left = right+1;
	right = logLine.find(" ",left);
	ID = logLine.substr(left,right-left);

	if(logType == "CMD_CREATE"){
	    chfs_command log;
	    log.type = CMD_CREATE;
	    log.id = std::strtoull(ID.c_str(),nullptr,10);
	    
	    left = right+1;
	    tmp = logLine.substr(left,1);//fileType
	    log.fileType = std::atoi(tmp.c_str());
	    
	    log_entries.push_back(log);
	    logLine = "";
	}else if(logType == "CMD_BEGIN"){
	    chfs_command log;
	    log.type = CMD_BEGIN;
	    log.id = std::strtoull(ID.c_str(),nullptr,10);
	    
	    log_entries.push_back(log);
	    logLine = "";
	}else if(logType == "CMD_COMMIT"){
	    chfs_command log;
	    log.type = CMD_COMMIT;
	    log.id = std::strtoull(ID.c_str(),nullptr,10);

	    log_entries.push_back(log);
	    logLine = "";
	}else if(logType == "CMD_REMOVE"){
	    chfs_command log;
	    log.type = CMD_REMOVE;
	    log.id = std::strtoull(ID.c_str(),nullptr,10);

	    left = right+1;
	    right = logLine.find(" ",left);
	    tmp = logLine.substr(left,right-left);//inodeNum
	    log.ino = std::strtoull(tmp.c_str(),nullptr,10);

	    log_entries.push_back(log);
	    logLine = "";
	}else if(logType == "CMD_PUT"){
	    chfs_command log;
	    log.type = CMD_PUT;
	    log.id = std::strtoull(ID.c_str(),nullptr,10);
	    
	    left = right+1;
	    right = logLine.find(" ",left);
	    tmp = logLine.substr(left,right-left);//inodeNum
	    log.ino = std::strtoull(tmp.c_str(),nullptr,10);
	    
	    left = right+1;
	    right = logLine.find("\n",left);
	    std::string con = logLine.substr(left,right-left);
	    //first line of content
	    logLine = "";
	    while(getline(fp,logLine)){
	    	if(logLine == "put over")break;
		con.append("\n"+logLine);
		logLine = "";
	    }
	    log.content = con;
	    
	    log_entries.push_back(log);
	    logLine = "";
	}
    }
};

template<typename command>
void persister<command>::restore_checkpoint() {
    // Your code here for lab2A

};

using chfs_persister = persister<chfs_command>;

#endif // persister_h
