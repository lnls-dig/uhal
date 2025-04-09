#ifndef UTIL_BITS_H
#define UTIL_BITS_H

#include <bit>
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>

static_assert(__cpp_lib_bitops >= 201907L);

template <typename T>
static inline void clear_and_insert(uint32_t &dest, T value, uint32_t mask)
{
    static_assert(std::is_integral_v<T>);

    unsigned shift = std::countr_zero(mask);
    uint32_t shifted_mask = mask >> shift;

    T max;
    T min;
    uint32_t uvalue;
    if constexpr (std::is_signed_v<T>) {
        using U = std::make_unsigned_t<T>;

        uint32_t umax = shifted_mask >> 1;
        /* in the case where mask == UINT32_MAX and value is signed, this would
         * result in umin = INT32_MIN. Assigning that to an int32_t variable to
         * then apply the unary minus operator below would be UB, which we avoid
         * by using an int64_t intermediary value */
        int64_t umin = shifted_mask ^ umax;

        max = umax;
        min = -umin;

        uvalue = (U)value;
    } else {
        max = shifted_mask;
        min = 0;
        uvalue = value;
    }

    if (value < min)
        throw std::runtime_error("value " + std::to_string(value) + " less than min (" + std::to_string(min) + ")");
    if (value > max)
        throw std::runtime_error("value " + std::to_string(value) + " greater than max (" + std::to_string(max) + ")");

    dest &= UINT32_MAX & ~mask;
    dest |= (uvalue << shift) & mask;
}

static inline void insert_bit(uint32_t &dest, bool value, uint32_t mask)
{
    assert(std::popcount(mask) == 1);
    if (value)
        dest |= mask;
    else
        dest &= ~mask;
}

static inline bool get_bit(uint32_t value, uint32_t mask)
{
    assert(std::popcount(mask) == 1);
    return value & mask;
}

template<typename Signed>
static inline int32_t sign_extend(uint32_t value)
{
    using Unsigned = std::make_unsigned_t<Signed>;
    return (Signed)(Unsigned)value;
}

static inline int32_t sign_extend(uint32_t value, unsigned width)
{
    switch (width) {
        case 8:
            return sign_extend<int8_t>(value);
        case 16:
            return sign_extend<int16_t>(value);
        case 32:
            return sign_extend<int32_t>(value);
        default:
            throw std::logic_error("invalid width should have been caught elsewhere");
    }
}

static inline int32_t extract_value(uint32_t value, uint32_t mask, bool is_signed=false)
{
    unsigned shift = std::countr_zero(mask);
    unsigned popcount = std::popcount(mask);

    /* should be using get_bit for single bit masks */
    assert(popcount > 1);
    /* the bit mask should be continuous */
    assert(popcount == 32 - shift - std::countl_zero(mask));

    uint32_t intermediary = (value & mask) >> shift;
    if (is_signed) {
        return sign_extend(intermediary, popcount);
    }
    return intermediary;
}

/* XXX: remove this when there are no more users */
template<typename T>
static inline T extract_value(uint32_t value, uint32_t mask)
{
    return extract_value(value, mask, std::is_signed_v<T>);
}

#endif
