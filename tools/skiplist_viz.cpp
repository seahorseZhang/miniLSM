#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "minilsm/skiplist.h"

using namespace minilsm;

static void print_help() {
    std::cout << "Commands:\n";
    std::cout << "  insert <key> <value>\n";
    std::cout << "  update <key> <value>\n";
    std::cout << "  erase <key>\n";
    std::cout << "  find <key>\n";
    std::cout << "  dump <file.dot>\n";
    std::cout << "  exit\n";
}

int main(int argc, char** argv) {
    SkipList<std::string,std::string> sl(16, 0.5);

    if (argc > 1) {
        // If a script file is provided, run commands from it
        std::ifstream in(argv[1]);
        if (!in) { std::cerr << "Cannot open command file: " << argv[1] << "\n"; return 1; }
        std::string line;
        while (std::getline(in, line)) {
            std::istringstream iss(line);
            std::string cmd; iss >> cmd;
            if (!iss) continue;
            if (cmd == "insert") {
                std::string k, v; iss >> k >> v; sl.insert(k,v); }
            else if (cmd == "update") { std::string k,v; iss>>k>>v; sl.update(k,v); }
            else if (cmd == "erase") { std::string k; iss>>k; sl.erase(k); }
            else if (cmd == "dump") { std::string fn; iss>>fn; std::ofstream out(fn); out << sl.to_dot(); }
        }
        return 0;
    }

    std::cout << "SkipList Visualizer (interactive). Type 'help' for commands.\n";
    print_help();
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;
        std::istringstream iss(line);
        std::string cmd; iss >> cmd;
        if (!iss) continue;
        if (cmd == "help") print_help();
        else if (cmd == "insert") {
            std::string k,v; iss >> k >> v; if (sl.insert(k,v)) std::cout<<"OK\n"; else std::cout<<"exists\n"; }
        else if (cmd == "update") { std::string k,v; iss>>k>>v; if (sl.update(k,v)) std::cout<<"OK\n"; else std::cout<<"not found\n"; }
        else if (cmd == "erase") { std::string k; iss>>k; if (sl.erase(k)) std::cout<<"OK\n"; else std::cout<<"not found\n"; }
        else if (cmd == "find") { std::string k; iss>>k; auto r=sl.find(k); if (r) std::cout<<r.value()<<"\n"; else std::cout<<"not found\n"; }
        else if (cmd == "dump") { std::string fn; iss>>fn; std::ofstream out(fn); out << sl.to_dot(); std::cout<<"Wrote "<<fn<<"\n"; }
        else if (cmd == "exit") break;
        else std::cout<<"Unknown command\n";
    }

    return 0;
}
