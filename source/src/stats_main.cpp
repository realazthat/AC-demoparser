
#include <tclap/CmdLine.h>
#include "logstats.hpp"
#include <iostream>
#include <fstream>

void stats_setupdemoplayback(const char* demopath=NULL);
void stats_readdemo();

#define AC_STATS_VERSION_STR "0.1"

int entry(int argc, const char** argv)
{
    try {
        TCLAP::CmdLine cmd("dumps stats from a demo file", ' ', AC_STATS_VERSION_STR);
        TCLAP::UnlabeledValueArg<std::string> demoArg("demopath","path to demo file",true,"","demo file path",cmd);
        TCLAP::ValueArg<std::string> outputArg("o", "output","path to dump file; dumps to stdout if not not specified",false,"","file path",cmd);
        TCLAP::ValueArg<std::string> logArg("d", "debug","path to debug log file",false,"","file path",cmd);
        
        
        cmd.parse( argc, argv );
        
        std::string demopath = demoArg.getValue();
        
        
        std::ofstream out,debug;
        logstats_out_ptr = &std::cout;
        logstats_debug_ptr = NULL;
        
        if (outputArg.isSet() && outputArg.getValue() != "-")
        {
            out.open(outputArg.getValue(), std::ios::app);
            
            logstats_out_ptr = &out;
        }
        
        if (logArg.isSet())
        {
            debug.open(logArg.getValue(), std::ios::app);
            
            logstats_out_ptr = &debug;
        }
        
        
        

        

        
        stats_setupdemoplayback(demopath.c_str());
        stats_readdemo();

        
        return 0;
    }
    catch (TCLAP::ArgException &e)  // catch any exceptions
    {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        return 1;
    }

}



