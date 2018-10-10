/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-present
//  Copyright Casey Carter 2016
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3

#ifndef RANGES_V3_UTILITY_TAGGED_PAIR_HPP
#define RANGES_V3_UTILITY_TAGGED_PAIR_HPP

#include <utility>
#include <meta/meta.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/detail/adl_get.hpp>
#include <range/v3/utility/concepts.hpp>
#include <range/v3/utility/swap.hpp>

namespace ranges
{
    inline namespace v3
    {
        /// \cond
        namespace detail
        {
            template<typename T>
            using tag_spec = meta::front<meta::as_list<T>>;

            template<typename T>
            using tag_elem = meta::back<meta::as_list<T>>;
        }

        namespace _tagged_
        {
            template<typename Base>
            struct wrap_base : Base
            {
                wrap_base() = default;
                using Base::Base;
#if !defined(__clang__) || __clang_major__ > 3
                CPP_member
                constexpr CPP_ctor(wrap_base)(Base&& base)(
                    noexcept(std::is_nothrow_move_constructible<Base>::value)
                    requires MoveConstructible<Base>)
                  : Base(static_cast<Base&&>(base))
                {}
                CPP_member
                constexpr CPP_ctor(wrap_base)(Base const& base)(
                    noexcept(std::is_nothrow_copy_constructible<Base>::value)
                    requires CopyConstructible<Base>)
                  : Base(base)
                {}
#else
                // Clang 3.x have a problem with inheriting constructors
                // that causes the declarations in the preceeding PP block to get
                // instantiated too early.
                CPP_template(typename B = Base)(
                    requires MoveConstructible<B>)
                constexpr wrap_base(Base&& base)
                    noexcept(std::is_nothrow_move_constructible<Base>::value)
                  : Base(static_cast<Base&&>(base))
                {}
                CPP_template(typename B = Base)(
                    requires CopyConstructible<B>)
                constexpr wrap_base(Base const& base)
                    noexcept(std::is_nothrow_copy_constructible<Base>::value)
                  : Base(base)
                {}
#endif
                template<std::size_t I>
                auto get(Base &base) noexcept ->
                    decltype(detail::adl_get<I>(base))
                {
                    return detail::adl_get<I>(base);
                }
                template<std::size_t I>
                auto get(Base const &base) noexcept ->
                    decltype(detail::adl_get<I>(base))
                {
                    return detail::adl_get<I>(base);
                }
                template<std::size_t I>
                auto get(Base &&base) noexcept ->
                    decltype(detail::adl_get<I>(static_cast<Base &&>(base)))
                {
                    return detail::adl_get<I>(static_cast<Base &&>(base));
                }
                template<std::size_t I>
                auto get(Base const &&base) noexcept ->
                    decltype(detail::adl_get<I>(static_cast<Base const &&>(base)))
                {
                    return detail::adl_get<I>(static_cast<Base const &&>(base));
                }
                template<typename T>
                auto get(Base &base) noexcept ->
                    decltype(detail::adl_get<T>(base))
                {
                    return detail::adl_get<T>(base);
                }
                template<typename T>
                auto get(Base const &base) noexcept ->
                    decltype(detail::adl_get<T>(base))
                {
                    return detail::adl_get<T>(base);
                }
                template<typename T>
                auto get(Base &&base) noexcept ->
                    decltype(detail::adl_get<T>(static_cast<Base &&>(base)))
                {
                    return detail::adl_get<T>(static_cast<Base &&>(base));
                }
                template<typename T>
                auto get(Base const &&base) noexcept ->
                    decltype(detail::adl_get<T>(static_cast<Base const &&>(base)))
                {
                    return detail::adl_get<T>(static_cast<Base const &&>(base));
                }
            };
            template<typename Base, std::size_t, typename...>
            struct chain
            {
                using type = wrap_base<Base>;
            };
            template<typename Base, std::size_t I, typename First, typename... Rest>
            struct chain<Base, I, First, Rest...>
            {
                using type = typename First::template getter<
                    Base, I, meta::_t<chain<Base, I + 1, Rest...>>>;
            };

#if RANGES_BROKEN_CPO_LOOKUP
            template<typename> struct adl_hook {};
#endif
        }
        /// \endcond

        template<typename Base, typename...Tags>
        class tagged
          : public meta::_t<_tagged_::chain<Base, 0, Tags...>>
#if RANGES_BROKEN_CPO_LOOKUP
          , private _tagged_::adl_hook<tagged<Base, Tags...>>
#endif
        {
            CPP_assert(Same<Base, uncvref_t<Base>>);
            using base_t = meta::_t<_tagged_::chain<Base, 0, Tags...>>;

            template<typename Other>
            using can_convert =
                meta::bool_<!std::is_same<Other, Base>::value &&
                    std::is_convertible<Other, Base>::value>;
        public:
            tagged() = default;
            using base_t::base_t;
#if !defined(__clang__) || __clang_major__ > 3
            template<typename Other>
            constexpr CPP_ctor(tagged)(tagged<Other, Tags...> && that)(
                noexcept(std::is_nothrow_constructible<Base, Other>::value)
                requires can_convert<Other>::value)
              : base_t(static_cast<Other &&>(that))
            {}
            template<typename Other>
            constexpr CPP_ctor(tagged)(tagged<Other, Tags...> const &that)(
                noexcept(std::is_nothrow_constructible<Base, Other const &>::value)
                requires can_convert<Other>::value)
              : base_t(static_cast<Other const &>(that))
            {}
#else
            // Clang 3.x have a problem with inheriting constructors
            // that causes the declarations in the preceeding PP block to get
            // instantiated too early.
            CPP_template(typename Other)(
                requires can_convert<Other>::value)
            constexpr tagged(tagged<Other, Tags...> && that)
                noexcept(std::is_nothrow_constructible<Base, Other>::value)
              : base_t(static_cast<Other &&>(that))
            {}
            CPP_template(typename Other)(
                requires can_convert<Other>::value)
            constexpr tagged(tagged<Other, Tags...> const &that)
                noexcept(std::is_nothrow_constructible<Base, Other const &>::value)
              : base_t(static_cast<Other const &>(that))
            {}
#endif
            template<typename Other>
            constexpr /*c++14*/ auto operator=(tagged<Other, Tags...> && that)
                noexcept(noexcept(std::declval<Base &>() = static_cast<Other &&>(that))) ->
                CPP_ret(tagged &)(
                    requires can_convert<Other>::value)
            {
                static_cast<Base &>(*this) = static_cast<Other &&>(that);
                return *this;
            }
            template<typename Other>
            constexpr /*c++14*/ auto operator=(tagged<Other, Tags...> const &that)
                noexcept(noexcept(std::declval<Base &>() = static_cast<Other const &>(that))) ->
                CPP_ret(tagged &)(
                    requires can_convert<Other>::value)
            {
                static_cast<Base &>(*this) = static_cast<Other const &>(that);
                return *this;
            }
            template<typename U>
            constexpr /*c++14*/ auto operator=(U && u)
                noexcept(noexcept(std::declval<Base &>() = static_cast<U&&>(u))) ->
                CPP_ret(tagged &)(
                    requires not defer::Same<tagged, detail::decay_t<U>> &&
                        defer::Satisfies<Base &, std::is_assignable, U>)
            {
                static_cast<Base &>(*this) = static_cast<U&&>(u);
                return *this;
            }
            template<typename B = Base>
            constexpr /*c++14*/
            auto swap(tagged &that)
                noexcept(is_nothrow_swappable<B>::value) ->
                CPP_ret(void)(
                    requires is_swappable<B>::value)
            {
                ranges::swap(static_cast<Base &>(*this), static_cast<Base &>(that));
            }
#if !RANGES_BROKEN_CPO_LOOKUP
            template<typename B = Base>
            friend constexpr /*c++14*/
            auto swap(tagged &x, tagged &y)
                noexcept(is_nothrow_swappable<B>::value) ->
                CPP_broken_friend_ret(void)(
                    requires is_swappable<B>::value)
            {
                x.swap(y);
            }
#endif
        };

#if RANGES_BROKEN_CPO_LOOKUP
        namespace _tagged_
        {
            template<typename Base, typename...Tags>
            constexpr /*c++14*/
            auto swap(tagged<Base, Tags...> &x, tagged<Base, Tags...> &y)
                noexcept(is_nothrow_swappable<Base>::value) ->
                CPP_ret(void)(
                    requires is_swappable<Base>::value)
            {
                x.swap(y);
            }
        }
#endif

        template<typename F, typename S>
        using tagged_pair =
            tagged<std::pair<detail::tag_elem<F>, detail::tag_elem<S>>,
                   detail::tag_spec<F>, detail::tag_spec<S>>;

        template<typename Tag1, typename Tag2, typename T1, typename T2,
            typename R = tagged_pair<Tag1(bind_element_t<T1>), Tag2(bind_element_t<T2>)>>
        constexpr R make_tagged_pair(T1 &&t1, T2 &&t2)
            noexcept(std::is_nothrow_constructible<R, T1, T2>::value)
        {
            return {static_cast<T1 &&>(t1), static_cast<T2 &&>(t2)};
        }
    }
}

