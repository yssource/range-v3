// Range v3 library
//
//  Copyright Eric Niebler 2014-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3

#include <string>
#include <cctype>
#include <sstream>
#include <range/v3/core.hpp>
#include <range/v3/view/counted.hpp>
#include <range/v3/view/c_str.hpp>
#include <range/v3/view/empty.hpp>
#include <range/v3/view/remove_if.hpp>
#include <range/v3/view/split.hpp>
#include <range/v3/view/split_when.hpp>
#include "../simple_test.hpp"
#include "../test_utils.hpp"
#include "../test_iterators.hpp"

RANGES_DIAGNOSTIC_IGNORE_SIGN_CONVERSION

#if defined(__clang__) && __clang_major__ < 6
// Workaround https://bugs.llvm.org/show_bug.cgi?id=33314
RANGES_DIAGNOSTIC_IGNORE_UNDEFINED_FUNC_TEMPLATE
#endif

namespace
{
    struct starts_with_g
    {
        template<typename I, typename S>
        std::pair<bool, I> operator()(I b, S) const
        {
            return {*b == 'g', b};
        }
    };

    template<std::size_t N>
    ranges::subrange<char const*> c_str(char const (&sz)[N])
    {
        return {&sz[0], &sz[N-1]};
    }
}

void moar_tests()
{
    using namespace ranges;
    std::string greeting = "now is the time";
    std::string pattern = " ";

    {
#if RANGES_CXX_DEDUCTION_GUIDES >= RANGES_CXX_DEDUCTION_GUIDES_17
        split_view sv{greeting, pattern};
#else
        split_view<view::all_t<std::string&>, view::all_t<std::string&>> sv{greeting, pattern};
#endif
        auto i = sv.begin();
        check_equal(*i, {'n','o','w'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, {'i','s'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, {'t','h','e'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, {'t','i','m','e'});
        ++i;
        CHECK(i == sv.end());

        using R = decltype(sv);
        CPP_assert(ForwardRange<R>);
        CPP_assert(ForwardRange<R const>);
    }

    {
#if RANGES_CXX_DEDUCTION_GUIDES >= RANGES_CXX_DEDUCTION_GUIDES_17
        split_view sv{greeting, ' '};
#else
        split_view<view::all_t<std::string&>, single_view<char>> sv{greeting, ' '};
#endif
        auto i = sv.begin();
        CHECK(i != sv.end());
        check_equal(*i, {'n','o','w'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, {'i','s'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, {'t','h','e'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, {'t','i','m','e'});
        ++i;
        CHECK(i == sv.end());

        using R = decltype(sv);
        CPP_assert(ForwardRange<R>);
        CPP_assert(ForwardRange<R const>);
    }

    {
        std::stringstream sin{greeting};
#if RANGES_CXX_DEDUCTION_GUIDES >= RANGES_CXX_DEDUCTION_GUIDES_17
        auto rng = subrange{
            std::istreambuf_iterator<char>{sin},
            std::istreambuf_iterator<char>{}};
#else
        auto rng = make_subrange(
            std::istreambuf_iterator<char>{sin},
            std::istreambuf_iterator<char>{});
#endif

        auto sv = view::split(rng, ' ');
        auto i = sv.begin();
        CHECK(i != sv.end());
        check_equal(*i, {'n','o','w'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, {'i','s'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, {'t','h','e'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, {'t','i','m','e'});
        ++i;
        CHECK(i == sv.end());

        using R = decltype(sv);
        CPP_assert(InputRange<R>);
        CPP_assert(!ForwardRange<R>);
        CPP_assert(!InputRange<R const>);
    }

    {
        std::string list{"eggs,milk,,butter"};
        auto sv = view::split(list, ',');
        auto i = sv.begin();
        CHECK(i != sv.end());
        check_equal(*i, {'e','g','g','s'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, {'m','i','l','k'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, view::empty<char>);
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, {'b','u','t','t','e','r'});
        ++i;
        CHECK(i == sv.end());
    }

    {
        std::string list{"eggs,milk,,butter"};
        std::stringstream sin{list};
        auto rng = make_subrange(
            std::istreambuf_iterator<char>{sin},
            std::istreambuf_iterator<char>{});
        auto sv = rng | view::split(',');
        auto i = sv.begin();
        CHECK(i != sv.end());
        check_equal(*i, {'e','g','g','s'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, {'m','i','l','k'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, view::empty<char>);
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, {'b','u','t','t','e','r'});
        ++i;
        CHECK(i == sv.end());
    }

    {
        std::string hello("hello");
        auto sv = view::split(hello, view::empty<char>);
        auto i = sv.begin();
        CHECK(i != sv.end());
        check_equal(*i, single_view<char>{'h'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, single_view<char>{'e'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, single_view<char>{'l'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, single_view<char>{'l'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, single_view<char>{'o'});
        ++i;
        CHECK(i == sv.end());
    }

    {
        std::string hello{"hello"};
        std::stringstream sin{hello};
        auto rng = make_subrange(
            std::istreambuf_iterator<char>{sin},
            std::istreambuf_iterator<char>{});
        auto sv = view::split(rng, view::empty<char>);
        auto i = sv.begin();
        CHECK(i != sv.end());
        check_equal(*i, single_view<char>{'h'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, single_view<char>{'e'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, single_view<char>{'l'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, single_view<char>{'l'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, single_view<char>{'o'});
        ++i;
        CHECK(i == sv.end());
    }

    {
        std::string hello{"hello"};
        auto sv = view::split(hello, view::empty<char>);
        auto i = sv.begin();
        CHECK(i != sv.end());
        ++i;
        CHECK(i != sv.end());
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, single_view<char>{'l'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, single_view<char>{'l'});
        ++i;
        CHECK(i != sv.end());
        ++i;
        CHECK(i == sv.end());
    }

    {
        std::string hello{"hello"};
        std::stringstream sin{hello};
        auto rng = make_subrange(
            std::istreambuf_iterator<char>{sin},
            std::istreambuf_iterator<char>{});
        auto sv = view::split(rng, view::empty<char>);
        auto i = sv.begin();
        CHECK(i != sv.end());
        ++i;
        CHECK(i != sv.end());
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, single_view<char>{'l'});
        ++i;
        CHECK(i != sv.end());
        check_equal(*i, single_view<char>{'l'});
        ++i;
        CHECK(i != sv.end());
        ++i;
        CHECK(i == sv.end());
    }
}

int main()
{
    using namespace ranges;

    {
        std::string str("Now is the time for all good men to come to the aid of their country.");
        auto rng = view::split(str, ' ');
        CHECK(distance(rng) == 16);
        if(distance(rng) == 16)
        {
            check_equal(*(next(begin(rng),0)), c_str("Now"));
            check_equal(*(next(begin(rng),1)), c_str("is"));
            check_equal(*(next(begin(rng),2)), c_str("the"));
            check_equal(*(next(begin(rng),3)), c_str("time"));
            check_equal(*(next(begin(rng),4)), c_str("for"));
            check_equal(*(next(begin(rng),5)), c_str("all"));
            check_equal(*(next(begin(rng),6)), c_str("good"));
            check_equal(*(next(begin(rng),7)), c_str("men"));
            check_equal(*(next(begin(rng),8)), c_str("to"));
            check_equal(*(next(begin(rng),9)), c_str("come"));
            check_equal(*(next(begin(rng),10)), c_str("to"));
            check_equal(*(next(begin(rng),11)), c_str("the"));
            check_equal(*(next(begin(rng),12)), c_str("aid"));
            check_equal(*(next(begin(rng),13)), c_str("of"));
            check_equal(*(next(begin(rng),14)), c_str("their"));
            check_equal(*(next(begin(rng),15)), c_str("country."));
        }
    }

    {
        std::string str("Now is the time for all good men to come to the aid of their country.");
        auto rng = view::split(str, c_str(" "));
        CHECK(distance(rng) == 16);
        if(distance(rng) == 16)
        {
            check_equal(*(next(begin(rng),0)), c_str("Now"));
            check_equal(*(next(begin(rng),1)), c_str("is"));
            check_equal(*(next(begin(rng),2)), c_str("the"));
            check_equal(*(next(begin(rng),3)), c_str("time"));
            check_equal(*(next(begin(rng),4)), c_str("for"));
            check_equal(*(next(begin(rng),5)), c_str("all"));
            check_equal(*(next(begin(rng),6)), c_str("good"));
            check_equal(*(next(begin(rng),7)), c_str("men"));
            check_equal(*(next(begin(rng),8)), c_str("to"));
            check_equal(*(next(begin(rng),9)), c_str("come"));
            check_equal(*(next(begin(rng),10)), c_str("to"));
            check_equal(*(next(begin(rng),11)), c_str("the"));
            check_equal(*(next(begin(rng),12)), c_str("aid"));
            check_equal(*(next(begin(rng),13)), c_str("of"));
            check_equal(*(next(begin(rng),14)), c_str("their"));
            check_equal(*(next(begin(rng),15)), c_str("country."));
        }
    }

    {
        std::string str("Now is the time for all ggood men to come to the aid of their country.");
        auto rng = view::split_when(str, starts_with_g{});
        CHECK(distance(rng) == 3);
        if(distance(rng) == 3)
        {
            check_equal(*begin(rng), c_str("Now is the time for all "));
            check_equal(*next(begin(rng)), c_str("g"));
            check_equal(*(next(begin(rng),2)), c_str("good men to come to the aid of their country."));
        }
    }

    {
        std::string str("Now is the time for all ggood men to come to the aid of their country.");
        forward_iterator<std::string::iterator> i {str.begin()};
        auto rng = view::counted(i, str.size()) | view::split_when(starts_with_g{});
        CHECK(distance(rng) == 3);
        if(distance(rng) == 3)
        {
            check_equal(*begin(rng), c_str("Now is the time for all "));
            check_equal(*next(begin(rng)), c_str("g"));
            check_equal(*(next(begin(rng),2)), c_str("good men to come to the aid of their country."));
        }
    }

    {
        std::string str("meow");
        auto rng = view::split(str, view::empty<char>);
        CHECK(distance(rng) == 4);
        if(distance(rng) == 4)
        {
            check_equal(*(next(begin(rng),0)), c_str("m"));
            check_equal(*(next(begin(rng),1)), c_str("e"));
            check_equal(*(next(begin(rng),2)), c_str("o"));
            check_equal(*(next(begin(rng),3)), c_str("w"));
        }
    }

    {
        int a[] = {0, 2, 3, 1, 4, 5, 1, 6, 7};
        auto rng = a | view::remove_if([](int i) { return i % 2 == 0; });
        auto srng = view::split(rng, 1);
        CHECK(distance(srng) == 3);
        check_equal(*begin(srng), {3});
        check_equal(*next(begin(srng), 1), {5});
        check_equal(*next(begin(srng), 2), {7});
    }

    {
        std::string str("now  is \t the\ttime");
        auto rng = view::split_when(str, (int(*)(int))&std::isspace);
        CHECK(distance(rng) == 4);
        if(distance(rng) == 4)
        {
            check_equal(*(next(begin(rng),0)), c_str("now"));
            check_equal(*(next(begin(rng),1)), c_str("is"));
            check_equal(*(next(begin(rng),2)), c_str("the"));
            check_equal(*(next(begin(rng),3)), c_str("time"));
        }
    }

    {   // Regression test for https://stackoverflow.com/questions/49015671
        auto const str = "quick brown fox";
        auto rng = view::c_str(str) | view::split(' ');
        CPP_assert(ForwardRange<decltype(rng)>);
    }

    {   // Regression test for #986
        std::string s;
        s | ranges::view::split_when([](char) { return true; });
    }

    moar_tests();

    {   // Regression test for #1041
        auto is_escape = [](auto first, auto last) {
            return std::make_pair(next(first) != last, first);
        };

        auto escapes = view::split_when(view::c_str(R"(\t)"), is_escape);
        CPP_assert(ForwardRange<decltype(escapes)>);

        auto const first = begin(escapes);
        CHECK(first != end(escapes));
        CHECK(first != next(first));
    }

    return test_result();
}
