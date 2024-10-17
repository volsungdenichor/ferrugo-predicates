#pragma once

#include <ferrugo/core/ostream_utils.hpp>
#include <ferrugo/core/source_location.hpp>
#include <ferrugo/core/type_traits.hpp>
#include <ferrugo/core/types.hpp>
#include <ferrugo/predicates/static_string.hpp>
#include <functional>
#include <optional>
#include <sstream>
#include <variant>

namespace ferrugo
{
namespace predicates
{

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
        Pred pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return !invoke_pred(pred, std::forward<U>(item));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(not " << item.pred << ")";
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
        Pred pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return static_cast<bool>(item) && invoke_pred(pred, *std::forward<U>(item));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(is_some " << item.pred << ")";
        }
    };

    struct void_impl
    {
        template <class U>
        bool operator()(U&& item) const
        {
            return static_cast<bool>(item);
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

template <class Op, class Name>
struct compare_fn
{
    template <class T>
    struct impl
    {
        T value;

        template <class U>
        bool operator()(U&& item) const
        {
            static const auto op = Op{};
            return op(std::forward<U>(item), value);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            static const auto name = Name{};
            return os << "(" << name << " " << item.value << ")";
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
        Pred pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return invoke_pred(pred, std::distance(std::begin(item), std::end(item)));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(size_is " << item.pred << ")";
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

struct each_fn
{
    template <class Pred>
    struct impl
    {
        Pred pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return std::all_of(
                std::begin(item), std::end(item), [&](auto&& v) { return invoke_pred(pred, std::forward<decltype(v)>(v)); });
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(each " << item.pred << ")";
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

struct contains_fn
{
    template <class Pred>
    struct impl
    {
        Pred pred;

        template <class U>
        bool operator()(U&& item) const
        {
            return std::any_of(
                std::begin(item), std::end(item), [&](auto&& v) { return invoke_pred(pred, std::forward<decltype(v)>(v)); });
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            return os << "(contains " << item.pred << ")";
        }
    };

    template <class Pred>
    auto operator()(Pred&& pred) const -> impl<std::decay_t<Pred>>
    {
        return impl<std::decay_t<Pred>>{ std::forward<Pred>(pred) };
    }
};

struct elements_are
{
};

struct elements_are_fn
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

struct elements_are_array_fn
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
                      << "elements_are_array " << ::ferrugo::core::safe_format(item.m_range) << ")";
        }
    };

    template <class Range>
    auto operator()(Range range) const -> impl<Range>
    {
        return impl<Range>{ std::move(range) };
    }
};

struct starts_with_elements_fn
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
            return size >= preds_count && elements_are_fn ::call(m_preds, b, std::next(b, preds_count));
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            os << "("
               << "starts_with_elements";
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
            return size >= preds_count && elements_are_array_fn::call(p_b, p_e, b, std::next(b, preds_count));
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

struct ends_with_elements_fn
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
            return size >= preds_count && elements_are_fn::call(m_preds, std::next(b, size - preds_count), e);
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            os << "("
               << "ends_with_elements";
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
            return size >= preds_count && elements_are_array_fn::call(p_b, p_e, std::next(b, size - preds_count), e);
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

struct contains_elements_fn
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
                if (elements_are_fn::call(m_preds, std::next(b, i), std::next(b, i + preds_count)))
                {
                    return true;
                }
            }
            return false;
        }

        friend std::ostream& operator<<(std::ostream& os, const impl& item)
        {
            os << "("
               << "contains_elements";
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
                if (elements_are_array_fn::call(p_b, p_e, std::next(b, i), std::next(b, i + preds_count)))
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

static constexpr inline auto any = detail::compound_fn<detail::any_tag, static_string<'a', 'n', 'y'>>{};
static constexpr inline auto all = detail::compound_fn<detail::all_tag, static_string<'a', 'l', 'l'>>{};
static constexpr inline auto negate = detail::negate_fn{};

static constexpr inline auto is_some = detail::is_some_fn{};

static constexpr inline auto each = detail::each_fn{};
static constexpr inline auto contains = detail::contains_fn{};
static constexpr inline auto size_is = detail::size_is_fn{};
static constexpr inline auto is_empty = detail::is_empty_fn{};

static constexpr inline auto elements_are = detail::elements_are_fn{};
static constexpr inline auto elements_are_array = detail::elements_are_array_fn{};
static constexpr inline auto starts_with_elements = detail::starts_with_elements_fn{};
static constexpr inline auto starts_with_array = detail::starts_with_array_fn{};
static constexpr inline auto ends_with_elements = detail::ends_with_elements_fn{};
static constexpr inline auto ends_with_array = detail::ends_with_array_fn{};
static constexpr inline auto contains_elements = detail::contains_elements_fn{};
static constexpr inline auto contains_array = detail::contains_array_fn{};

static constexpr inline auto eq = detail::compare_fn<std::equal_to<>, static_string<'e', 'q'>>{};
static constexpr inline auto ne = detail::compare_fn<std::not_equal_to<>, static_string<'n', 'e'>>{};
static constexpr inline auto lt = detail::compare_fn<std::less<>, static_string<'l', 't'>>{};
static constexpr inline auto gt = detail::compare_fn<std::greater<>, static_string<'g', 't'>>{};
static constexpr inline auto le = detail::compare_fn<std::less_equal<>, static_string<'l', 'e'>>{};
static constexpr inline auto ge = detail::compare_fn<std::greater_equal<>, static_string<'g', 'e'>>{};

static constexpr inline auto is_divisible_by = detail::is_divisible_by_fn{};
static constexpr inline auto is_odd = detail::is_odd_fn{};
static constexpr inline auto is_even = detail::is_even_fn{};

static constexpr inline auto result_of = detail::result_of_fn<static_string<'r', 'e', 's', 'u', 'l', 't', '_', 'o', 'f'>>{};
static constexpr inline auto field = detail::result_of_fn<static_string<'f', 'i', 'e', 'l', 'd'>>{};
static constexpr inline auto property = detail::result_of_fn<static_string<'p', 'r', 'o', 'p', 'e', 'r', 't', 'y'>>{};

static constexpr inline auto is_space = detail::is_space_fn{};
static constexpr inline auto is_digit = detail::is_digit_fn{};
static constexpr inline auto is_alnum = detail::is_alnum_fn{};
static constexpr inline auto is_alpha = detail::is_alpha_fn{};
static constexpr inline auto is_upper = detail::is_upper_fn{};
static constexpr inline auto is_lower = detail::is_lower_fn{};

}  // namespace predicates
}  // namespace ferrugo