#define RANGES_DEFINE_TAG_SPECIFIER(NAME)                                            \
    namespace tag                                                                    \
    {                                                                                \
        struct NAME                                                                  \
        {                                                                            \
            template<typename Untagged, std::size_t I, typename Next>                \
            class getter : public Next                                               \
            {                                                                        \
            protected:                                                               \
                ~getter() = default;                                                 \
            public:                                                                  \
                getter() = default;                                                  \
                getter(getter &&) = default;                                         \
                getter(getter const &) = default;                                    \
                using Next::Next;                                                    \
                getter &operator=(getter &&) = default;                              \
                getter &operator=(getter const &) = default;                         \
                constexpr /*c++14*/                                               \
                meta::_t<std::tuple_element<I, Untagged>> &NAME() &                  \
                    noexcept(noexcept(                                               \
                        detail::adl_get<I>(std::declval<Untagged &>())))             \
                {                                                                    \
                    return detail::adl_get<I>(static_cast<Untagged &>(*this));       \
                }                                                                    \
                constexpr /*c++14*/                                               \
                meta::_t<std::tuple_element<I, Untagged>> &&NAME() &&                \
                    noexcept(noexcept(                                               \
                        detail::adl_get<I>(std::declval<Untagged>())))               \
                {                                                                    \
                    return detail::adl_get<I>(static_cast<Untagged &&>(*this));      \
                }                                                                    \
                constexpr                                                            \
                meta::_t<std::tuple_element<I, Untagged>> const &NAME() const &      \
                    noexcept(noexcept(                                               \
                        detail::adl_get<I>(std::declval<Untagged const &>())))       \
                {                                                                    \
                    return detail::adl_get<I>(static_cast<Untagged const &>(*this)); \
                }                                                                    \
            };                                                                       \
        };                                                                           \
    }                                                                                \
    /**/

RANGES_DIAGNOSTIC_PUSH
RANGES_DIAGNOSTIC_IGNORE_MISMATCHED_TAGS

namespace std
{
    template<typename Untagged, typename...Tags>
    struct tuple_size< ::ranges::v3::tagged<Untagged, Tags...>>
      : tuple_size<Untagged>
    {};

    template<size_t N, typename Untagged, typename...Tags>
    struct tuple_element<N, ::ranges::v3::tagged<Untagged, Tags...>>
      : tuple_element<N, Untagged>
    {};
}

RANGES_DIAGNOSTIC_POP

#endif
