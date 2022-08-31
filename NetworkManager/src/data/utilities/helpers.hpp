#include <strTools.h>
#include <string>
#include <sstream>
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
#define TODO(x) _Pragma(STR(message("TODO: " STR(x) "::" __FILE__ "@" \
                                                    "(" $Line ")")))
#define Message(desc) _Pragma(STR(message(__FILE__ "(" $Line "):" #desc)))

namespace Helpers
{
    char *itoa(int value, char *result, int base);
    void split(std::string str, std::string splitBy, std::vector<std::string> &tokens);
    std::vector<std::string> split(const std::string &s, char delimiter);
    char *appendChartoChar(const char *hostname, const char *def_host);
    char *StringtoChar(const std::string &inputString);
}