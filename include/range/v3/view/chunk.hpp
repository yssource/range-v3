/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-present
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_VIEW_CHUNK_HPP
#define RANGES_V3_VIEW_CHUNK_HPP

#include <functional>
#include <limits>
#include <utility>
#include <meta/meta.hpp>
#include <range/v3/begin_end.hpp>
#include <range/v3/iterator_range.hpp>
#include <range/v3/range_concepts.hpp>
#include <range/v3/range_fwd.hpp>
#include <range/v3/range_traits.hpp>
#include <range/v3/view_adaptor.hpp>
#include <range/v3/view_facade.hpp>
#include <range/v3/detail/satisfy_boost_range.hpp>
#include <range/v3/utility/box.hpp>
#include <range/v3/utility/compressed_pair.hpp>
#include <range/v3/utility/optional.hpp>
#include <range/v3/utility/static_const.hpp>
#include <range/v3/view/all.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/view.hpp>

namespace ranges
{
    inline namespace v3
    {
        /// \addtogroup group-views
        /// @{
        template<typename Rng, bool IsForwardRange>
        struct chunk_view_
          : view_adaptor<
                chunk_view_<Rng, IsForwardRange>,
                Rng,
                is_finite<Rng>::value ? finite : range_cardinality<Rng>::value>
        {
        private:
            friend range_access;
            CPP_assert(ForwardRange<Rng>);
            template<typename T>
            using constify = meta::const_if_c<ForwardRange<Rng const>, T>;
            static constexpr bool CanSizedSentinel =
                SizedSentinel<iterator_t<constify<Rng>>, iterator_t<constify<Rng>>>;

            using offset_t =
                meta::if_c<
                    BidirectionalRange<constify<Rng>> || CanSizedSentinel,
                    range_difference_type_t<Rng>,
                    constant<range_difference_type_t<Rng>, 0>>;

            range_difference_type_t<Rng> n_ = 0;

            struct adaptor
              : adaptor_base, private box<offset_t>
            {
            private:
                range_difference_type_t<Rng> n_;
                sentinel_t<constify<Rng>> end_;

                constexpr /*c++14*/
                offset_t const &offset() const
                {
                    offset_t const &result = this->box<offset_t>::get();
                    RANGES_EXPECT(0 <= result && result < n_);
                    return result;
                }
                constexpr /*c++14*/
                offset_t &offset()
                {
                    return const_cast<offset_t &>(const_cast<adaptor const &>(*this).offset());
                }
            public:
                adaptor() = default;
                constexpr adaptor(constify<chunk_view_> &cv)
                  : box<offset_t>{0}
                  , n_((RANGES_EXPECT(0 < cv.n_), cv.n_))
                  , end_(ranges::end(cv.base()))
                {}
                constexpr /*c++14*/
                auto read(iterator_t<constify<Rng>> const &it) const ->
                    decltype(view::take(make_iterator_range(it, end_), n_))
                {
                    RANGES_EXPECT(it != end_);
                    RANGES_EXPECT(0 == offset());
                    return view::take(make_iterator_range(it, end_), n_);
                }
                constexpr /*c++14*/
                void next(iterator_t<constify<Rng>> &it)
                {
                    RANGES_EXPECT(it != end_);
                    RANGES_EXPECT(0 == offset());
                    offset() = ranges::advance(it, n_, end_);
                }
                CPP_member
                constexpr /*c++14*/
                auto prev(iterator_t<constify<Rng>> &it) -> CPP_ret(void)(
                    requires BidirectionalRange<constify<Rng>>)
                {
                    ranges::advance(it, -n_ + offset());
                    offset() = 0;
                }
                CPP_member
                constexpr /*c++14*/
                auto distance_to(
                    iterator_t<constify<Rng>> const &here,
                    iterator_t<constify<Rng>> const &there,
                    adaptor const &that) const ->
                        CPP_ret(range_difference_type_t<Rng>)(
                            requires CanSizedSentinel)
                {
                    auto const delta = (there - here) + (that.offset() - offset());
                    // This can fail for cyclic base ranges when the chunk size does not divide the
                    // cycle length. Such iterator pairs are NOT in the domain of -.
                    RANGES_ENSURE(0 == delta % n_);
                    return delta / n_;
                }
                CPP_member
                constexpr /*c++14*/
                auto advance(iterator_t<constify<Rng>> &it, range_difference_type_t<Rng> n) ->
                    CPP_ret(void)(requires RandomAccessRange<constify<Rng>>)
                {
                    using Limits = std::numeric_limits<range_difference_type_t<Rng>>;
                    if(0 < n)
                    {
                        RANGES_EXPECT(0 == offset());
                        RANGES_EXPECT(n <= Limits::max() / n_);
                        auto const remainder = ranges::advance(it, n * n_, end_) % n_;
                        RANGES_EXPECT(0 <= remainder && remainder < n_);
                        offset() = remainder;
                    }
                    else if(0 > n)
                    {
                        RANGES_EXPECT(n >= Limits::min() / n_);
                        ranges::advance(it, n * n_ + offset());
                        offset() = 0;
                    }
                }
            };

            constexpr /*c++14*/
            adaptor begin_adaptor()
            {
                return adaptor{*this};
            }
            CPP_member
            constexpr auto begin_adaptor() const ->
                CPP_ret(adaptor)(requires ForwardRange<Rng const>)
            {
                return adaptor{*this};
            }
            constexpr /*c++14*/
            range_size_type_t<Rng> size_(range_difference_type_t<Rng> base_size) const
            {
                CPP_assert(SizedRange<Rng>);
                base_size = base_size / n_ + (0 != (base_size % n_));
                return static_cast<range_size_type_t<Rng>>(base_size);
            }
        public:
            chunk_view_() = default;
            constexpr chunk_view_(Rng rng, range_difference_type_t<Rng> n)
              : chunk_view_::view_adaptor(detail::move(rng))
              , n_((RANGES_EXPECT(0 < n), n))
            {}
            CPP_member
            constexpr /*c++14*/
            auto size() const -> CPP_ret(range_size_type_t<Rng>)(
                requires SizedRange<Rng const>)
            {
                return size_(ranges::distance(this->base()));
            }
            CPP_member
            constexpr /*c++14*/
            auto size() -> CPP_ret(range_size_type_t<Rng>)(
                requires SizedRange<Rng> && !SizedRange<Rng const>)
            {
                return size_(ranges::distance(this->base()));
            }
        };

        template<typename Rng>
        struct chunk_view_<Rng, false>
          : view_facade<
                chunk_view_<Rng, false>,
                is_finite<Rng>::value ? finite : range_cardinality<Rng>::value>
        {
        private:
            friend range_access;
            CPP_assert(InputRange<Rng> && !ForwardRange<Rng>);

            using iter_cache_t = detail::non_propagating_cache<iterator_t<Rng>>;

            mutable compressed_tuple<
                Rng,                          // base
                range_difference_type_t<Rng>, // n
                range_difference_type_t<Rng>, // remainder
                iter_cache_t                  // it
            > data_{};

            constexpr /*c++14*/ Rng &base() noexcept { return ranges::get<0>(data_); }
            constexpr Rng const &base() const noexcept { return ranges::get<0>(data_); }
            constexpr /*c++14*/ range_difference_type_t<Rng> &n() noexcept
            {
                return ranges::get<1>(data_);
            }
            constexpr range_difference_type_t<Rng> const &n() const noexcept
            {
                return ranges::get<1>(data_);
            }

            constexpr /*c++14*/ range_difference_type_t<Rng> &remainder() noexcept
            {
                return ranges::get<2>(data_);
            }
            constexpr range_difference_type_t<Rng> const &remainder() const noexcept
            {
                return ranges::get<2>(data_);
            }

            constexpr iter_cache_t &it_cache() const noexcept { return ranges::get<3>(data_); }
            constexpr /*c++14*/ iterator_t<Rng> &it() noexcept { return *it_cache(); }
            constexpr iterator_t<Rng> const &it() const noexcept { return *it_cache(); }

            struct outer_cursor
            {
            private:
                struct inner_view
                  : view_facade<inner_view, finite>
                {
                private:
                    friend range_access;

                    using value_type = range_value_type_t<Rng>;

                    chunk_view_ *rng_ = nullptr;

                    constexpr /*c++14*/
                    bool done() const noexcept
                    {
                        RANGES_EXPECT(rng_);
                        return rng_->remainder() == 0;
                    }
                    constexpr /*c++14*/
                    bool equal(default_sentinel) const noexcept
                    {
                        return done();
                    }
                    constexpr /*c++14*/
                    reference_t<iterator_t<Rng>> read() const
                    {
                        RANGES_EXPECT(!done());
                        return *rng_->it();
                    }
                    constexpr /*c++14*/
                    rvalue_reference_t<iterator_t<Rng>> move() const
                    {
                        RANGES_EXPECT(!done());
                        return ranges::iter_move(rng_->it());
                    }
                    constexpr /*c++14*/
                    void next()
                    {
                        RANGES_EXPECT(!done());
                        ++rng_->it();
                        --rng_->remainder();
                        if(rng_->remainder() != 0 && rng_->it() == ranges::end(rng_->base()))
                            rng_->remainder() = 0;
                    }
                    CPP_member
                    constexpr /*c++14*/
                    auto distance_to(default_sentinel) const ->
                        CPP_ret(range_difference_type_t<Rng>)(
                            requires SizedSentinel<sentinel_t<Rng>, iterator_t<Rng>>)
                    {
                        RANGES_EXPECT(rng_);
                        auto const d = ranges::end(rng_->base()) - rng_->it();
                        return ranges::min(d, rng_->remainder());
                    }
                public:
                    inner_view() = default;
                    constexpr explicit inner_view(chunk_view_ &view) noexcept
                      : rng_{&view}
                    {}
                    CPP_member
                    constexpr /*c++14*/
                    auto size() ->
                        CPP_ret(range_size_type_t<Rng>)(
                            requires SizedSentinel<sentinel_t<Rng>, iterator_t<Rng>>)
                    {
                        auto const d = distance_to(default_sentinel{});
                        return static_cast<range_size_type_t<Rng>>(d);
                    }
                };

                chunk_view_ *rng_ = nullptr;

            public:
                using value_type = inner_view;

                outer_cursor() = default;
                constexpr explicit outer_cursor(chunk_view_ &view) noexcept
                  : rng_{&view}
                {}
                constexpr /*c++14*/
                inner_view read() const
                {
                    RANGES_EXPECT(!done());
                    return inner_view{*rng_};
                }
                constexpr /*c++14*/
                bool done() const
                {
                    RANGES_EXPECT(rng_);
                    return rng_->it() == ranges::end(rng_->base()) && rng_->remainder() != 0;
                }
                constexpr /*c++14*/
                bool equal(default_sentinel) const
                {
                    return done();
                }
                constexpr /*c++14*/
                void next()
                {
                    RANGES_EXPECT(!done());
                    ranges::advance(rng_->it(), rng_->remainder(), ranges::end(rng_->base()));
                    rng_->remainder() = rng_->n();
                }
                CPP_member
                constexpr /*c++14*/
                auto distance_to(default_sentinel) const ->
                    CPP_ret(range_difference_type_t<Rng>)(
                        requires SizedSentinel<sentinel_t<Rng>, iterator_t<Rng>>)
                {
                    RANGES_EXPECT(rng_);
                    auto d = ranges::end(rng_->base()) - rng_->it();
                    if(d < rng_->remainder())
                        return 1;

                    d -= rng_->remainder();
                    d = (d + rng_->n() - 1) / rng_->n();
                    d += (rng_->remainder() != 0);
                    return d;
                }
            };

            constexpr /*c++14*/
            outer_cursor begin_cursor() noexcept
            {
                it_cache() = ranges::begin(base());
                return outer_cursor{*this};
            }
            constexpr /*c++14*/
            range_size_type_t<Rng> size_(range_difference_type_t<Rng> base_size) const
            {
                CPP_assert(SizedRange<Rng>);
                auto const n = this->n();
                base_size = base_size / n + (0 != base_size % n);
                return static_cast<range_size_type_t<Rng>>(base_size);
            }
        public:
            chunk_view_() = default;
            constexpr /*c++14*/
            chunk_view_(Rng rng, range_difference_type_t<Rng> n)
              : data_{detail::move(rng), (RANGES_EXPECT(0 < n), n), n, nullopt}
            {}
            CPP_member
            constexpr /*c++14*/
            auto size() const
                noexcept(noexcept(ranges::distance(std::declval<Rng const &>()))) ->
                CPP_ret(range_size_type_t<Rng>)(
                    requires SizedRange<Rng const>)
            {
                return size_(ranges::distance(base()));
            }
            CPP_member
            constexpr /*c++14*/
            auto size()
                noexcept(noexcept(ranges::distance(std::declval<Rng &>()))) ->
                CPP_ret(range_size_type_t<Rng>)(
                    requires SizedRange<Rng>)
            {
                return size_(ranges::distance(base()));
            }
        };

        template<typename Rng>
        struct chunk_view
          : chunk_view_<Rng, (bool) ForwardRange<Rng>>
        {
            using chunk_view::chunk_view_::chunk_view_;
        };

        namespace view
        {
            // In:  Range<T>
            // Out: Range<Range<T>>, where each inner range has $n$ elements.
            //                       The last range may have fewer.
            struct chunk_fn
            {
            private:
                friend view_access;
                template<typename Int>
                static auto CPP_fun(bind)(chunk_fn chunk, Int n)(
                    requires Integral<Int>)
                {
                    return make_pipeable(std::bind(chunk, std::placeholders::_1, n));
                }
            public:
                CPP_template(typename Rng)(
                    requires InputRange<Rng>)
                chunk_view<all_t<Rng>> operator()(Rng &&rng, range_difference_type_t<Rng> n) const
                {
                    return {all(static_cast<Rng &&>(rng)), n};
                }

                // For the sake of better error messages:
            #ifndef RANGES_DOXYGEN_INVOKED
            private:
                CPP_template(typename Int)(
                    requires not Integral<Int>)
                static detail::null_pipe bind(chunk_fn, Int)
                {
                    CPP_assert_msg(Integral<Int>,
                        "The object passed to view::chunk must be Integral");
                    return {};
                }
            public:
                CPP_template(typename Rng, typename T)(
                    requires not (InputRange<Rng> && Integral<T>))
                void operator()(Rng &&, T) const
                {
                    CPP_assert_msg(InputRange<Rng>,
                        "The first argument to view::chunk must satisfy the InputRange concept");
                    CPP_assert_msg(Integral<T>,
                        "The second argument to view::chunk must satisfy the Integral concept");
                }
            #endif
            };

            /// \relates chunk_fn
            /// \ingroup group-views
            RANGES_INLINE_VARIABLE(view<chunk_fn>, chunk)
        }
        /// @}
    }
}

RANGES_SATISFY_BOOST_RANGE(::ranges::v3::chunk_view)

#endif
