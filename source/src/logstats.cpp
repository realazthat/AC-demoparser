
#include "logstats.hpp"

#include <iostream>
#include <fstream>
#include <cctype>
#include <boost/format.hpp>


#include <boost/any.hpp>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/unordered_map.hpp>


std::ostream* logstats_out_ptr = NULL;
std::ostream* logstats_debug_ptr = NULL;


/**
 *  From http://stackoverflow.com/a/13399138/586784
 *       https://gist.github.com/pyrtsa/2945472
 */
struct type_info_hash {
    std::size_t operator()(std::type_info const & t) const {
        return 42; // t.hash_code();
    }
};


/**
 * From http://stackoverflow.com/a/11826666/586784
 */
class null_buffer : public std::streambuf
{
public:
  int overflow(int c) { return c; }
};
class null_stream : public std::ostream {
public:
    null_stream() : std::ostream(&m_sb) {}
private:
    null_buffer m_sb;
};

struct equal_ref {
    template <typename T> bool operator()(boost::reference_wrapper<T> a,boost::reference_wrapper<T> b) const {
        return a.get() == b.get();
    }
};
struct any_visitor {
    boost::unordered_map<boost::reference_wrapper<std::type_info const>, boost::function<void(boost::any&)>, type_info_hash, equal_ref> fs;

    template <typename T> static T any_cast_f(boost::any& any) { return boost::any_cast<T>(any); }

    template <typename T> void insert_visitor(boost::function<void(T)> f) {
        try {
            fs.insert(std::make_pair(boost::ref(typeid(T)), boost::bind(f, boost::bind(any_cast_f<T>, boost::lambda::_1))));
        } catch (boost::bad_any_cast& e) {
            std::cout << e.what() << std::endl;
        }
    }

    bool operator()(boost::any & x) {
        boost::unordered_map<boost::reference_wrapper<std::type_info const>, boost::function<void(boost::any&)>, type_info_hash, equal_ref>::iterator it = fs.find(boost::ref(x.type()));
        if (it != fs.end()) {
            it->second(x);
            return true;
        } else {
            return false;
        }
    }
};








std::string escaped(const std::string& data)
{
    
    std::string result;
    for (uint8_t c : data)
    {
        switch (c)
        {
            case ('"'): {
                result += "\\\""; break;
            } case ('\\'):  {
                result += "\\\\"; break;
            }  case ('/'):  {
                result += "\\/"; break;
            } case ('\b'):  {
                result += "\\b"; break;
            } case ('\f'):  {
                result += "\\f"; break;
            } case ('\n'):  {
                result += "\\n"; break;
            } case ('\r'):  {
                result += "\\r"; break;
            } case ('\t'):  {
                result += "\\t"; break;
            }
            default: {
                if (!std::isprint(c))
                {
                    result += (boost::format("\\%x") % c).str();
                } else {
                    result.push_back(c);
                }
            }
        }
        
    }
    
    return result;
}


struct json_printer_t{
    typedef json_printer_t self_type;
    json_printer_t(std::ostream& out, int indent_level, bool formatted)
        : out(out), indent_level(indent_level), formatted(formatted)
    {
        f.insert_visitor<bool>(boost::bind(&self_type::bool_stream_it, this, _1));
        f.insert_visitor<int8_t>(boost::bind(&self_type::just_stream_it<int8_t>, this, _1));
        f.insert_visitor<int16_t>(boost::bind(&self_type::just_stream_it<int16_t>, this, _1));
        f.insert_visitor<int32_t>(boost::bind(&self_type::just_stream_it<int32_t>, this, _1));
        f.insert_visitor<int64_t>(boost::bind(&self_type::just_stream_it<int64_t>, this, _1));
        f.insert_visitor<uint8_t>(boost::bind(&self_type::just_stream_it<int8_t>, this, _1));
        f.insert_visitor<uint16_t>(boost::bind(&self_type::just_stream_it<int16_t>, this, _1));
        f.insert_visitor<uint32_t>(boost::bind(&self_type::just_stream_it<int32_t>, this, _1));
        f.insert_visitor<uint64_t>(boost::bind(&self_type::just_stream_it<int64_t>, this, _1));
        
        f.insert_visitor<float>(boost::bind(&self_type::just_stream_it<float>, this, _1));
        f.insert_visitor<double>(boost::bind(&self_type::just_stream_it<double>, this, _1));
        
        f.insert_visitor<const char*>(boost::bind(&self_type::quoted_escape_stream_it, this, _1));
        f.insert_visitor<char*>(boost::bind(&self_type::quoted_escape_stream_it, this, _1));
        //f.insert_visitor<const unsigned char*>(boost::bind(&printer_t::quoted_escape_stream_it, this, _1));
        //f.insert_visitor<unsigned char*>(boost::bind(&printer_t::quoted_escape_stream_it, this, _1));
        f.insert_visitor<std::string>(boost::bind(&self_type::quoted_escape_stream_it, this, _1));
        f.insert_visitor<logdata_t>(boost::bind(&self_type::print, this, _1));
        
    }
    
