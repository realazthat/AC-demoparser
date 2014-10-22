#ifndef LOGSTATS_HPP
#define LOGSTATS_HPP 1

#include <string>
#include <map>
#include <boost/any.hpp>



extern std::ostream* logstats_out_ptr;
extern std::ostream* logstats_debug_ptr;

typedef std::map<std::string, boost::any> logdata_t;

logdata_t merge_logdata(const logdata_t& data0, const logdata_t& data1);
void logstats(const char* type, const logdata_t& data1, const logdata_t& data0=logdata_t());

#endif // LOGSTATS_HPP
