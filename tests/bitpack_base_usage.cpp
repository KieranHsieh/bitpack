#include <array>
#include <bitpack/bitpack.hpp>
#include <climits>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <cstdint>

int main() {
    // Test that bitmask is correctly creating masks
    static_assert(bitpack::bitmask_v<size_t, 1> == 1);
    static_assert(bitpack::bitmask_v<size_t, 2> == 3);
    static_assert(bitpack::bitmask_v<size_t, 3> == 7);

    // Check that data in the bitpack::layout is calculated correctly
    using pack_layout = bitpack::layout<bitpack::storage_preference::SMALL, bitpack::bitwidth<8>, bitpack::bitwidth<9>>;
    static_assert(pack_layout::field_sizes.size() == 2);
    static_assert(pack_layout::field_sizes[0] == 8);
    static_assert(pack_layout::field_sizes[1] == 9);
    static_assert(bitpack::layout_traits<pack_layout>::total_bitwidth == 17);

    // Check that the default storage detector correctly detects which type to use
    static_assert(
        std::is_same_v<bitpack::layout_storage_detector<bitpack::storage_preference::SMALL, 1>::type, std::uint_least8_t>);
    static_assert(
        std::is_same_v<bitpack::layout_storage_detector<bitpack::storage_preference::SMALL, 9>::type, std::uint_least16_t>);
    static_assert(
        std::is_same_v<bitpack::layout_storage_detector<bitpack::storage_preference::SMALL, 17>::type, std::uint_least32_t>);

    static_assert(
        std::is_same_v<bitpack::layout_storage_detector<bitpack::storage_preference::FAST, 1>::type, std::uint_fast8_t>);
    static_assert(
        std::is_same_v<bitpack::layout_storage_detector<bitpack::storage_preference::FAST, 9>::type, std::uint_fast16_t>);
    static_assert(
        std::is_same_v<bitpack::layout_storage_detector<bitpack::storage_preference::FAST, 17>::type, std::uint_fast32_t>);

    // Test the constexpr accumulate implementation in the detail namespace
    constexpr std::array<size_t, 4> arr = { 1, 2, 3, 4 };
    static_assert(bitpack::detail::accumulate(arr.begin(), arr.end(), 0) == 10);
    static_assert(bitpack::detail::accumulate(arr.begin(), arr.begin() + 2, 0) == 3);

    // Test that bitpack accesses and assignments work correctly
    auto bitpack                        = bitpack::bitpack<pack_layout> {};

    assert(bitpack.get<0>() == 0);
    assert(bitpack.get<1>() == 0);
    bitpack.set<0>(255);
    assert(bitpack.get<0>() == 255);
    assert(bitpack.get<1>() == 0);
    bitpack.set<1>(511);
    assert(bitpack.get<0>() == 255);
    assert(bitpack.get<1>() == 511);
    bitpack.set<1>(3);
    bitpack.set<0>(1);
    assert(bitpack.get<0>() == 1);
    assert(bitpack.get<1>() == 3);

    // Test that we can access and assign through enums.
    enum class PacketIdx {
        Header  = 0,
        Content = 1
    };

    bitpack.set<PacketIdx::Header>(1);
    bitpack.set<PacketIdx::Content>(8);
    assert(bitpack.get<PacketIdx::Header>() == 1);
    assert(bitpack.get<PacketIdx::Content>() == 8);

    using layout_1 = bitpack::fast_layout<bitpack::bitwidth<12>, bitpack::bitwidth<8>>;
    using layout_2 = bitpack::fast_layout<bitpack::bitwidth<4>, bitpack::bitwidth<4>, bitpack::bitwidth<4>>;
    using pack1 = bitpack::bitpack<layout_1>;
    using pack2 = bitpack::bitpack<layout_2>;

    // Test that a pack can be constructed from it's storage_type
    pack1 pack_1{};
    pack_1.set<0>(1);
    pack2 pack_2(pack_1.get<0>());
    assert(pack_2.data == 1);

    std::cout << "Tests passed!\n";

    return 0;
}