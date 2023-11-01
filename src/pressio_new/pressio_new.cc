#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include "templates.h"

std::map<std::string, std::string> const templates {
    {"compressor", compressor_template},
    {"metric", metric_template},
    {"io", io_template},
    {"search", search_template},
    {"searchmetric", searchmetric_template},
    {"dataset", dataset_template},
    {"client:cpp", cpp_client},
    {"client:c", c_client},
    {"client:python", python_client},
};

std::size_t replace_all(std::string& inout, std::string what, std::string with)
{
    std::size_t count{};
    for (std::string::size_type pos{};
         inout.npos != (pos = inout.find(what.data(), pos, what.length()));
         pos += with.length(), ++count)
        inout.replace(pos, what.length(), with.data(), with.length());
    return count;
}


int main(int argc, char *argv[])
{
    try {
        if(argc < 2) { 
            std::ostringstream usage;
            usage << "usage: ./pressio_new [template] [arg1...]" << std::endl;
            for (auto const& t : templates) {
               usage << t.first << std::endl; 
            }
            throw std::runtime_error(usage.str());
        }

        std::string template_id  = argv[1];
        std::vector<std::string> replacements(argv+2, argv+argc);
        auto content = templates.at(template_id);
        for (int i = 0; i < argc-2; ++i) {
            std::string pattern = "$" + std::to_string(i+1);
            replace_all(content, pattern, replacements.at(i));
        }
        std::cout << content << std::endl;
    } catch(std::exception const& ex) {
        std::cerr << ex.what() << std::endl;
    }
    return 0;
}
