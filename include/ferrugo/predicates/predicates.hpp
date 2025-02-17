#pragma once

#include <ferrugo/core/ostream_utils.hpp>
#include <ferrugo/core/source_location.hpp>
#include <ferrugo/core/str_t.hpp>
#include <ferrugo/core/type_traits.hpp>
#include <ferrugo/core/types.hpp>
#include <functional>
#include <optional>
#include <regex>
#include <sstream>
#include <variant>

namespace ferrugo
{
namespace predicates
{

enum class string_comparison
{
    case_sensitive,
    case_insensitive
};

inline std::ostream& operator<<(std::ostream& os, const string_comparison item)
{
    switch (item)
    {
        case string_comparison::case_insensitive: return os << "case_insensitive";
        case string_comparison::case_sensitive: return os << "case_sensitive";
    }
    return os;
}

namespace detail
{

struct unwrap_fn
{
    template <class T>
    auto operator()(T& item) const -> T&
    {
        return item;
    }

    template <class T>
    auto operator()(std::reference_wrapper<T> item) const -> T&
    {
        return item;
    }
};

static constexpr inline auto unwrap = unwrap_fn{};

template <class L, class R>
using is_equality_comparable = decltype(std::declval<L>() == std::declval<R>());

template <class Pred, class T>
constexpr bool invoke_pred(Pred&& pred, T&& item)
{
    if constexpr (std::is_invocable_v<Pred, T>)
    {
        return std::invoke(std::forward<Pred>(pred), std::forward<T>(item));
    }
    else if constexpr (core::is_detected<is_equality_comparable, T, Pred>{})
    {
        return pred == item;
    }
    else
    {
        static_assert(ferrugo::core::always_false<T>::value, "type must be either equality comparable or invocable");
    }
}

struct all_tag
{
};
struct any_tag
{
};

template <class Tag, class Name>
struct compound_fn
{
    template <class... Preds>
    struct impl
    {
        std::tuple<Preds...> m_preds;

        template <class U>
        constexpr bool operator()(U&& item) const
        {
            if constexpr (std::is_same_v<Tag, any_tag>)
            {
                return std::apply(
                    [&](const auto&... preds) { return (... || invoke_pred(preds, std::forward<U>(item))); }, m_preds);
            }
            else if constexpr (std::is_same_v<Tag, all_tag>)
            {
                return std::apply(
                    [&](const auto&... preds) { return (... && invoke_pred(preds, std::forward<U>(item))); }, m_preds);
            }
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            static const auto name = Name{};

            os << "(" << name;
            std::apply(
                [&](const auto&... preds) { ((os << " " << ::ferrugo::core::safe_format(preds)), ...); }, item.m_preds);
            os << ")";
            return os;
        }
    };

    template <class Pipe>
    auto to_tuple(Pipe pipe) const -> std::tuple<Pipe>
    {
        return std::tuple<Pipe>{ std::move(pipe) };
    }

    template <class... Pipes>
    auto to_tuple(impl<Pipes...> pipe) const -> std::tuple<Pipes...>
    {
        return pipe.m_preds;
    }

    template <class... Pipes>
    auto from_tuple(std::tuple<Pipes...> tuple) const -> impl<Pipes...>
    {
        return impl<Pipes...>{ std::move(tuple) };
    }

    template <class... Pipes>
    auto operator()(Pipes... pipes) const -> decltype(from_tuple(std::tuple_cat(to_tuple(std::move(pipes))...)))
    {
        return from_tuple(std::tuple_cat(to_tuple(std::move(pipes))...));
    }
};

struct negate_fn
{
    template <class Pred>
    struct impl
    {
        Pred m_pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return !invoke_pred(m_pred, std::forward<U>(item));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(not " << item.m_pred << ")";
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

struct is_some_fn
{
    template <class Pred>
    struct impl
    {
        Pred m_pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return static_cast<bool>(item) && invoke_pred(m_pred, *std::forward<U>(item));
        }

