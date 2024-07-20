/*
MIT License

Copyright (c) 2023 Kieran Hsieh

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef BITPACK_HPP
#define BITPACK_HPP

#include <cassert>
#include <limits.h>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>

static_assert(__cplusplus >= 201703L, "C++ Standard must be at least C++17");

namespace bitpack {
    namespace detail {
        /**
         * @brief Simple metafunction for comparing two compile time constants with the <= operator
         *
         * @tparam X The lhs of the operation
         * @tparam Y The rhs of the operation
         */
        template<auto X, auto Y>
        struct is_less_equal {
            /**
             * @brief If X is less than or equal to Y
             *
             */
            static constexpr bool value = X <= Y;
        };

        /**
         * @brief Helper alias for is_less_equal<X, Y>::value
         *
         * @tparam X The lhs of the operation
         * @tparam Y The rhs of the operation
         */
        template<auto X, auto Y>
        constexpr bool is_less_equal_v = is_less_equal<X, Y>::value;

        // Simple metafunction for comparing two compile time constants
        template<auto X, auto Y>
        struct is_greater {
            /**
             * @brief If X is greater than Y
             *
             */
            static constexpr bool value = X > Y;
        };

        // Helper alias for is_greater<X, Y>::value
        template<auto X, auto Y>
        constexpr bool is_greater_v = is_greater<X, Y>::value;

        // Helper type to allow raw false static_assert without failing compilation until it's instantiated.
        template<typename T>
        struct always_false : std::false_type { };

        // Compile time alternative to std::accumulate (which is not constexpr until c++20)
        template<typename IT, typename T>
        constexpr T accumulate(IT first, IT last, T init) {
            for(; first != last; ++first) { init = std::move(init) + *first; }
            return init;
        }
    }

    /**
     * @brief Constructs a bitmask of type T with a width of W
     *
     * @tparam T The type of the bitmask
     * @tparam W The width of the bitmask
     */
    template<typename T, T W>
    struct bitmask_t {
        /**
         * @brief The bitmask
         *
         */
        static constexpr T value = (T(1) << (W)) - T(1);
    };

    /**
     * @brief Helper alias for bitmask_t<T, W>::value
     *
     * @tparam T The type of the bitmask
     * @tparam W The width of the bitmask
     */
    template<typename T, T W>
    constexpr T bitmask_v = bitmask_t<T, W>::value;

    /**
     * @brief Marker type for bitpack field widths
     *
     * @tparam W The number of bits in the field
     */
    template<size_t W>
    struct bitwidth {
        /**
         * @brief The number of bits int he field
         *
         */
        static constexpr size_t width = W;
    };

    /**
     * @brief Compile time check to test if a type is a specialization of bitwidth
     *
     * @tparam T The type to check
     */
    template<typename T>
    struct is_bitwidth : std::false_type { };

    /**
     * @brief Compile time check to test if a type is a specialization of bitwidth
     *
     * @tparam W The width of the bitwidth
     */
    template<size_t W>
    struct is_bitwidth<bitwidth<W>> : std::true_type { };

    /**
     * @brief Helper alias for is_bitwidth<T>::value
     *
     * @tparam T The type to test
     */
    template<typename T>
    constexpr bool is_bitwidth_v = is_bitwidth<T>::value;

    /**
     * @brief Marker type used to designate a preference for fast or small storage
     *
     */
    enum class storage_preference {
        FAST,
        SMALL
    };

    /**
     * @brief Layout type that determines the fields and their widths in a bitpack
     *
     * @tparam P The users preference for fast or small storage
     * @tparam FIELDS The fields of the layout.
     */
    template<storage_preference P, typename... FIELDS>
    struct layout {
        static_assert((is_bitwidth_v<FIELDS> && ... && true), "layout fields must be a bitwidth type.");

        /**
         * @brief The storage preference (fast or small)
         *
         */
        static constexpr storage_preference storage_preference = P;

        /**
         * @brief The width in bits of each field in the layout
         *
         */
        static constexpr std::array<size_t, sizeof...(FIELDS)> field_sizes = { FIELDS::width... };
    };

    /**
     * @brief Alias for a bitpack layout with a fast storage preference
     *
     * @tparam FIELDS The fields of the layout
     */
    template<typename... FIELDS>
    using fast_layout = layout<storage_preference::FAST, FIELDS...>;

    /**
     * @brief An alias for a bitpack layout with a small storage preference
     *
     * @tparam FIELDS The fields of the layout
     */
    template<typename... FIELDS>
    using small_layout = layout<storage_preference::SMALL, FIELDS...>;

    namespace detail {
        /*

            Implementation details for the default layout_storage_detector. Checks the size of each type and returns either
            uint_fast/leastX_t depending on the user's storage preference.

            There is currently no implementation for types larger than 64 bits.

        */

        template<storage_preference P, size_t TOTAL_SIZE, typename E = void>
        struct layout_storage_detector_2;

        template<storage_preference P, size_t TOTAL_SIZE>
        struct layout_storage_detector_2<P, TOTAL_SIZE, std::enable_if_t<(detail::is_less_equal_v<TOTAL_SIZE, 8>), void>> {
            /**
             * @brief Either uint_fast8_t or uint_least8_t depending on P
             *
             */
            using type = std::conditional_t<P == storage_preference::FAST, std::uint_fast8_t, std::uint_least8_t>;
        };

        template<storage_preference P, size_t TOTAL_SIZE>
        struct layout_storage_detector_2<
            P,
            TOTAL_SIZE,
            std::enable_if_t<(detail::is_less_equal_v<TOTAL_SIZE, 16> && detail::is_greater_v<TOTAL_SIZE, 8>), void>> {
            /**
             * @brief Either uint_fast16_t or uint_least16_t depending on P
             *
             */
            using type = std::conditional_t<P == storage_preference::FAST, std::uint_fast16_t, std::uint_least16_t>;
        };

        template<storage_preference P, size_t TOTAL_SIZE>
        struct layout_storage_detector_2<
            P,
            TOTAL_SIZE,
            std::enable_if_t<(detail::is_less_equal_v<TOTAL_SIZE, 32> && detail::is_greater_v<TOTAL_SIZE, 16>), void>> {
            /**
             * @brief Either uint_fast32_t or uint_least32_t depending on P
             *
             */
            using type = std::conditional_t<P == storage_preference::FAST, std::uint_fast32_t, std::uint_least32_t>;
        };

        template<storage_preference P, size_t TOTAL_SIZE>
        struct layout_storage_detector_2<
            P,
            TOTAL_SIZE,
            std::enable_if_t<(detail::is_less_equal_v<TOTAL_SIZE, 64> && detail::is_greater_v<TOTAL_SIZE, 32>), void>> {
            /**
             * @brief Either uint_fast64_t or uint_least64_t depending on P
             *
             */
            using type = std::conditional_t<P == storage_preference::FAST, std::uint_fast64_t, std::uint_least64_t>;
        };

        template<storage_preference P, size_t TOTAL_SIZE>
        struct layout_storage_detector_2<P, TOTAL_SIZE, std::enable_if_t<(detail::is_greater_v<TOTAL_SIZE, 64>), void>> {
            static_assert(detail::always_false<std::integral_constant<size_t, TOTAL_SIZE>>::value,
                          "Storage for greater than 64 bits is not implemented");
        };
    }

    /**
     * @brief The default detector used by bitpacks. This will return either uint_fast/leastX_t depending on the layout's storage
     * preference.
     *
     * @tparam P The storage preference
     * @tparam TOTAL_SIZE The number of bits that storage is being chosen for
     */
    template<storage_preference P, size_t TOTAL_SIZE>
    using layout_storage_detector = detail::layout_storage_detector_2<P, TOTAL_SIZE>;

    /**
     * @brief Trait type used to get information about bitpack layouts
     *
     * @tparam T The layout
     * @tparam D The storage detector
     */
    template<typename T, template<storage_preference, size_t> typename D = layout_storage_detector>
    struct layout_traits;

    /**
     * @brief Trait implementation for all types
     *
     * @tparam P The user's storage_preference
     * @tparam D The storage detector
     * @tparam FIELDS The fields in the layout
     */
    template<storage_preference P, template<storage_preference, size_t> typename D, typename... FIELDS>
    struct layout_traits<layout<P, FIELDS...>, D> {
        /**
         * @brief The total number of bits requried by the layout
         *
         */
        static constexpr size_t total_bitwidth = (FIELDS::width + ... + 0);

        /**
         * @brief The type used to store all of the bits in the bitpack
         *
         */
        using storage_type = typename D<P, total_bitwidth>::type;

        static_assert(sizeof(storage_type) * CHAR_BIT >= total_bitwidth, "The storage type is not able to store enough bits");
    };

    template<typename L, template<storage_preference, size_t> typename D = layout_storage_detector>
    struct bitpack {
    public:
        /**
         * @brief The type used to store data
         *
         */
        using storage_type = typename layout_traits<L, D>::storage_type;

        /**
         * @brief The actual data stored by the bitpack
         *
         */
        storage_type data = 0;

    private:
        /**
         * @brief Converts an unknown index type I (Either enum or integral) to a size_t value.
         *
         * @tparam I The index
         * @return constexpr size_t The converted index
         */
        template<auto I>
        static constexpr size_t _index_to_sizet() {
            static_assert(std::is_enum_v<decltype(I)> || std::is_integral_v<decltype(I)>, "Index must be numeric or enum type");
            using index_type = decltype(I);
            if constexpr(std::is_enum_v<index_type>) {
                return static_cast<std::underlying_type_t<index_type>>(I);
            }
            else {
                return I;
            }
        }

        /**
         * @brief Uses the layout_storage_detector to determine the storage type that should be used at index I
         *
         * @tparam I The index
         */
        template<auto I>
        using storage_at = typename layout_storage_detector<L::storage_preference, L::field_sizes[_index_to_sizet<I>()]>::type;

    public:
        /**
         * @brief Constructs the bitpack with a default value of 0
         *
         */
        constexpr bitpack() = default;

        /**
         * @brief Constructs the bitpack with a value of val
         *
         */
        constexpr bitpack(storage_type val) : data(val) { }

        /**
         * @brief Gets the value stored in the bitpack at field index I
         *
         * @tparam I The index of the data being accessed
         * @tparam R The return type. This defaults to what is detected by the bitpack's detector
         * @return constexpr R The data stored at field I
         */
        template<auto I, typename R = storage_at<I>>
        constexpr R get() const noexcept {
            constexpr size_t index        = _index_to_sizet<I>();
            constexpr auto unshifted_mask = bitmask_v<storage_type, L::field_sizes[index]>;
            constexpr auto shift = detail::accumulate(L::field_sizes.begin(), L::field_sizes.begin() + index, storage_type(0));
            constexpr storage_type mask = unshifted_mask << shift;
            return (data & mask) >> shift;
        }

        /**
         * @brief Sets the data at bitpack field index I
         *
         * @tparam I The index of the data being accessed
         * @param value The value of the data at field I
         */
        template<auto I>
        constexpr void set(storage_at<I> value) noexcept {
            constexpr size_t index        = _index_to_sizet<I>();
            constexpr auto unshifted_mask = bitmask_v<storage_type, L::field_sizes[index]>;
            constexpr auto shift = detail::accumulate(L::field_sizes.begin(), L::field_sizes.begin() + index, storage_type(0));
            constexpr storage_type mask = unshifted_mask << shift;

            // Debug check to make sure that data isn't overflowing
            assert((value & unshifted_mask) == value &&
                   "The input value overflows the bitwidth associated with the provided index");
            data &= ~mask;
            data |= (value & unshifted_mask) << shift;
        }
    };
}

#endif