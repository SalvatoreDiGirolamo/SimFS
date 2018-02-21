#include "toolbox/FileSystemHelper.h"
#include "toolbox/Logger.h"
#include "toolbox/KeyValueStore.h"
#include "toolbox/NetworkHelper.h"
#include "getopt/dv_cmdline.h"
#include "dv.h"

#include "server/DV.h"

#include <sys/types.h>
#include <unistd.h>


#define DIR_SIMFS ".simfs"

#define SIMFS_WORKSPACE "/home/salvo/simfs"
#define SIMFS_WSNAME "workspace"

#define CONF_NAME "conf.dv"

#define IPKEY(env) env + "_ip"
#define PORTKEY(env) env + "_port"

#define SIMFS_INIT "init"
#define SIMFS_START "start"
#define SIMF_LS "ls"
#define SIMFS_INDEX "index"
#define SIMFS_VIRTUALIZE "virtualize"


using namespace dv;
using namespace toolbox;
using namespace std;

volatile int gdbstop;

namespace simfs { 

    /* Search a configuration file. 
     * Starts from the CWD up to the root, looking for .simfs/conf.dv. 
    */
    string findConf(){
        string current_path = FileSystemHelper::getCwd();
        while (!FileSystemHelper::folderExists(current_path + "/" + DIR_SIMFS) && current_path!="/"){
            current_path = FileSystemHelper::getDirname(current_path);
        }

        if (current_path=="/"){
            LOG(ERROR, "This is not a SimFS envirorment!");
            return "";
        }
               
        string conf_file = current_path + "/" + DIR_SIMFS + "/" + CONF_NAME;
        
        if (!FileSystemHelper::fileExists(conf_file)) {
            LOG(ERROR, "SimFS configuration file not found!");
            return "";
        }
        
        LOG(INFO, "Configuration file found in: " + conf_file);

        return conf_file;
    }

    /* Loads the SimFS workspace. This is general, not simulation dependent */
    int loadWorkspace(KeyValueStore &ws){
        /* check if the workspace exists */
        if (!FileSystemHelper::folderExists(SIMFS_WORKSPACE)){
            printf("Creating workspace: %s\n", SIMFS_WORKSPACE);
            if (FileSystemHelper::mkDir(SIMFS_WORKSPACE)) return 0;
        }

        /* load simfs general configuration */
        ws.fromFile(SIMFS_WORKSPACE + std::string("/") + SIMFS_WSNAME);

        return 1;

    }
    
    /* Saves the SimFS workspace */
    void saveWorkspace(KeyValueStore &ws){
        ws.toFile(SIMFS_WORKSPACE"/workspace");
    }


    /* Initializes a virtualized envirorment (i.e., simulation). */
    int initEnvirorment(KeyValueStore &ws, std::string name, std::string conf_file){

        /* create local folder */
        if (toolbox::FileSystemHelper::mkDir(".simfs")) {
            printf("Failed to initialize simfs dir!\n");
            return -1;
        }

        /* copy conf file */
        toolbox::FileSystemHelper::cpFile(conf_file, DIR_SIMFS + std::string("/") + CONF_NAME);
 
        /* save key in the workspace */        
        string current_path = FileSystemHelper::getCwd();
        ws.setString(name, current_path);
        simfs::saveWorkspace(ws);

        return 0;    
    }
}


void error_exit(const std::string &name, const std::string &additional_text) {
	std::cout << "Usage: " << name << " <command> [OPTIONS]" << std::endl;
	std::cout << "\t" << name << " help for list of OPTIONS" << std::endl;
	std::cout << std::endl;
    LOG(ERROR, additional_text);
	exit(1);
}


int main(int argc, char * argv[]){

    /* Let GDB in */
    int attach_gdb = getenv("DV_ATTACH_GDB")!=NULL;
    if (attach_gdb){
        gdbstop=1;
        std::cout << "Waiting for GDB (PID: " << getpid() << "; type 'set gdbstop=0' to continue)." << std::endl;
        while (gdbstop){;}
    }

    /* Load SimFS workspace */
    KeyValueStore ws;
    if (!simfs::loadWorkspace(ws)){
        LOG(ERROR, "Failed the load SimFS configuration!");
        exit(-1);
    }



    /* Parsing & checking arguments */
	if (argc < 2) {
		error_exit(argv[0], "A command is expected!");
	}


    std::string cmd(argv[1]);    


    if (cmd == SIMFS_VIRTUALIZE){
        /* we do not parse the arguments in this case */
        
    }

    
    /* Executing commands*/
    if (cmd == SIMFS_INIT){

        printf("init!\n");
        if (argc!=4) {
            error_exit(argv[0], "Invalid syntax. See usage.");
        }
        
        std::string env = argv[2];
        std::string conf_file = argv[3];

        simfs::initEnvirorment(ws, env, conf_file);

    }else if (cmd == SIMFS_VIRTUALIZE){
        if (argc<3){
            error_exit(argv[0], "Invalid syntax. See usage.");
        }

        std::string env = argv[2];
        if (!ws.hasKey(env)){ error_exit(argv[0], "This environment does not exit!"); }
        std::string conf_file = ws.getString(env) + CONF_NAME;

        /* Check if there is a known IP/PORT for it */
        if (ws.hasKey(IPKEY(env)) && ws.hasKey(PORTKEY(env))){
            string ip = ws.getString(IPKEY(env));
            string port = ws.getString(PORTKEY(env));
            
            printf("DV should be listening on %s:%s\n", ip.c_str(), port.c_str());

        }

       
    }else if (cmd == SIMFS_START){

	    gengetopt_args_info args_info;
    
        if (argc<3){
            error_exit(argv[0], "SimFS envirorment not specified!");
        }
    
        std::string env(argv[2]);     
        argv[2] = argv[0];

        /* Parse DV options */
        cmdline_parser(argc-2, &(argv[2]), &args_info);
    	if (cmdline_parser(argc, argv, &args_info) != 0) {
		    error_exit(argv[0], "Error while parsing command line arguments");
	    }

        /* Load env */
        if (!ws.hasKey(env)){ error_exit(argv[0], "This environment does not exit!"); }
        std::string conf_file = ws.getString(env) + "/" + DIR_SIMFS + "/" + CONF_NAME;
       
        DV * dv = DVCreate(args_info, conf_file);
        if (dv==NULL){ exit(1); }
 
        /* Save the IP/PORT on which we are listening */
        std::string ip = dv->getIpAddress();
        if (ip=="0.0.0.0"){
            vector<string> ips;
            NetworkHelper::getAllIPs(ips);
            ip = "";
            for (auto lip : ips){ ip = ip + lip + ","; }            
        }

        std::string port = dv->getClientPort();        
   
        ws.setString(IPKEY(env), ip);
        ws.setString(PORTKEY(env), port);               
        simfs::saveWorkspace(ws);


        dv->run(); 
        delete dv;

	    cmdline_parser_free(&args_info);
    }




    /* Cleaning up */

	return 0;
}