        bool operator()(nullptr_t) const
        {
            return false;
        }

        bool operator()(std::nullopt_t) const
        {
            return false;
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(is_some " << item.m_pred << ")";
        }
    };

    struct void_impl
    {
        template <class U>
        bool operator()(U&& item) const
        {
            return static_cast<bool>(item);
        }

        bool operator()(nullptr_t) const
        {
            return false;
        }

        bool operator()(std::nullopt_t) const
        {
            return false;
        }

        friend std::ostream& operator<<(std::ostream& os, const void_impl& item)
        {
            return os << "(is_some)";
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }

    auto operator()() const -> void_impl
    {
        return void_impl{};
    }
};

struct is_none_fn
{
    struct impl
    {
        template <class U>
        bool operator()(U&& item) const
        {
            return !static_cast<bool>(item);
        }

        bool operator()(nullptr_t) const
        {
            return true;
        }

        bool operator()(std::nullopt_t) const
        {
            return true;
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(is_none)";
        }
    };

    auto operator()() const -> impl
    {
        return impl{};
    }
};

template <class Op, class Name>
struct compare_fn
{
    template <class T>
    struct impl
    {
        T m_value;

        template <class U>
        bool operator()(U&& item) const
        {
            static const auto op = Op{};
            return op(std::forward<U>(item), m_value);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            static const auto name = Name{};
            return os << "(" << name << " " << item.m_value << ")";
        }
    };

    template <class T>
    auto operator()(T&& value) const -> impl<std::decay_t<T>>
    {
        return impl<std::decay_t<T>>{ std::forward<T>(value) };
    }
};

struct size_is_fn
{
    template <class Pred>
    struct impl
    {
        Pred m_pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return invoke_pred(m_pred, std::distance(std::begin(item), std::end(item)));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(size_is " << item.m_pred << ")";
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

struct is_empty_fn
{
    struct impl
    {
        template <class U>
        bool operator()(U&& item) const
        {
            return std::begin(item) == std::end(item);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(is_empty)";
        }
    };

    auto operator()() const -> impl
    {
        return impl{};
    }
};

struct each_item_fn
{
    template <class Pred>
    struct impl
    {
        Pred m_pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return std::all_of(
                std::begin(item),
                std::end(item),
                [&](auto&& v) { return invoke_pred(m_pred, std::forward<decltype(v)>(v)); });
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(each_item " << item.m_pred << ")";
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

struct contains_item_fn
{
    template <class Pred>
    struct impl
    {
        Pred m_pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return std::any_of(
                std::begin(item),
                std::end(item),
                [&](auto&& v) { return invoke_pred(m_pred, std::forward<decltype(v)>(v)); });
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(contains_item " << item.m_pred << ")";
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

struct items_are
{
};

struct items_are_fn
{
    template <std::size_t N = 0, class... Preds, class Iter>
    static bool call(const std::tuple<Preds...>& preds, Iter begin, Iter end)
    {
        if constexpr (N == sizeof...(Preds))
        {
            return begin == end;
        }
        else
        {
            return begin != end && invoke_pred(std::get<N>(preds), *begin) && call<N + 1>(preds, std::next(begin), end);
        }
    }
    template <class... Preds>
    struct impl
    {
        std::tuple<Preds...> m_preds;

        template <class U>
        bool operator()(U&& item) const
        {
            return call(m_preds, std::begin(item), std::end(item));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            os << "("
               << "items_are";
            std::apply(
                [&](const auto&... preds) { ((os << " " << ::ferrugo::core::safe_format(preds)), ...); }, item.m_preds);
            os << ")";
            return os;
        }
    };

