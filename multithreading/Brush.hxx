#pragma once
#include "Infrastructure.hxx"
#include "lib/RGBA.h"

namespace Brush::ImplementationDetail {
	template<typename PointerType>
	struct PlaneView {
		field(Data, static_cast<PointerType>(nullptr));
		field(RowSize, 0_z);

	public:
		auto operator[](std::integral auto y) const {
			return Data + y * RowSize;
		}
	};

	auto Create2DView(auto&& FlattenedStructure) {
		auto Radius = FlattenedStructure.Radius;
		auto AlignedData = FlattenedStructure.Storage.data() + (2 * Radius + 1) * Radius + Radius;
		return PlaneView<decltype(AlignedData)>{ .Data = AlignedData, .RowSize = 2 * Radius + 1 };
	}
}

namespace Brush::IntensityFunctions {
	auto Constant = [](auto...) { return 1; };
	auto Linear = [](auto Radius, auto OffsetY, auto OffsetX) {
		auto EuclideanDistance = std::sqrt(OffsetY * OffsetY + OffsetX * OffsetX);
		return 1. - EuclideanDistance / Radius;
	};
	auto Quadratic = [](auto Radius, auto OffsetY, auto OffsetX) {
		auto [A, B, C] = std::array{ 1. / (Radius * Radius), -2. / Radius, 1. };
		auto EuclideanDistance = std::sqrt(OffsetY * OffsetY + OffsetX * OffsetX);
		return A * EuclideanDistance * EuclideanDistance + B * EuclideanDistance + C;
	};
}

namespace Brush {
	struct CircularMask {
		field(Radius, 0);
		field(Storage, std::vector<double>{});

	public:
		auto Update(auto&& IntensityFunction) {
			Storage.resize((2 * Radius + 1) * (2 * Radius + 1));
			for (auto Self = ImplementationDetail::Create2DView(*this); auto OffsetY : Range{ -Radius, Radius + 1 })
				for (auto OffsetX : Range{ -Radius, Radius + 1 })
					if (OffsetY * OffsetY + OffsetX * OffsetX > Radius * Radius)
						Self[OffsetY][OffsetX] = 0.;
					else
						Self[OffsetY][OffsetX] = IntensityFunction(Radius, OffsetY, OffsetX);
		}
	};

	struct ImagePatch {
		field(Radius, 0);
		field(Storage, std::vector<RGBA>{});

	public:
		auto Update(auto&& Canvas, std::integral auto CenterY, std::integral auto CenterX) {
			Storage.resize((2 * Radius + 1) * (2 * Radius + 1));
			for (auto Self = ImplementationDetail::Create2DView(*this); auto OffsetY : Range{ -Radius, Radius + 1 })
				if (auto AbsoluteY = CenterY + OffsetY; 0 <= AbsoluteY && AbsoluteY < Canvas.Height)
					for (auto OffsetX : Range{ -Radius, Radius + 1 })
						if (auto AbsoluteX = CenterX + OffsetX; 0 <= AbsoluteX && AbsoluteX < Canvas.Width)
							Self[OffsetY][OffsetX] = Canvas[AbsoluteY][AbsoluteX];
						else
							Self[OffsetY][OffsetX].a = 0; // if the pixel we're trying to cache does not exist (out of canvas), make it fully transparent to exclude it from the overlaying process.
				else
					for (auto OffsetX : Range{ -Radius, Radius + 1 })
						Self[OffsetY][OffsetX].a = 0; // same as above, out of canvas -> made transparent.
		}
	};

	auto Overlay(auto&& Canvas, auto&& IntensityMask, auto&& Foreground, std::integral auto CenterY, std::integral auto CenterX) {
		auto [Mask, Coating, Radius] = [&] {
			if constexpr (requires { { Foreground }->SubtypeOf<RGBA>; })
				return std::tuple{ ImplementationDetail::Create2DView(IntensityMask), 42, IntensityMask.Radius };
			else
				return std::tuple{ ImplementationDetail::Create2DView(IntensityMask), ImplementationDetail::Create2DView(Foreground), IntensityMask.Radius };
		}();
		for (auto OffsetY : Range{ -Radius, Radius + 1 })
			if (auto AbsoluteY = CenterY + OffsetY; 0 <= AbsoluteY && AbsoluteY < Canvas.Height)
				for (auto OffsetX : Range{ -Radius, Radius + 1 })
					if (auto AbsoluteX = CenterX + OffsetX; 0 <= AbsoluteX && AbsoluteX < Canvas.Width) {
						auto [CoatingR, CoatingG, CoatingB, CoatingA] = [&] {
							if constexpr (requires { { Foreground }->SubtypeOf<RGBA>; })
								return std::tuple{ Foreground.r, Foreground.g, Foreground.b, Foreground.a };
							else
								return std::tuple{ Coating[OffsetY][OffsetX].r, Coating[OffsetY][OffsetX].g, Coating[OffsetY][OffsetX].b, Coating[OffsetY][OffsetX].a };
						}();
						auto CoatingIntensity = Mask[OffsetY][OffsetX] * CoatingA / 255.;
						Canvas[AbsoluteY][AbsoluteX].r = Canvas[AbsoluteY][AbsoluteX].r * (1 - CoatingIntensity) + CoatingR * CoatingIntensity;
						Canvas[AbsoluteY][AbsoluteX].g = Canvas[AbsoluteY][AbsoluteX].g * (1 - CoatingIntensity) + CoatingG * CoatingIntensity;
						Canvas[AbsoluteY][AbsoluteX].b = Canvas[AbsoluteY][AbsoluteX].b * (1 - CoatingIntensity) + CoatingB * CoatingIntensity;
					}
		if constexpr (requires { Foreground.Update(Canvas, CenterY, CenterX); })
			Foreground.Update(Canvas, CenterY, CenterX);
	}
}