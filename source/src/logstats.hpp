#ifndef LOGSTATS_HPP
#define LOGSTATS_HPP 1

#include <set>
#include <string>
#include <map>
#include <stdint.h>


extern std::ostream* logstats_out_ptr;
extern std::ostream* logstats_debug_ptr;
extern std::set<std::string> output_types;


struct dict_t{
    
    std::ostream* out;
    bool comma;
    dict_t(const char* name);
    ~dict_t();
    
    
    void reset(const char* name);
    void flush();
    
    void set(const char* key, bool value);
    void set(const char* key, double value);
    void set(const char* key, float value);
    void set(const char* key, int32_t value);
    void set(const char* key, int64_t value);
    void set(const char* key, uint32_t value);
    void set(const char* key, uint64_t value);
    void set(const char* key, const char* value);
    void set(const char* key, const std::string& value);
};

#endif // LOGSTATS_HPP