    template <class... Preds>
    auto operator()(Preds&&... preds) const -> impl<std::decay_t<Preds>...>
    {
        return impl<std::decay_t<Preds>...>{ { std::forward<Preds>(preds)... } };
    }
};

struct items_are_array_fn
{
    template <class PIter, class Iter>
    static bool call(PIter p_b, PIter p_e, Iter begin, Iter end)
    {
        return std::equal(p_b, p_e, begin, end, [](auto&& p, auto&& it) { return invoke_pred(p, it); });
    }
    template <class Range>
    struct impl
    {
        Range m_range;

        template <class U>
        bool operator()(U&& item) const
        {
            return call(std::begin(unwrap(m_range)), std::end(unwrap(m_range)), std::begin(item), std::end(item));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "("
                      << "items_are_array " << ::ferrugo::core::safe_format(item.m_range) << ")";
        }
    };

    template <class Range>
    auto operator()(Range range) const -> impl<Range>
    {
        return impl<Range>{ std::move(range) };
    }
};

struct starts_with_items_fn
{
    template <class... Preds>
    struct impl
    {
        std::tuple<Preds...> m_preds;

        template <class U>
        bool operator()(U&& item) const
        {
            const auto b = std::begin(unwrap(item));
            const auto e = std::end(unwrap(item));
            const auto preds_count = sizeof...(Preds);
            const auto size = std::distance(b, e);
            return size >= preds_count && items_are_fn ::call(m_preds, b, std::next(b, preds_count));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            os << "("
               << "starts_with_items";
            std::apply(
                [&](const auto&... preds) { ((os << " " << ::ferrugo::core::safe_format(preds)), ...); }, item.m_preds);
            os << ")";
            return os;
        }
    };

    template <class... Preds>
    auto operator()(Preds&&... preds) const -> impl<std::decay_t<Preds>...>
    {
        return impl<std::decay_t<Preds>...>{ { std::forward<Preds>(preds)... } };
    }
};

struct starts_with_array_fn
{
    template <class Range>
    struct impl
    {
        Range m_range;

        template <class U>
        bool operator()(U&& item) const
        {
            const auto p_b = std::begin(unwrap(m_range));
            const auto p_e = std::end(unwrap(m_range));
            const auto b = std::begin(unwrap(item));
            const auto e = std::end(unwrap(item));
            const auto preds_count = std::distance(p_b, p_e);
            const auto size = std::distance(b, e);
            return size >= preds_count && items_are_array_fn::call(p_b, p_e, b, std::next(b, preds_count));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "("
                      << "starts_with_array " << ::ferrugo::core::safe_format(item.m_range) << ")";
        }
    };

    template <class Range>
    auto operator()(Range range) const -> impl<Range>
    {
        return impl<Range>{ std::move(range) };
    }
};

struct ends_with_items_fn
{
    template <class... Preds>
    struct impl
    {
        std::tuple<Preds...> m_preds;

        template <class U>
        bool operator()(U&& item) const
        {
            const auto b = std::begin(unwrap(item));
            const auto e = std::end(unwrap(item));
            const auto preds_count = sizeof...(Preds);
            const auto size = std::distance(b, e);
            return size >= preds_count && items_are_fn::call(m_preds, std::next(b, size - preds_count), e);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            os << "("
               << "ends_with_items";
            std::apply(
                [&](const auto&... preds) { ((os << " " << ::ferrugo::core::safe_format(preds)), ...); }, item.m_preds);
            os << ")";
            return os;
        }
    };

    template <class... Preds>
    auto operator()(Preds&&... preds) const -> impl<std::decay_t<Preds>...>
    {
        return impl<std::decay_t<Preds>...>{ { std::forward<Preds>(preds)... } };
    }
};

struct ends_with_array_fn
{
    template <class Range>
    struct impl
    {
        Range m_range;

