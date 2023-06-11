#ifndef HELPERS_HPP
#define HELPERS_HPP
//#include <strTools.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

/**
 * @brief The below Macros print data to the terminal during compilation.
 * !TODO("");
 * !TODO(Variable);
 * !Message("");
 * !Reminder("");
 * !Feature("");
 */
#define Stringize(L) #L
#define MakeString(M, L) M(L)
#define $Line MakeString(Stringize, __LINE__)

#define Reminder __FILE__ "(" $Line ") : Reminder:: "
#define Feature __FILE__ "(" $Line ") : Feature:: "

#define _STR(x) #x
#define STR(x) _STR(x)
#define TODO(x)                                           \
    _Pragma(STR(message("TODO: " STR(x) "::" __FILE__ "@" \
                                        "(" $Line ")")))
#define Message(desc) _Pragma(STR(message(__FILE__ "(" $Line "):" #desc)))

namespace Helpers {
char* itoa(int value, char* result, int base);
void split(const std::string& str, const std::string& splitBy,
           std::vector<std::string>& tokens);
std::vector<std::string> split(const std::string& s, char delimiter);
// char* appendChartoChar(const char* hostname, const char* def_host);
// char* StringtoChar(const std::string& inputString);
void update_progress_bar(int progress, int total);

/// @brief
/// @tparam ...Args
/// @param format
/// @param ...args
/// @return
template <typename... Args>
std::string format_string(const std::string& format, Args... args) {
    int size_s = std::snprintf(nullptr, 0, format.c_str(), args...) +
                 1;  // Extra space for '\0'
    if (size_s <= 0) {
        std::cout << "Error during formatting.";
        return "";
    }
    auto size = static_cast<size_t>(size_s);
    std::unique_ptr<char[]> buf(new char[size]);
    std::snprintf(buf.get(), size, format.c_str(), args...);
    return std::string(buf.get(),
                       buf.get() + size - 1);  // We don't want the '\0' inside
}
}  // namespace Helpers

#endif  // HELPERS_HPP