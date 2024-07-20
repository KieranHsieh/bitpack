# Bitpack

Bitpack is a single header library for packing data into unsigned integer types. It focuses on providng safety, reliability, speed, and readability.

# Compile-Time Impact

Since Bitpack makes heavy use of templates, you should expect some level of increase to compile times with extremely heavy use of the library. However, since it avoids common patterns like recursion, the overall effect should be fairly negligible.

Note that the compile time impact of this library hasn't been benchmarked and what is stated above is just an educated guess.

# Limits

- When using the clang compiler, you may run into limits with fold expressions when creating bitpacks with more than 256 fields. To get around this, you can set the bracket-depth compiler flag to be larger.
- This library currently doesn't support storage requiring more than 64 bits.

# Examples

## Basic Usage
```cpp
#include <bitpack/bitpack.hpp>
#include <iostream>

// This declares a layout of 8 bits, followed by 16 bits, followed again by 32 bits.
// The data type used to store this is calculated as the smallest type that can store all of the input fields:
//      In this case, we see 8 + 16 + 32 = 56, so uint_fast64_t is automatically detected as the most appropriate storage type.
//
// Note here the usage of bitpack::fast_layout. This changes the internal types used to be the uint_fastX_t variety
// rather than the uint_leastX_t variety. The alias for smaller types is bitpack::small_layout.
using packet_layout = bitpack::fast_layout<bitpack::bitwidth<8>, bitpack::bitwidth<16>, bitpack::bitwidth<32>>;

// This is an optional declaration of an enum that can be used to access the individual bitpack fields, the value
// of each enum member is used as the index into the previously declared layout.
enum class packet_idx {
    Id = 0,
    Meta = 1,
    Content = 2,
};

// This is an optional type alias to make it more clear about what we're working with
using packet = bitpack::bitpack<packet_layout>;

int main() {
    packet incoming_packet{};
    // This sets the "Id" (Field 0 with a bit width of 8) to have a value of 8
    incoming_packet.set<packet_idx::Id>(8);
    // This sets the "Content" (Field 2 with a bit width of 32) to have a value of 1000
    incoming_packet.set<packet_idx::Content>(1000);
    // incoming_packet.set<100>(90); <-- This results in a compile time error

    // Bitpack fields can also be accessed by index directly:
    // incoming_packet.set<0>(8);

    // Cast here because the return type is a uint8, and won't be interpreted as a number when printing
    std::cout << "Id: " << static_cast<unsigned int>(incoming_packet.get<packet_idx::Id>()) << "\n"; // 8
    std::cout << "Meta: " << incoming_packet.get<packet_idx::Meta>() << "\n"; // 0
    std::cout << "Content: " << incoming_packet.get<packet_idx::Content>() << "\n"; // 1000
}
```

## Custom Detectors

There are some use cases where you might want to directly control the storage type used by bitpack. To do this, you should implement your own storage detector:

```cpp
// Define a detector that always uses uint64_t
template<bitpack::storage_preference P, size_t SIZE>
struct my_storage_detector {
    using type = uint64_t;
};

// Define a specialization that uses uint16_t for sizes of 4 bits
template<bitpack::storage_preference P>
struct my_storage_detector<P, 4> {
    using type = uint16_t;
};

// Define a layout
using layout = bitpack::fast_layout<bitpack::bitwidth<8>, bitpack::bitwidth<4>>

// Define a bitpack using your custom detector.
// As defined by the detector above, the bitpack will store everything in a single uint64_t, and when accessing
// the second field, the result will be returned in a uint16_t.
using pack = bitpack::bitpack<layout, my_storage_detector>;

int main() {
    pack my_pack{};
    my_pack.get<0>(); // uint64_t
    my_pack.get<1>(); // uint16_t
}
```

It should also be noted that custom types can be used by your detectors. However, they must fulfill the following requirements:
- Constructible via your_type(int)
- Supports the `&`, `<<`, `>>`, `|`, and `~` operators.
- Supports addition
- Supports implicit conversion from the result of std::underlying_type_t for an enum class.

# Extra Utilities

Besides the `bitpack` type itself, this library also provides the `bitmask_t` and `bitmask_v` types, which can be used to create an N bit mask.

```cpp
int main() {
    // 0b11
    auto my_mask = bitpack::bitmask_v<size_t, 2>;
    // 0b111
    auto my_mask_2 = bitpack::bitmask_v<size_t, 3>;
}
```

# Future Work

Some things still missing from this library:
- Default support for storage needs greater than 64 bits.
- Type safety for bitpack enum accessors