        template <class U>
        bool operator()(U&& item) const
        {
            const auto p_b = std::begin(unwrap(m_range));
            const auto p_e = std::end(unwrap(m_range));
            const auto b = std::begin(unwrap(item));
            const auto e = std::end(unwrap(item));
            const auto preds_count = std::distance(p_b, p_e);
            const auto size = std::distance(b, e);
            return size >= preds_count && items_are_array_fn::call(p_b, p_e, std::next(b, size - preds_count), e);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "("
                      << "ends_with_array " << ::ferrugo::core::safe_format(item.m_range) << ")";
        }
    };

    template <class Range>
    auto operator()(Range range) const -> impl<Range>
    {
        return impl<Range>{ std::move(range) };
    }
};

struct contains_items_fn
{
    template <class... Preds>
    struct impl
    {
        std::tuple<Preds...> m_preds;

        template <class U>
        bool operator()(U&& item) const
        {
            const auto b = std::begin(unwrap(item));
            const auto e = std::end(unwrap(item));
            const auto preds_count = sizeof...(Preds);
            const auto size = std::distance(b, e);
            if (size < preds_count)
            {
                return false;
            }
            for (std::size_t i = 0; i < size - preds_count + 1; ++i)
            {
                if (items_are_fn::call(m_preds, std::next(b, i), std::next(b, i + preds_count)))
                {
                    return true;
                }
            }
            return false;
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            os << "("
               << "contains_items";
            std::apply(
                [&](const auto&... preds) { ((os << " " << ::ferrugo::core::safe_format(preds)), ...); }, item.m_preds);
            os << ")";
            return os;
        }
    };

    template <class... Preds>
    auto operator()(Preds&&... preds) const -> impl<std::decay_t<Preds>...>
    {
        return impl<std::decay_t<Preds>...>{ { std::forward<Preds>(preds)... } };
    }
};

struct contains_array_fn
{
    template <class Range>
    struct impl
    {
        Range m_range;

        template <class U>
        bool operator()(U&& item) const
        {
            const auto p_b = std::begin(unwrap(m_range));
            const auto p_e = std::end(unwrap(m_range));
            const auto b = std::begin(unwrap(item));
            const auto e = std::end(unwrap(item));
            const auto preds_count = std::distance(p_b, p_e);
            const auto size = std::distance(b, e);
            if (size < preds_count)
            {
                return false;
            }
            for (std::size_t i = 0; i < size - preds_count + 1; ++i)
            {
                if (items_are_array_fn::call(p_b, p_e, std::next(b, i), std::next(b, i + preds_count)))
                {
                    return true;
                }
            }
            return false;
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "("
                      << "contains_array " << ::ferrugo::core::safe_format(item.m_range) << ")";
        }
    };

    template <class Range>
    auto operator()(Range range) const -> impl<Range>
    {
        return impl<Range>{ std::move(range) };
    }
};

template <class Name>
struct result_of_fn
{
    template <class Func, class Pred>
    struct impl
    {
        Func m_func;
        Pred m_pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return invoke_pred(m_pred, std::invoke(m_func, std::forward<U>(item)));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            static const auto name = Name{};
            return os << "(" << name << " " << ::ferrugo::core::safe_format(item.m_func) << " "
                      << ::ferrugo::core::safe_format(item.m_pred) << ")";
        }
    };

    template <class Func, class Pred>
    auto operator()(Func&& func, Pred&& pred) const -> impl<std::decay_t<Func>, std::decay_t<Pred>>
    {
        return { std::forward<Func>(func), std::forward<Pred>(pred) };
    }
};

struct approx_eq_fn
{
    template <class T>
    struct impl
    {
        T m_value;

        template <class U>
        bool operator()(U&& item) const
        {
            return std::abs(item - m_value) < std::numeric_limits<T>::epsilon();
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(approx_eq " << item.m_value << ")";
        }
    };

    template <class T>
    auto operator()(T value) const -> impl<T>
    {
        return impl{ value };
    }
};

struct is_divisible_by_fn
{
    struct impl
    {
        int m_divisor;

        template <class T>
        bool operator()(T&& item) const
        {
            return item % m_divisor == 0;
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(is_divisible_by " << item.m_divisor << ")";
        }
    };

