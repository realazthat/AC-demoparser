
#include <tclap/CmdLine.h>
#include "logstats.hpp"
#include <iostream>
#include <fstream>
#include <boost/algorithm/string.hpp>

void stats_setupdemoplayback(const char* demopath=NULL);
void stats_readdemo();

#define AC_STATS_VERSION_STR "0.1"

int entry(int argc, const char** argv)
{
    try {
        TCLAP::CmdLine cmd("dumps stats from a demo file", ' ', AC_STATS_VERSION_STR);
        TCLAP::UnlabeledValueArg<std::string> demoArg(
            "demopath" /*name*/,
            "path to demo file" /*desc*/,
            true /*req*/,
            "" /*value (default)*/,
            "demo-file-path" /*typeDesc*/,
            cmd /*parser*/);
        TCLAP::ValueArg<std::string> eventsArg(
            "e" /*flag*/,
            "events" /*name*/,
            "path to file containing list of comma-separated events to output;"
            "defaults to all events if not specified" /*desc*/,
            false /*req*/,
            "" /*value (default)*/,
            "events-file-path" /*typeDesc*/,
            cmd /*parser*/);
        TCLAP::ValueArg<std::string> outputArg(
            "o" /*flag*/,
            "output" /*name*/,
            "path to dump file; dumps to stdout if not not specified" /*desc*/,
            false /*req*/,
            "" /*value (default)*/,
            "output-file-path" /*typeDesc*/,
            cmd /*parser*/);
        TCLAP::ValueArg<std::string> logArg(
            "d" /*flag*/,
            "debug" /*name*/,
            "path to debug log file" /*desc*/,
            false /*req*/,
            "" /*value (default)*/,
            "log-file-path" /*typeDesc*/,
            cmd /*parser*/);
        
        
        cmd.parse( argc, argv );
        
        std::string demopath = demoArg.getValue();
        
        std::ofstream out,debug;
        logstats_out_ptr = &std::cout;
        logstats_debug_ptr = NULL;
        
        if (outputArg.isSet() && outputArg.getValue() != "-")
        {
            out.open(outputArg.getValue(), std::ios::out);
            
            logstats_out_ptr = &out;
        }
        
        if (logArg.isSet())
        {
            debug.open(logArg.getValue(), std::ios::out);
            
            logstats_debug_ptr = &debug;
        }
        
        if (eventsArg.isSet())
        {
            output_types.clear();
            std::ifstream eventlist(eventsArg.getValue(), std::ios::in);
            
            std::string line;
            
            while(std::getline(eventlist, line, ','))
            {
                boost::algorithm::trim(line);
                
                if (line.size() == 0)
                    continue;
                    
                output_types.insert(line);
            }
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


/**
 * http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system#
 */
#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int CALLBACK WinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPSTR       lpCmdLine,
    int         nCmdShow
    )
{
    return entry(__argc, (const char**)(__argv));
}

#else

int main(int argc, const char** argv)
{
    return entry(argc, argv);
}

#endif

