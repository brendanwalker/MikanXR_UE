#pragma once

#include <type_traits>

// Enable bitwise operations for enum classes
template <typename Enum>
struct EnableBitMaskOperators : std::false_type
{
};

template <typename Enum>
constexpr bool EnableBitMaskOperatorsV= EnableBitMaskOperators<Enum>::value;

// Bitwise OR
template <typename Enum>
constexpr typename std::enable_if_t<EnableBitMaskOperatorsV<Enum>, Enum> operator|(Enum lhs, Enum rhs)
{
	using Underlying= typename std::underlying_type_t<Enum>;
	return static_cast<Enum>(static_cast<Underlying>(lhs) | static_cast<Underlying>(rhs));
}

// Bitwise AND
template <typename Enum>
constexpr typename std::enable_if_t<EnableBitMaskOperatorsV<Enum>, Enum> operator&(Enum lhs, Enum rhs)
{
	using Underlying= typename std::underlying_type_t<Enum>;
	return static_cast<Enum>(static_cast<Underlying>(lhs) & static_cast<Underlying>(rhs));
}

// Bitwise OR assignment
template <typename Enum>
constexpr typename std::enable_if_t<EnableBitMaskOperatorsV<Enum>, Enum&> operator|=(Enum& lhs, Enum rhs)
{
	lhs= lhs | rhs;
	return lhs;
}

// Bitwise AND assignment
template <typename Enum>
constexpr typename std::enable_if_t<EnableBitMaskOperatorsV<Enum>, Enum&> operator&=(Enum& lhs, Enum rhs)
{
	lhs= lhs & rhs;
	return lhs;
}

// Bitwise NOT
template <typename Enum>
constexpr typename std::enable_if_t<EnableBitMaskOperatorsV<Enum>, Enum> operator~(Enum value)
{
	using Underlying= typename std::underlying_type_t<Enum>;
	return static_cast<Enum>(~static_cast<Underlying>(value));
}

template <typename Enum>
constexpr typename std::enable_if_t<EnableBitMaskOperatorsV<Enum>, bool> has_any_bits_set(Enum value)
{
	using Underlying= typename std::underlying_type_t<Enum>;
	return static_cast<Underlying>(value) != 0;
}

#define DEFINE_ENUM_BITMASK_OPERATORS(EnumType)                                                                        \
	template <>                                                                                                        \
	struct EnableBitMaskOperators<EnumType> : std::true_type                                                           \
	{                                                                                                                  \
	};