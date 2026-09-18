#ifndef _PLUGINS_ARG_PARSER_H_
#define _PLUGINS_ARG_PARSER_H_
#include "plugin.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace plugins {

class ArgParser : public Plugin<ArgParser> {
  public:
  private:
    std::unordered_map<std::string_view, std::vector<std::string_view>>
        options{};
    std::vector<std::string_view> commands{};
    int m_argc;
    char **m_argv;

  public:
    void parse(int argc, char **argv);
    const std::vector<std::string_view> *getValue(std::string_view key) const;
    const std::vector<std::string_view> &getCommands() const;
    const std::string getCommand() const;
};

}; // namespace plugins

#endif
