#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <tuple>
#include <array>
#include <unordered_map>
#include <type_traits>
#include <functional>
#include <algorithm>
#include <numbers>
#include <concepts>
#include <limits>
#include <utility>
#include <memory>
#include <initializer_list>
#include <new>
#include <stdexcept>
#include <cstring>
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <cstdlib>

#define field(FieldIdentifier, ...) std::decay_t<decltype(__VA_ARGS__)> FieldIdentifier = __VA_ARGS__
#define Forward(...) std::forward<decltype(__VA_ARGS__)>(__VA_ARGS__)

using namespace std::literals;

constexpr auto operator""_u64(unsigned long long Value) {
	return static_cast<std::uint64_t>(Value);
}

constexpr auto operator""_i64(unsigned long long Value) {
	return static_cast<std::int64_t>(Value);
}

constexpr auto operator""_u32(unsigned long long Value) {
	return static_cast<std::uint32_t>(Value);
}

constexpr auto operator""_i32(unsigned long long Value) {
	return static_cast<std::int32_t>(Value);
}

constexpr auto operator""_z(unsigned long long Value) {
	return static_cast<std::ptrdiff_t>(Value);
}

constexpr auto operator""_uz(unsigned long long Value) {
	return static_cast<std::size_t>(Value);
}

struct Range {
	field(Startpoint, 0_z);
	field(Endpoint, 0_z);
	field(Step, 1_z);

private:
	struct Iterator {
		field(Cursor, 0_z);
		field(Step, 0_z);

	public:
		constexpr auto operator*() const {
			return Cursor;
		}
		constexpr auto& operator++() {
			Cursor += Step;
			return *this;
		}
		constexpr auto operator!=(auto&& OtherIterator) const {
			if (Step > 0)
				return Cursor < OtherIterator.Cursor;
			else
				return Cursor > OtherIterator.Cursor;
		}
	};

public:
	constexpr Range() = default;
	constexpr Range(std::integral auto Endpoint) {
		if (Endpoint < 0)
			this->Step = -1;
		this->Endpoint = Endpoint;
	}
	constexpr Range(std::integral auto Startpoint, std::integral auto Endpoint) {
		if (Startpoint > Endpoint)
			this->Step = -1;
		this->Startpoint = Startpoint;
		this->Endpoint = Endpoint;
	}
	constexpr Range(std::integral auto Startpoint, std::integral auto Endpoint, std::integral auto Step) {
		this->Startpoint = Startpoint;
		this->Endpoint = Endpoint;
		this->Step = Step;
	}

public:
	constexpr auto begin() const {
		return Iterator{ .Cursor = Startpoint, .Step = Step };
	}
	constexpr auto end() const {
		return Iterator{ .Cursor = Endpoint, .Step = Step };
	}
};

template<typename UnknownType, typename ReferenceType>
concept SubtypeOf = std::same_as<std::decay_t<UnknownType>, ReferenceType> || std::derived_from<std::decay_t<UnknownType>, ReferenceType>;

template<typename UnknownType, typename ...ReferenceTypes>
concept AnyOf = (SubtypeOf<UnknownType, ReferenceTypes> || ...);

template<typename UnknownType, typename ...ReferenceTypes>
concept AnyBut = !AnyOf<UnknownType, ReferenceTypes...>;

template<typename UnknownType, typename ReferenceType>
concept ExplicitlyConvertibleTo = requires(UnknownType x) { static_cast<ReferenceType>(Forward(x)); };

template<typename UnknownType, typename ReferenceType>
concept ConstructibleFrom = ExplicitlyConvertibleTo<ReferenceType, std::decay_t<UnknownType>>;

template<typename UnknownType>
concept BuiltinArray = std::is_array_v<std::remove_cvref_t<UnknownType>>;

template<typename UnknownType>
concept Advanceable = requires(UnknownType x) { ++x; };

template<typename UnknownType>
concept Iterable = BuiltinArray<UnknownType> || requires(UnknownType x) {
	{ x.begin() }->Advanceable;
	{ *x.begin() }->AnyBut<void>;
	{ x.begin() != x.end() }->ExplicitlyConvertibleTo<bool>;
};

template<typename UnknownType>
concept Countable = std::integral<std::decay_t<UnknownType>> || std::is_enum_v<std::decay_t<UnknownType>>;

template<typename UnknownType>
concept Real = std::integral<std::decay_t<UnknownType>> || std::floating_point<std::decay_t<UnknownType>>;

auto& operator+=(Iterable auto&& Self, Iterable auto&& Items) requires requires { Self.insert(Self.end(), Items.begin(), Items.end()); } {
	Self.insert(Self.end(), Items.begin(), Items.end());
	return Self;
}

auto operator+(Iterable auto&& PrimaryContainer, Iterable auto&& Items) requires requires { PrimaryContainer.insert(PrimaryContainer.end(), Items.begin(), Items.end()); } {
	auto PrimaryContainerReplica = Forward(PrimaryContainer);
	PrimaryContainerReplica.insert(PrimaryContainerReplica.end(), Items.begin(), Items.end());
	return PrimaryContainerReplica;
}

auto operator+(auto&& LeftHandSideOperand, auto&& RightHandSideOperand) requires requires { std::tuple_cat(Forward(LeftHandSideOperand), Forward(RightHandSideOperand)); } {
	return std::tuple_cat(Forward(LeftHandSideOperand), Forward(RightHandSideOperand));
}

namespace Utility::Arithmetic {
	auto Max(auto x, auto y) {
		return x > y ? x : y;
	}
	auto Min(auto x, auto y) {
		return x < y ? x : y;
	}
}

