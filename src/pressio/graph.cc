#include <libpressio_ext/cpp/libpressio.h>
#include <libpressio_meta.h>
#include <map>
#include <string_view>
#include <iostream>
#include <iomanip>
#include <functional>
#include <regex>
#include <unistd.h>
std::string get_or_default(std::map<std::string, std::string>const& m, std::string const& v, std::string const& default_value) {
    if(auto it = m.find(v); it != m.end()) {
        return it->second;
    }
    return default_value;
}
std::string to_id(std::map<std::string, std::string> &m, std::string const& name) {
    if(auto it = m.find(name); it != m.end()){
        return it->second;
    }
    return m[name] = std::to_string(m.size());
}

struct graph_printer {
    virtual ~graph_printer()=default;
    virtual void add_edge(std::string parent_id, std::string child_id)=0;
    virtual void add_node(std::string node_id, std::vector<std::string> node_text, std::string node_type)=0;
};
struct graphviz_printer: public graph_printer {
    static const std::map<std::string, std::string> shapes;
    graphviz_printer(std::ostream& out): out(out) {
        out << "digraph {" << std::endl;
    }
    ~graphviz_printer() {
        out << "}" << std::endl;
    }
    graphviz_printer(): graphviz_printer(std::cout) {};
    void add_edge(std::string parent_id, std::string child_id) override {
        out << parent_id << " -> " << child_id << std::endl;
    }
    void add_node(std::string node_id, std::vector<std::string> node_text, std::string node_type) override {
        std::stringstream ss;
        ss << node_text[0];
        for (size_t i = 1; i < node_text.size(); ++i) {
           ss << "\n" << node_text[i]; 
        }
        out << node_id << "[shape=" << get_or_default(shapes, node_type, "ellipse") << ",label=" << std::quoted(ss.str()) << "]" << std::endl;
    }
    std::ostream& out;
};
const std::map<std::string, std::string> graphviz_printer::shapes {
   {"compressor", "ellipse"},
   {"metric", "box"},
   {"launch", "house"},
   {"search", "diamond"},
   {"search_metric", "parallelogram"},
   {"io", "hexagon"},
   {"dataset", "octogon"},
};
struct d2_printer: public graph_printer {
    static const std::map<std::string, std::string> shapes;
    d2_printer(std::ostream& out): out(out) {
        out << "direction: down" << std::endl;
    }
    d2_printer(): d2_printer(std::cout) {};
    ~d2_printer() {}
    void add_edge(std::string parent_id, std::string child_id) override {
        out << parent_id << " -> " << child_id << std::endl;
    }
    void add_node(std::string node_id, std::vector<std::string> node_text, std::string node_type) override {
        std::stringstream ss;
        ss << node_text[0];
        for (size_t i = 1; i < node_text.size(); ++i) {
           ss << "\\n" << node_text[i]; 
        }
        out << node_id << ": {shape:" << get_or_default(shapes, node_type, "ellipse") << "\nlabel:" << ss.str() << "}" << std::endl;
    }
    std::ostream& out;
};
const std::map<std::string, std::string> d2_printer::shapes {
   {"compressor", "oval"},
   {"metric", "square"},
   {"launch", "page"},
   {"search", "circle"},
   {"search_metric", "hexagon"},
   {"io", "cylinder"},
   {"dataset", "cloud"},
};
std::unique_ptr<graph_printer> make_printer(std::string const& s, std::ostream& out = std::cout) {
    if(s == "d2") return std::make_unique<d2_printer>(out);
    else if(s == "graphviz") return std::make_unique<graphviz_printer>(out);
    else return std::unique_ptr<graph_printer>();
}

bool starts_with(std::string const& base, std::string const& pattern) {
    return base.find(pattern) == 0;
}

void print_graph(pressio_compressor const& comp, std::string printer_format) {
    if(comp->get_name() == "") {
        std::cout << "fully quallify mode is required for graph mode" << std::endl;
        exit(1);
    }
    auto config = comp->get_configuration();
    auto options = comp->get_options();
    std::map<std::string, std::string> name_to_id;
    bool verbose_options = false;

    std::regex child_regex{"/([^:]+):pressio:children"};
    std::regex type_regex{"/([^:]+):pressio:type"};
    std::regex prefix_regex{"/([^:]+):pressio:prefix"};
    std::regex version_regex{"/([^:]+):pressio:version"};
    std::smatch match;

    auto printer = make_printer(printer_format);
    if(!printer) {
        std::cerr << "unknown printer format " << printer_format << std::endl;
        exit(1);
    }

    //collect metadata
    std::map<std::string, std::string> version, prefix;
    for (auto const& c : config) {
        if(std::regex_match(c.first, match, prefix_regex)) {
            prefix[match[1].str()] = c.second.get_value<std::string>();
        }
        if(std::regex_match(c.first, match, version_regex)) {
            version[match[1].str()] = c.second.get_value<std::string>();
        }
    }

    //build graph
    for (auto const& c : config) {
        if(std::regex_match(c.first, match, child_regex)) {
            std::string parent_node_id = to_id(name_to_id, match[1].str());
            for(auto const& child: c.second.get_value<std::vector<std::string>>()) {
                std::string child_node_id = to_id(name_to_id, child);
                printer->add_edge(parent_node_id, child_node_id);
            }
        }
        if(std::regex_match(c.first, match, type_regex)) {
            std::string name = match[1].str();
            std::string node_id = to_id(name_to_id, name);
            std::vector<std::string> labels {name , prefix[name] + '@' + version[name]};
            if(verbose_options) {
                for (auto const& o : options) {
                    if(starts_with(o.first, "/" + name + ":")) {
                        std::stringstream ss;
                        ss << o.first.substr(o.first.find(':')+1);
                        auto option_str =  o.second.as(pressio_option_charptr_type, pressio_conversion_special);
                        if(option_str.has_value()) {
                            ss << '=' << option_str.get_value<std::string>();
                        }
                        labels.emplace_back(ss.str());
                    }
                }
            }
            printer->add_node(node_id, labels, c.second.get_value<std::string>());
        }
    }
}