    auto operator()(int divisor) const -> impl
    {
        return impl{ divisor };
    }
};

struct is_even_fn
{
    struct impl
    {
        template <class T>
        bool operator()(T&& item) const
        {
            return item % 2 == 0;
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(is_even)";
        }
    };

    auto operator()() const -> impl
    {
        return impl{};
    }
};

struct is_odd_fn
{
    struct impl
    {
        template <class T>
        bool operator()(T&& item) const
        {
            return item % 2 != 0;
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(is_odd)";
        }
    };

    auto operator()() const -> impl
    {
        return impl{};
    }
};

struct is_digit_fn
{
    struct impl
    {
        bool operator()(char item) const
        {
            return std::isdigit(item);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(is_digit)";
        }
    };

    auto operator()() const -> impl
    {
        return impl{};
    }
};
struct is_space_fn
{
    struct impl
    {
        bool operator()(char item) const
        {
            return std::isspace(item);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(is_space)";
        }
    };

    auto operator()() const -> impl
    {
        return impl{};
    }
};
struct is_alnum_fn
{
    struct impl
    {
        bool operator()(char item) const
        {
            return std::isalnum(item);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(is_alnum)";
        }
    };

    auto operator()() const -> impl
    {
        return impl{};
    }
};
struct is_alpha_fn
{
    struct impl
    {
        bool operator()(char item) const
        {
            return std::isalpha(item);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(is_alpha)";
        }
    };

    auto operator()() const -> impl
    {
        return impl{};
    }
};
struct is_upper_fn
{
    struct impl
    {
        bool operator()(char item) const
        {
            return std::isupper(item);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(is_upper)";
        }
    };

    auto operator()() const -> impl
    {
        return impl{};
    }
};
struct is_lower_fn
{
    struct impl
    {
        bool operator()(char item) const
        {
            return std::islower(item);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(is_lower)";
        }
    };

    auto operator()() const -> impl
    {
        return impl{};
    }
};

template <std::size_t N>
struct field_at_fn
{
    template <class Pred>
    struct impl
    {
        Pred pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return invoke_pred(pred, std::get<N>(item));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(element " << N << " " << item.pred << ")";
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

struct fields_are_fn
{
    template <class... Preds>
    struct impl
    {
        std::tuple<Preds...> m_preds;

        template <class U>
        bool operator()(U&& item) const
        {
            return call(std::forward<U>(item), std::index_sequence_for<Preds...>{});
        }

        template <class U, std::size_t... I>
        bool call(U&& item, std::index_sequence<I...>) const
        {
            return (invoke_pred(std::get<I>(m_preds), std::get<I>(std::forward<U>(item))) && ...);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            os << "("
               << "elements_are";
            std::apply(
                [&](const auto&... preds) { ((os << " " << ::ferrugo::core::safe_format(preds)), ...); }, item.m_preds);
            os << ")";
            return os;
        }
    };

    template <class... Preds>
    auto operator()(Preds&&... preds) const -> impl<std::decay_t<Preds>...>
    {
        return impl<std::decay_t<Preds>...>{ { std::forward<Preds>(preds)... } };
    }
};

template <class T>
struct variant_with_fn
{
    template <class Pred>
    struct impl
    {
        Pred pred;

