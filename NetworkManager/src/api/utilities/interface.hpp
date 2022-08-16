#include <string>
#include <map>
#include <vector>
#include <typeinfo>
#include <typeindex>
#include <cassert>

template <typename A>
using voidFunctionType = void (A::*)(void);

template <typename A>
struct Interface
{

    std::map<std::string, std::pair<voidFunctionType<A>, std::type_index>> m1;

    template <typename T>
    void insert(std::string s1, T f1)
    {
        auto tt = std::type_index(typeid(f1));
        m1.insert(std::make_pair(s1,
                                 std::make_pair((voidFunctionType<A>)f1, tt)));
    }

    template <typename T, typename... Args>
    T searchAndCall(A a, std::string s1, Args &&...args)
    {
        auto mapIter = m1.find(s1);
        auto mapVal = mapIter->second;

        auto typeCastedFun = (T(A::*)(Args...))(mapVal.first);

        assert(mapVal.second == std::type_index(typeid(typeCastedFun)));
        return (a.*typeCastedFun)(std::forward<Args>(args)...);
    }
};

// every function pointer will be stored as this type
typedef void (*voidFunctionType_norm)(void);

struct Interface_norm
{

    std::map<std::string, std::pair<voidFunctionType_norm, std::type_index>> m1;

    template <typename T>
    void insert(std::string s1, T f1)
    {
        auto tt = std::type_index(typeid(f1));
        m1.insert(std::make_pair(s1,
                                 std::make_pair((voidFunctionType_norm)f1, tt)));
    }

    template <typename T, typename... Args>
    T searchAndCall(std::string s1, Args &&...args)
    {
        auto mapIter = m1.find(s1);
        /*chk if not end*/
        auto mapVal = mapIter->second;

        // auto typeCastedFun = reinterpret_cast<T(*)(Args ...)>(mapVal.first);
        auto typeCastedFun = (T(*)(Args...))(mapVal.first);

        // compare the types is equal or not
        assert(mapVal.second == std::type_index(typeid(typeCastedFun)));
        return typeCastedFun(std::forward<Args>(args)...);
    }
};