#include <catch2/catch_test_macros.hpp>
#include <ferrugo/core/ostream_utils.hpp>
#include <ferrugo/predicates/predicates.hpp>

#include "matchers.hpp"

using namespace ferrugo;
using namespace std::string_view_literals;

constexpr auto divisible_by(int divisor)
{
    return [=](int v) { return v % divisor == 0; };
}

TEST_CASE("predicates - format", "")
{
    REQUIRE_THAT(  //
        core::str(predicates::all(predicates::ge(0), predicates::lt(5))),
        matchers::equal_to("(all (ge 0) (lt 5))"sv));
    REQUIRE_THAT(  //
        core::str(predicates::any(1, 2, 3, predicates::ge(100))),
        matchers::equal_to("(any 1 2 3 (ge 100))"sv));
    REQUIRE_THAT(  //
        core::str(predicates::negate(predicates::any(1, 2, 3))),
        matchers::equal_to("(not (any 1 2 3))"sv));
    REQUIRE_THAT(  //
        core::str(predicates::each(predicates::any(1, 2, 3))),
        matchers::equal_to("(each (any 1 2 3))"sv));
    REQUIRE_THAT(  //
        core::str(predicates::contains(predicates::any(1, 2, 3))),
        matchers::equal_to("(contains (any 1 2 3))"sv));
    REQUIRE_THAT(  //
        core::str(predicates::size_is(predicates::lt(8))),
        matchers::equal_to("(size_is (lt 8))"sv));
}

TEST_CASE("predicates - eq", "")
{
    const auto pred = predicates::eq(10);
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(eq 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(true));
    REQUIRE_THAT(pred(5), matchers::equal_to(false));
    REQUIRE_THAT(pred(15), matchers::equal_to(false));
}

TEST_CASE("predicates - ne", "")
{
    const auto pred = predicates::ne(10);
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(ne 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(false));
    REQUIRE_THAT(pred(5), matchers::equal_to(true));
    REQUIRE_THAT(pred(15), matchers::equal_to(true));
}

TEST_CASE("predicates - lt", "")
{
    const auto pred = predicates::lt(10);
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(lt 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(false));
    REQUIRE_THAT(pred(5), matchers::equal_to(true));
    REQUIRE_THAT(pred(15), matchers::equal_to(false));
}

TEST_CASE("predicates - gt", "")
{
    const auto pred = predicates::gt(10);
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(gt 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(false));
    REQUIRE_THAT(pred(5), matchers::equal_to(false));
    REQUIRE_THAT(pred(15), matchers::equal_to(true));
}

TEST_CASE("predicates - le", "")
{
    const auto pred = predicates::le(10);
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(le 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(true));
    REQUIRE_THAT(pred(5), matchers::equal_to(true));
    REQUIRE_THAT(pred(15), matchers::equal_to(false));
}

TEST_CASE("predicates - ge", "")
{
    const auto pred = predicates::ge(10);
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(ge 10)"sv));
    REQUIRE_THAT(pred(10), matchers::equal_to(true));
    REQUIRE_THAT(pred(5), matchers::equal_to(false));
    REQUIRE_THAT(pred(15), matchers::equal_to(true));
}