        template <class U>
        bool operator()(U&& item) const
        {
            const auto ptr = std::get_if<T>(&item);
            return ptr && invoke_pred(pred, *ptr);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(variant_with " << core::type_name<T>() << " " << item.pred << ")";
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

inline auto compare_characters(string_comparison comparison) -> std::function<bool(char, char)>
{
    static const auto to_lower = [](char ch) -> char { return std::tolower(ch); };
    switch (comparison)
    {
        case string_comparison::case_sensitive: return [](char lt, char rt) { return lt == rt; };
        case string_comparison::case_insensitive: return [](char lt, char rt) { return to_lower(lt) == to_lower(rt); };
    }
    throw std::runtime_error{ "unhandled comparison mode" };
}

struct string_is_fn
{
    struct impl
    {
        std::string m_expected;
        string_comparison m_comparison;

        bool operator()(std::string_view actual) const
        {
            return std::equal(
                std::begin(actual),
                std::end(actual),
                std::begin(m_expected),
                std::end(m_expected),
                compare_characters(m_comparison));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(string_is " << item.m_comparison << " \"" << item.m_expected << "\")";
        }
    };

    auto operator()(std::string expected, string_comparison comparison) const
    {
        return impl{ std::move(expected), comparison };
    }
};

struct string_starts_with_fn
{
    struct impl
    {
        std::string m_expected;
        string_comparison m_comparison;

        bool operator()(std::string_view actual) const
        {
            return actual.size() >= m_expected.size()
                   && std::equal(
                       std::begin(actual),
                       std::next(std::begin(actual), m_expected.size()),
                       std::begin(m_expected),
                       std::end(m_expected),
                       compare_characters(m_comparison));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(string_starts_with " << item.m_comparison << " \"" << item.m_expected << "\")";
        }
    };

    auto operator()(std::string expected, string_comparison comparison) const
    {
        return impl{ std::move(expected), comparison };
    }
};

struct string_ends_with_fn
{
    struct impl
    {
        std::string m_expected;
        string_comparison m_comparison;

        bool operator()(std::string_view actual) const
        {
            return actual.size() >= m_expected.size()
                   && std::equal(
                       std::next(std::begin(actual), actual.size() - m_expected.size()),
                       std::end(actual),
                       std::begin(m_expected),
                       std::end(m_expected),
                       compare_characters(m_comparison));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(string_ends_with " << item.m_comparison << " \"" << item.m_expected << "\")";
        }
    };

    auto operator()(std::string expected, string_comparison comparison) const
    {
        return impl{ std::move(expected), comparison };
    }
};

struct string_contains_fn
{
    struct impl
    {
        std::string m_expected;
        string_comparison m_comparison;

        bool operator()(std::string_view actual) const
        {
            return std::search(
                       std::begin(actual),
                       std::end(actual),
                       std::begin(m_expected),
                       std::end(m_expected),
                       compare_characters(m_comparison))
                   != std::end(actual);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(string_contains " << item.m_comparison << " \"" << item.m_expected << "\")";
        }
    };

    auto operator()(std::string expected, string_comparison comparison) const
    {
        return impl{ std::move(expected), comparison };
    }
};

struct string_matches_fn
{
    struct impl
    {
        std::regex m_regex;

        bool operator()(std::string_view actual) const
        {
            return std::regex_match(actual.begin(), actual.end(), m_regex);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(string_matches)";
        }
    };

    auto operator()(std::regex regex) const
    {
        return impl{ std::move(regex) };
    }

    auto operator()(const std::string& regex) const
    {
        return (*this)(std::regex(regex));
    }
};

}  // namespace detail

template <class T>
struct predicate : std::function<bool(::ferrugo::core::in_t<T>)>
{
    using base_t = std::function<bool(::ferrugo::core::in_t<T>)>;
    using base_t::base_t;

    friend std::ostream& operator<<(std::ostream& os, const predicate& item)
    {
        return os << "predicate<" << ::ferrugo::core::type_name<T>() << ">";
    }
};

struct assertion_error : std::runtime_error
{
    explicit assertion_error(std::string msg) : std::runtime_error(std::move(msg))
    {
    }
};

template <class T, class Pred>
void assert_that(const T& item, const Pred& pred, const std::optional<::ferrugo::core::source_location>& loc = {})
{
    if (pred(item))
    {
        return;
    }
    std::stringstream ss;
    ss << "assertion failed:" << '\n';
    ss << "value: " << ::ferrugo::core::safe_format(item) << '\n';
    ss << "does not match the predicate: " << ::ferrugo::core::safe_format(pred) << '\n';
    if (loc)
    {
        ss << "at " << *loc << "\n";
    }
    throw assertion_error{ ss.str() };
}

static constexpr inline auto any = detail::compound_fn<detail::any_tag, FERRUGO_STR_T("any")>{};
static constexpr inline auto all = detail::compound_fn<detail::all_tag, FERRUGO_STR_T("all")>{};
static constexpr inline auto negate = detail::negate_fn{};

static constexpr inline auto is_some = detail::is_some_fn{};
static constexpr inline auto is_none = detail::is_none_fn{};

static constexpr inline auto each_item = detail::each_item_fn{};
static constexpr inline auto contains_item = detail::contains_item_fn{};
static constexpr inline auto size_is = detail::size_is_fn{};
static constexpr inline auto is_empty = detail::is_empty_fn{};

static constexpr inline auto items_are = detail::items_are_fn{};
static constexpr inline auto items_are_array = detail::items_are_array_fn{};
static constexpr inline auto starts_with_items = detail::starts_with_items_fn{};
static constexpr inline auto starts_with_array = detail::starts_with_array_fn{};
static constexpr inline auto ends_with_items = detail::ends_with_items_fn{};
static constexpr inline auto ends_with_array = detail::ends_with_array_fn{};
static constexpr inline auto contains_items = detail::contains_items_fn{};
static constexpr inline auto contains_array = detail::contains_array_fn{};

static constexpr inline auto string_is = detail::string_is_fn{};
static constexpr inline auto string_starts_with = detail::string_starts_with_fn{};
static constexpr inline auto string_ends_with = detail::string_ends_with_fn{};
static constexpr inline auto string_contains = detail::string_contains_fn{};
static constexpr inline auto string_matches = detail::string_matches_fn{};

static constexpr inline auto eq = detail::compare_fn<std::equal_to<>, FERRUGO_STR_T("eq")>{};
static constexpr inline auto ne = detail::compare_fn<std::not_equal_to<>, FERRUGO_STR_T("ne")>{};
static constexpr inline auto lt = detail::compare_fn<std::less<>, FERRUGO_STR_T("lt")>{};
static constexpr inline auto gt = detail::compare_fn<std::greater<>, FERRUGO_STR_T("gt")>{};
static constexpr inline auto le = detail::compare_fn<std::less_equal<>, FERRUGO_STR_T("le")>{};
static constexpr inline auto ge = detail::compare_fn<std::greater_equal<>, FERRUGO_STR_T("ge")>{};

static constexpr inline auto approx_eq = detail::approx_eq_fn{};
static constexpr inline auto is_divisible_by = detail::is_divisible_by_fn{};
static constexpr inline auto is_odd = detail::is_odd_fn{};
static constexpr inline auto is_even = detail::is_even_fn{};

static constexpr inline auto result_of = detail::result_of_fn<FERRUGO_STR_T("result_of")>{};
static constexpr inline auto field = detail::result_of_fn<FERRUGO_STR_T("field")>{};
static constexpr inline auto property = detail::result_of_fn<FERRUGO_STR_T("property")>{};

static constexpr inline auto is_space = detail::is_space_fn{};
static constexpr inline auto is_digit = detail::is_digit_fn{};
static constexpr inline auto is_alnum = detail::is_alnum_fn{};
static constexpr inline auto is_alpha = detail::is_alpha_fn{};
static constexpr inline auto is_upper = detail::is_upper_fn{};
static constexpr inline auto is_lower = detail::is_lower_fn{};

template <std::size_t N>
static constexpr inline auto element = detail::field_at_fn<N>{};

static constexpr inline auto elements_are = detail::fields_are_fn{};

template <class T>
static constexpr inline auto variant_with = detail::variant_with_fn<T>{};

}  // namespace predicates
}  // namespace ferrugo