namespace Utility::Reflection::Private {
	template<typename, typename>
	struct ContainerReplaceTypeArgument {};
	template<template<typename> typename ContainerTypeConstructor, typename TypeBeingReplaced, typename TargetElementType>
	struct ContainerReplaceTypeArgument<ContainerTypeConstructor<TypeBeingReplaced>, TargetElementType> {
		using ReassembledType = ContainerTypeConstructor<TargetElementType>;
	};
	template<template<typename, auto> typename ContainerTypeConstructor, typename TypeBeingReplaced, auto Length, typename TargetElementType>
	struct ContainerReplaceTypeArgument<ContainerTypeConstructor<TypeBeingReplaced, Length>, TargetElementType> {
		using ReassembledType = ContainerTypeConstructor<TargetElementType, Length>;
	};
	template<template<typename, typename> typename ContainerTypeConstructor, template<typename> typename AllocatorTypeConstructor, typename TypeBeingReplaced, typename TargetElementType>
	struct ContainerReplaceTypeArgument<ContainerTypeConstructor<TypeBeingReplaced, AllocatorTypeConstructor<TypeBeingReplaced>>, TargetElementType> {
		using ReassembledType = ContainerTypeConstructor<TargetElementType, AllocatorTypeConstructor<TargetElementType>>;
	};
}

namespace Utility::Reflection {
	template<typename ReferenceContainerType, typename TargetElementType>
	using ContainerReplaceElementType = Private::ContainerReplaceTypeArgument<std::decay_t<ReferenceContainerType>, TargetElementType>::ReassembledType;
}

namespace Utility::ContainerManipulators {
	auto Distance(auto&& Startpoint, auto&& Endpoint) {
		if constexpr (requires { { Endpoint - Startpoint }->ExplicitlyConvertibleTo<std::ptrdiff_t>; })
			return static_cast<std::ptrdiff_t>(Endpoint - Startpoint);
		else {
			auto DistanceCounter = 0_z;
			for (auto Cursor = Forward(Startpoint); Cursor != Endpoint; ++Cursor)
				++DistanceCounter;
			return DistanceCounter;
		}
	}
	template<typename TargetContainerType>
	auto MapWithNaturalTransformation(auto&& TransformationForEachElement, auto&& SourceContainer) {
		constexpr auto SourceIsMovable = std::is_rvalue_reference_v<decltype(SourceContainer)>;
		auto TargetContainer = TargetContainerType{};
		auto ApplyElementWiseTransformation = [&](auto&& Value) {
			if constexpr (SourceIsMovable)
				return TransformationForEachElement(std::move(Value));
			else
				return TransformationForEachElement(Value);
		};
		auto ConstructPlaceholderForTransformedElement = [&] {
			if constexpr (requires { { SourceContainer }->BuiltinArray; })
				return ApplyElementWiseTransformation(SourceContainer[0]);
			else
				return ApplyElementWiseTransformation(*SourceContainer.begin());
		};
		auto EstimateSourceContainerSize = [&] {
			if constexpr (requires { { SourceContainer }->BuiltinArray; })
				return sizeof(SourceContainer) / sizeof(SourceContainer[0]);
			else if constexpr (requires { { SourceContainer.size() }->std::integral; })
				return SourceContainer.size();
			else
				return Distance(SourceContainer.begin(), SourceContainer.end());
		};
		auto CacheTransformedElements = [&] {
			using TransformedElementType = std::decay_t<decltype(ConstructPlaceholderForTransformedElement())>;
			auto CachedElements = std::vector<TransformedElementType>{};
			CachedElements.reserve(EstimateSourceContainerSize());
			for (auto&& x : SourceContainer)
				CachedElements.push_back(ApplyElementWiseTransformation(x));
			return CachedElements;
		};
		auto ExtractCursors = [&] {
			if constexpr (requires { { SourceContainer }->BuiltinArray; })
				return std::tuple{ TargetContainer.begin(), SourceContainer };
			else
				return std::tuple{ TargetContainer.begin(), SourceContainer.begin() };
		};
		if constexpr (requires { TargetContainer.push_back(ConstructPlaceholderForTransformedElement()); })
			for (auto&& x : SourceContainer)
				TargetContainer.push_back(ApplyElementWiseTransformation(x));
		else if constexpr (requires { TargetContainer.insert(ConstructPlaceholderForTransformedElement()); })
			for (auto&& x : SourceContainer)
				TargetContainer.insert(ApplyElementWiseTransformation(x));
		else if constexpr (requires { TargetContainer.push_front(ConstructPlaceholderForTransformedElement()); })
			for (auto CachedElements = CacheTransformedElements(); CachedElements.empty() == false; CachedElements.pop_back())
				TargetContainer.push_front(std::move(CachedElements.back()));
		else
			for (auto [TargetContainerCursor, SourceContainerCursor] = ExtractCursors(); auto _ : Range{ Arithmetic::Min(EstimateSourceContainerSize(), Distance(TargetContainer.begin(), TargetContainer.end())) }) {
				*TargetContainerCursor = ApplyElementWiseTransformation(*SourceContainerCursor);
				++TargetContainerCursor;
				++SourceContainerCursor;
			}
		return TargetContainer;
	}
	auto fMap(auto&& TransformationForEachElement, auto&& SourceContainer) {
		using TransformedElementType = decltype(TransformationForEachElement(*SourceContainer.begin()));
		using TargetContainerType = Reflection::ContainerReplaceElementType<decltype(SourceContainer), TransformedElementType>;
		return MapWithNaturalTransformation<TargetContainerType>(Forward(TransformationForEachElement), Forward(SourceContainer));
	}
}