    void print(const logdata_t& data)
    {
        std::string indentation(4*indent_level,' ');
    
        indent_level++;
        
        
        out << "{";
        
        bool precede_with_comma = false;
        for(auto keyvalue : data)
        {
            auto key = keyvalue.first;
            auto value = keyvalue.second;
            
            if (precede_with_comma)
                out << ",";
            precede_with_comma = true;
            
            if (formatted)
                out << "\n" << indentation;
            
            quoted_escape_stream_it(key); out << ":";
            
            bool success = f(value);
            
            if (!success)
            {
                std::string name = value.type().name();
                throw std::runtime_error(std::string("logstats: invalid type: ") + name);
            }
        }
        
        if (formatted)
            out << "\n";
        out << "}";
        
        indent_level--;
    }
    
    template<typename T>
    void just_stream_it(const T& data)
    {
        out << " " << data;
    }
    
    void bool_stream_it(const bool& data)
    {
        out << (data ? " true" : " false");
    }
    
    void quoted_escape_stream_it(const std::string& data)
    {
        out << " \"" << escaped(data) << "\"";
    }
    
    
    std::ostream& out;
    any_visitor f;
    int indent_level;
    bool formatted;
};

logdata_t merge_logdata(const logdata_t& data0, const logdata_t& data1)
{
    logdata_t result(data0);
    
    result.insert(data1.begin(),data1.end());

    return result;
}

void logstats(const char* type, const logdata_t& data1, const logdata_t& data0)
{
    
    static null_stream static_out;
    static null_stream static_debug;
        
    std::ostream& out = (logstats_out_ptr) ? *logstats_out_ptr : static_out;
    std::ostream& debug = (logstats_debug_ptr) ? *logstats_debug_ptr : static_debug;
    
    
    
    json_printer_t printer(/*stream*/ out,/*indent*/ 1, /*formatted*/ false);
    json_printer_t debug_printer(/*stream*/ debug,/*indent*/ 1, /*formatted*/ false);
    
    std::set<std::string> output_types{
              "SV_THROWNADE", "SV_SHOTFX"
            , "SV_MAPCHANGE", "SV_INITCLIENT"
            , "SV_DAMAGE", "SV_GIBDAMAGE"
            , "SV_DIED", "SV_GIBDIED"
            
            , "SV_RESUME"
            , "SV_SWITCHNAME"
            , "SV_CLIENTPING", "SV_PONG"
            , "SV_TIMEUP"
            , "SV_SERVMSG"
            , "SV_FLAGMSG", "SV_FLAGCNT"
            , "SV_FORCEDEATH"
            , "SV_ARENAWIN"
            
            , "SV_DEMOPLAYBACK"
            , "nextplayback" };
    
    logdata_t data {{"type", type}, {"info",merge_logdata(data0,data1)}};
    if (output_types.count(type) != 0) {
        printer.print(data);
        ///FIXME: maybe should be preceding, to be consistent?
        out << std::endl;
    
    } else {
        debug_printer.print(data);
        debug << std::endl;
    }
}