TEST_CASE("predicates - all", "")
{
    const auto pred = predicates::all(predicates::ge(10), predicates::lt(20), divisible_by(3));
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(all (ge 10) (lt 20) divisible_by(int)::{lambda(int)#1})"sv));
    REQUIRE_THAT(pred(9), matchers::equal_to(false));
    REQUIRE_THAT(pred(10), matchers::equal_to(false));
    REQUIRE_THAT(pred(11), matchers::equal_to(false));
    REQUIRE_THAT(pred(12), matchers::equal_to(true));
    REQUIRE_THAT(pred(13), matchers::equal_to(false));
    REQUIRE_THAT(pred(14), matchers::equal_to(false));
    REQUIRE_THAT(pred(15), matchers::equal_to(true));
    REQUIRE_THAT(pred(16), matchers::equal_to(false));
    REQUIRE_THAT(pred(17), matchers::equal_to(false));
    REQUIRE_THAT(pred(18), matchers::equal_to(true));
    REQUIRE_THAT(pred(19), matchers::equal_to(false));
    REQUIRE_THAT(pred(20), matchers::equal_to(false));
    REQUIRE_THAT(pred(21), matchers::equal_to(false));
}

TEST_CASE("predicates - any", "")
{
    const auto pred = predicates::any(divisible_by(5), divisible_by(3), 100);
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(any divisible_by(int)::{lambda(int)#1} divisible_by(int)::{lambda(int)#1} 100)"sv));
    REQUIRE_THAT(pred(9), matchers::equal_to(true));
    REQUIRE_THAT(pred(10), matchers::equal_to(true));
    REQUIRE_THAT(pred(11), matchers::equal_to(false));
    REQUIRE_THAT(pred(12), matchers::equal_to(true));
    REQUIRE_THAT(pred(13), matchers::equal_to(false));
    REQUIRE_THAT(pred(14), matchers::equal_to(false));
    REQUIRE_THAT(pred(15), matchers::equal_to(true));
    REQUIRE_THAT(pred(16), matchers::equal_to(false));
    REQUIRE_THAT(pred(17), matchers::equal_to(false));
    REQUIRE_THAT(pred(18), matchers::equal_to(true));
    REQUIRE_THAT(pred(19), matchers::equal_to(false));
    REQUIRE_THAT(pred(20), matchers::equal_to(true));
    REQUIRE_THAT(pred(21), matchers::equal_to(true));
    REQUIRE_THAT(pred(100), matchers::equal_to(true));
}

TEST_CASE("predicates - negate", "")
{
    const auto pred = predicates::negate(predicates::all(predicates::ge(0), predicates::lt(5)));
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(not (all (ge 0) (lt 5)))"sv));
    REQUIRE_THAT(pred(-1), matchers::equal_to(true));
    REQUIRE_THAT(pred(0), matchers::equal_to(false));
    REQUIRE_THAT(pred(1), matchers::equal_to(false));
    REQUIRE_THAT(pred(2), matchers::equal_to(false));
    REQUIRE_THAT(pred(3), matchers::equal_to(false));
    REQUIRE_THAT(pred(4), matchers::equal_to(false));
    REQUIRE_THAT(pred(5), matchers::equal_to(true));
}

TEST_CASE("predicates - is_some", "")
{
    const auto pred = predicates::is_some(predicates::ge(5));
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(is_some (ge 5))"sv));
    REQUIRE_THAT(pred(std::optional{ 5 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::optional{ 6 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::optional{ 4 }), matchers::equal_to(false));
    REQUIRE_THAT(pred(std::optional<int>{}), matchers::equal_to(false));
}

TEST_CASE("predicates - is_empty", "")
{
    const auto pred = predicates::is_empty();
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(is_empty)"sv));
    REQUIRE_THAT(pred(""sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("###"sv), matchers::equal_to(false));
}

TEST_CASE("predicates - size_is", "")
{
    const auto pred = predicates::size_is(predicates::lt(3));
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(size_is (lt 3))"sv));
    REQUIRE_THAT(pred(""sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("#"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("##"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("###"sv), matchers::equal_to(false));
}

TEST_CASE("predicates - each", "")
{
    const auto pred = predicates::each('#');
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(each #)"sv));
    REQUIRE_THAT(pred(""sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("#"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("##"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("##__"sv), matchers::equal_to(false));
}

TEST_CASE("predicates - contains", "")
{
    const auto pred = predicates::contains('#');
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(contains #)"sv));
    REQUIRE_THAT(pred(""sv), matchers::equal_to(false));
    REQUIRE_THAT(pred("#"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("##"sv), matchers::equal_to(true));
    REQUIRE_THAT(pred("__"sv), matchers::equal_to(false));
}

TEST_CASE("predicates - elements_are", "")
{
    const auto pred = predicates::elements_are(0, predicates::ge(3), predicates::le(5), 10);
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(elements_are 0 (ge 3) (le 5) 10)"sv));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 4, 4, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5, 10, 100 }), matchers::equal_to(false));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5 }), matchers::equal_to(false));
}

TEST_CASE("predicates - elements_are_array", "")
{
    const auto pred = predicates::elements_are_array(std::vector{ 1, 3, 5 });
    // REQUIRE_THAT(  //
    //     core::str(pred),
    //     matchers::equal_to("(elements_are 0 (ge 3) (le 5) 10)"sv));
    REQUIRE_THAT(pred(std::vector{ 1, 3, 5 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 1, 3, 5, 10 }), matchers::equal_to(false));
    REQUIRE_THAT(pred(std::vector{ 1, 3 }), matchers::equal_to(false));
    REQUIRE_THAT(pred(std::vector{ 2, 2, 5 }), matchers::equal_to(false));
}

TEST_CASE("predicates - starts_with_elements", "")
{
    const auto pred = predicates::starts_with_elements(0, predicates::ge(3), predicates::le(5), 10);
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(starts_with_elements 0 (ge 3) (le 5) 10)"sv));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5, 10, 100 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 4, 4, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5 }), matchers::equal_to(false));
}

TEST_CASE("predicates - starts_with_array", "")
{
    const auto pred = predicates::starts_with_array(std::vector{ 1, 3, 5 });
    // REQUIRE_THAT(  //
    //     core::str(pred),
    //     matchers::equal_to("(elements_are 0 (ge 3) (le 5) 10)"sv));
    REQUIRE_THAT(pred(std::vector{ 1, 3, 5 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 1, 3, 5, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 1, 3 }), matchers::equal_to(false));
    REQUIRE_THAT(pred(std::vector{ 2, 2, 5 }), matchers::equal_to(false));
}

TEST_CASE("predicates - ends_with_elements", "")
{
    const auto pred = predicates::ends_with_elements(0, predicates::ge(3), predicates::le(5), 10);
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(ends_with_elements 0 (ge 3) (le 5) 10)"sv));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 100, 99, 0, 3, 5, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 4, 4, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5 }), matchers::equal_to(false));
}

TEST_CASE("predicates - ends_with_array", "")
{
    const auto pred = predicates::ends_with_array(std::vector{ 1, 3, 5 });
    // REQUIRE_THAT(  //
    //     core::str(pred),
    //     matchers::equal_to("(elements_are 0 (ge 3) (le 5) 10)"sv));
    REQUIRE_THAT(pred(std::vector{ 1, 3, 5 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 10, 1, 3, 5 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 3, 5 }), matchers::equal_to(false));
    REQUIRE_THAT(pred(std::vector{ 2, 2, 5 }), matchers::equal_to(false));
}

TEST_CASE("predicates - contains_elements", "")
{
    const auto pred = predicates::contains_elements(0, predicates::ge(3), predicates::le(5), 10);
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(contains_elements 0 (ge 3) (le 5) 10)"sv));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 100, 99, 0, 3, 5, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 100, 99, 0, 3, 5, 10, 200 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 4, 4, 10 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 0, 3, 5 }), matchers::equal_to(false));
}

TEST_CASE("predicates - contains_array", "")
{
    const auto pred = predicates::contains_array(std::vector{ 1, 3, 5 });
    // REQUIRE_THAT(  //
    //     core::str(pred),
    //     matchers::equal_to("(elements_are 0 (ge 3) (le 5) 10)"sv));
    REQUIRE_THAT(pred(std::vector{ 1, 3, 5 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 10, 1, 3, 5 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 10, 1, 3, 5, 20 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::vector{ 3, 5 }), matchers::equal_to(false));
    REQUIRE_THAT(pred(std::vector{ 2, 2, 5 }), matchers::equal_to(false));
}

TEST_CASE("predicates - result_of", "")
{
    const auto pred = predicates::result_of([](const std::string& v) { return v.size(); }, predicates::le(3));
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(result_of 1 (le 3))"sv));
    REQUIRE_THAT(pred(std::string{ "abc" }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::string{ "ab" }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::string{ "abcd" }), matchers::equal_to(false));
}

TEST_CASE("predicates - field", "")
{
    struct test_t
    {
        int field;
    };
    const auto pred = predicates::field(&test_t::field, predicates::le(3));
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(field 1 (le 3))"sv));
    REQUIRE_THAT(pred(test_t{ 3 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(test_t{ 2 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(test_t{ 12 }), matchers::equal_to(false));
}

TEST_CASE("predicates - property", "")
{
    struct test_t
    {
        int m_field;

        int field() const
        {
            return m_field;
        }
    };
    const auto pred = predicates::property(&test_t::field, predicates::le(3));
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(property 1 (le 3))"sv));
    REQUIRE_THAT(pred(test_t{ 3 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(test_t{ 2 }), matchers::equal_to(true));
    REQUIRE_THAT(pred(test_t{ 12 }), matchers::equal_to(false));
}

TEST_CASE("predicates - is_divisible_by", "")
{
    const auto pred = predicates::is_divisible_by(3);
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(is_divisible_by 3)"sv));

    REQUIRE_THAT(pred(3), matchers::equal_to(true));
    REQUIRE_THAT(pred(6), matchers::equal_to(true));
    REQUIRE_THAT(pred(5), matchers::equal_to(false));
}

TEST_CASE("predicates - is_odd", "")
{
    const auto pred = predicates::is_odd();
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(is_odd)"sv));
    REQUIRE_THAT(pred(3), matchers::equal_to(true));
    REQUIRE_THAT(pred(5), matchers::equal_to(true));
    REQUIRE_THAT(pred(2), matchers::equal_to(false));
}

TEST_CASE("predicates - is_even", "")
{
    const auto pred = predicates::is_even();
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(is_even)"sv));
    REQUIRE_THAT(pred(4), matchers::equal_to(true));
    REQUIRE_THAT(pred(6), matchers::equal_to(true));
    REQUIRE_THAT(pred(5), matchers::equal_to(false));
}

TEST_CASE("predicates - is_space", "")
{
    const auto pred = predicates::is_space();
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(is_space)"sv));
    REQUIRE_THAT(pred(' '), matchers::equal_to(true));
    REQUIRE_THAT(pred('\n'), matchers::equal_to(true));
    REQUIRE_THAT(pred('X'), matchers::equal_to(false));
}

TEST_CASE("predicates - is_digit", "")
{
    const auto pred = predicates::is_digit();
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(is_digit)"sv));
    REQUIRE_THAT(pred('1'), matchers::equal_to(true));
    REQUIRE_THAT(pred('9'), matchers::equal_to(true));
    REQUIRE_THAT(pred('A'), matchers::equal_to(false));
}

TEST_CASE("predicates - is_alnum", "")
{
    const auto pred = predicates::is_alnum();
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(is_alnum)"sv));
    REQUIRE_THAT(pred('A'), matchers::equal_to(true));
    REQUIRE_THAT(pred('3'), matchers::equal_to(true));
    REQUIRE_THAT(pred(' '), matchers::equal_to(false));
}

TEST_CASE("predicates - is_alpha", "")
{
    const auto pred = predicates::is_alpha();
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(is_alpha)"sv));
    REQUIRE_THAT(pred('A'), matchers::equal_to(true));
    REQUIRE_THAT(pred('Z'), matchers::equal_to(true));
    REQUIRE_THAT(pred('3'), matchers::equal_to(false));
    REQUIRE_THAT(pred(' '), matchers::equal_to(false));
}

TEST_CASE("predicates - is_upper", "")
{
    const auto pred = predicates::is_upper();
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(is_upper)"sv));
    REQUIRE_THAT(pred('A'), matchers::equal_to(true));
    REQUIRE_THAT(pred('a'), matchers::equal_to(false));
}

TEST_CASE("predicates - is_lower", "")
{
    const auto pred = predicates::is_lower();
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(is_lower)"sv));
    REQUIRE_THAT(pred('a'), matchers::equal_to(true));
    REQUIRE_THAT(pred('A'), matchers::equal_to(false));
}

TEST_CASE("predicates - fields_are", "")
{
    const auto pred = predicates::fields_are(predicates::ge(10), predicates::eq('X'), predicates::contains('_'));
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(fields_are (ge 10) (eq X) (contains _))"sv));
    REQUIRE_THAT(pred(std::tuple{ 10, 'X', "12_"sv }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::tuple{ 15, 'X', "_"sv }), matchers::equal_to(true));

    REQUIRE_THAT(pred(std::tuple{ 10, 'Z', "_"sv }), matchers::equal_to(false));
}

TEST_CASE("predicates - element", "")
{
    const auto pred = predicates::element<1>(predicates::ge(5));
    REQUIRE_THAT(  //
        core::str(pred),
        matchers::equal_to("(element 1 (ge 5))"sv));

    REQUIRE_THAT(pred(std::tuple{ ' ', 5, ' ' }), matchers::equal_to(true));
    REQUIRE_THAT(pred(std::tuple{ ' ', 6, ' ' }), matchers::equal_to(true));

    REQUIRE_THAT(pred(std::tuple{ ' ', 4, ' ' }), matchers::equal_to(false));
}