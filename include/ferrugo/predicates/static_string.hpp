#pragma once

#include <string>
#include <string_view>

namespace ferrugo
{
namespace predicates
{

template <char... Ch>
struct static_string
{
    static std::string_view str()
    {
        static const auto instance = std::string{ Ch... };
        return instance;
    }

    operator std::string_view() const
    {
        return str();
    }

    friend std::ostream& operator<<(std::ostream& os, const static_string)
    {
        return os << str();
    }
};

template <class... Args>
struct concat_result;

template <class... Args>
using concat_result_t = typename concat_result<Args...>::type;

template <class T>
struct concat_result<T>
{
    using type = T;
};

template <char... L, char... R>
struct concat_result<static_string<L...>, static_string<R...>>
{
    using type = static_string<L..., R...>;
};

template <class A, class B, class... Tail>
struct concat_result<A, B, Tail...>
{
    using type = concat_result_t<concat_result_t<A, B>, concat_result_t<Tail...>>;
};

template <class T>
struct trim_result
{
    using type = T;
};

template <class T>
using trim_result_t = typename trim_result<T>::type;

template <char L, char... R>
struct trim_result<static_string<L, R...>>
{
    using type = concat_result_t<static_string<L>, trim_result_t<static_string<R...>>>;
};

template <char... Tail>
struct trim_result<static_string<'\0', Tail...>>
{
    using type = static_string<>;
};

}  // namespace predicates
}  // namespace ferrugo

#define FERRUGO_GET_SS_1(str, i) ((i) + 1 < sizeof(str) ? str[(i)] : '\0')

#define FERRUGO_GET_SS_4(str, i) \
    FERRUGO_GET_SS_1(str, i + 0), FERRUGO_GET_SS_1(str, i + 1), FERRUGO_GET_SS_1(str, i + 2), FERRUGO_GET_SS_1(str, i + 3)

#define FERRUGO_GET_SS_16(str, i) \
    FERRUGO_GET_SS_4(str, i + 0), FERRUGO_GET_SS_4(str, i + 4), FERRUGO_GET_SS_4(str, i + 8), FERRUGO_GET_SS_4(str, i + 12)

#define FERRUGO_GET_SS_64(str, i)                                                                  \
    FERRUGO_GET_SS_16(str, i + 0), FERRUGO_GET_SS_16(str, i + 16), FERRUGO_GET_SS_16(str, i + 32), \
        FERRUGO_GET_SS_16(str, i + 48)

#define FERRUGO_GET_SS(str) FERRUGO_GET_SS_64(str, 0)

#define FERRUGO_STATIC_STRING(str) trim_result_t<string_t<FERRUGO_GET_SS(str)>>
