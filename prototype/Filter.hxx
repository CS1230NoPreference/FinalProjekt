#pragma once
#include "Frame.hxx"
#include "lib/RGBA.h"

namespace Filter::DisplayPort {
	auto ConstructFrameFrom(auto&& PackedRGBSource) requires requires {
		{ PackedRGBSource[0][0] }->SubtypeOf<RGBA>;
		{ PackedRGBSource.Height* PackedRGBSource.Width }->std::integral;
	} {
		auto ConvertedFrame = Frame{ PackedRGBSource.Height, PackedRGBSource.Width, 3 };
		for (auto y : Range{ PackedRGBSource.Height })
			for (auto x : Range{ PackedRGBSource.Width }) {
				ConvertedFrame[0][y][x] = PackedRGBSource[y][x].r / 255.;
				ConvertedFrame[1][y][x] = PackedRGBSource[y][x].g / 255.;
				ConvertedFrame[2][y][x] = PackedRGBSource[y][x].b / 255.;
			}
		return ConvertedFrame.Finalize();
	}
	auto Transfer(auto&& DisplayDevice, auto&& SourceFrame) {
		for (auto FloatingPointToUInt8 = [](auto x) { return std::clamp(static_cast<int>(255 * x + 0.5), 0, 255); }; auto y : Range{ DisplayDevice.Height })
			for (auto x : Range{ DisplayDevice.Width })
				if (SourceFrame.PlaneCount == 3)
					DisplayDevice[y][x] = RGBA{ FloatingPointToUInt8(SourceFrame[0][y][x]), FloatingPointToUInt8(SourceFrame[1][y][x]), FloatingPointToUInt8(SourceFrame[2][y][x]) };
				else
					DisplayDevice[y][x] = RGBA{ FloatingPointToUInt8(SourceFrame[0][y][x]), FloatingPointToUInt8(SourceFrame[0][y][x]), FloatingPointToUInt8(SourceFrame[0][y][x]) };
	}
}

namespace Filter {
	auto operator*(auto&& Kernel, auto&& SourceFrame) requires requires { { Kernel(SourceFrame[0].View(0, 0)) }->std::convertible_to<std::decay_t<decltype(SourceFrame[0][0][0])>>; } {
		auto ProcessedFrame = Frame{ SourceFrame[0].Height, SourceFrame[0].Width, SourceFrame.PlaneCount };
		for (auto c : Range{ SourceFrame.PlaneCount })
			for (auto y : Range{ SourceFrame[c].Height })
				for (auto x : Range{ SourceFrame[c].Width })
					ProcessedFrame[c][y][x] = Kernel(SourceFrame[c].View(y, x));
		return ProcessedFrame.Finalize();
	}
	auto Grayscale(auto&& SourceFrame) {
		auto ProcessedFrame = Frame{ SourceFrame[0].Height, SourceFrame[0].Width, 1 };
		for (auto y : Range{ SourceFrame[0].Height })
			for (auto x : Range{ SourceFrame[0].Width })
				ProcessedFrame[0][y][x] = 0.299 * SourceFrame[0][y][x] + 0.587 * SourceFrame[1][y][x] + 0.114 * SourceFrame[2][y][x];
		return ProcessedFrame.Finalize();
	}
	auto Sobel(auto&& SourceFrame, Real auto Sensitivity) {
		auto HorizontalKernel = [](auto WeightOfTheLeftNeighbor, auto WeightOfTheCenter, auto WeightOfTheRightNeighbor) {
			return [=](auto&& Center) {
				return WeightOfTheLeftNeighbor * Center[0][-1] + WeightOfTheCenter * Center[0][0] + WeightOfTheRightNeighbor * Center[0][1];
			};
		};
		auto VerticalKernel = [](auto WeightOfTheTopNeighbor, auto WeightOfTheCenter, auto WeightOfTheBottomNeighbor) {
			return [=](auto&& Center) {
				return WeightOfTheTopNeighbor * Center[-1][0] + WeightOfTheCenter * Center[0][0] + WeightOfTheBottomNeighbor * Center[1][0];
			};
		};
		auto Luminance = Grayscale(SourceFrame);
		auto ProcessedFrame = Frame{ SourceFrame[0].Height, SourceFrame[0].Width, 1 };
		for (auto [Gx, Gy] = std::tuple{ VerticalKernel(1, 2, 1) * (HorizontalKernel(-1, 0, 1) * Luminance), VerticalKernel(1, 0, -1) * (HorizontalKernel(1, 2, 1) * Luminance) }; auto y : Range{ SourceFrame[0].Height })
			for (auto x : Range{ SourceFrame[0].Width })
				ProcessedFrame[0][y][x] = Sensitivity * std::sqrt(Gx[0][y][x] * Gx[0][y][x] + Gy[0][y][x] * Gy[0][y][x]);
		return ProcessedFrame.Finalize();
	}
	auto GaussKernel(std::integral auto Radius) {
		auto FetchKernel = [=, Storage = std::vector<double>{}]() mutable {
			Storage.resize((2 * Radius + 1)* (2 * Radius + 1));
			return ImplementationDetail::CanvasProxy<double>{ 2 * Radius + 1, 2 * Radius + 1, 2 * Radius + 1, Storage.data() + (2 * Radius + 1) * Radius + Radius };
		};
		auto NormalizationFactor = 0.;
		for (auto [Kernel, σ] = std::tuple{ FetchKernel(), Radius / 3. }; auto y : Range{ -Radius, Radius + 1 })
			for (auto x : Range{ -Radius, Radius + 1 }) {
				Kernel[y][x] = std::exp(-(x * x + y * y) / (2 * σ * σ)) / (2 * std::numbers::pi * σ * σ);
				NormalizationFactor += Kernel[y][x];
			}
		return [=, FetchKernel = std::move(FetchKernel)](auto&& Center) mutable {
			auto WeightedSum = 0.;
			for (auto Kernel = FetchKernel(); auto y : Range{ -Radius, Radius + 1 })
				for (auto x : Range{ -Radius, Radius + 1 })
					WeightedSum += Kernel[y][x] * Center[y][x];
			return WeightedSum / NormalizationFactor;
		};
	}
	auto Transpose(auto&& SourceFrame) {
		auto ProcessedFrame = Frame{ SourceFrame[0].Width, SourceFrame[0].Height, SourceFrame.PlaneCount };
		for (auto c : Range{ SourceFrame.PlaneCount })
			for (auto y : Range{ SourceFrame[c].Height })
				for (auto x : Range{ SourceFrame[c].Width })
					ProcessedFrame[c][x][y] = SourceFrame[c][y][x];
		return ProcessedFrame.Finalize();
	}
	auto HorizontalScale(auto&& SourceFrame, Real auto Scale) {
		auto ProcessedFrame = Frame{ SourceFrame[0].Height, static_cast<std::size_t>(Scale * SourceFrame[0].Width + 0.5), SourceFrame.PlaneCount };
		auto Radius = Scale > 1 ? 1. : 1. / Scale;
		auto TriangleKernel = [&](auto Displacement) { return std::abs(Displacement) > Radius ? 0. : (1 - std::abs(Displacement) / Radius) / Radius; };
		auto BackMap = [&](auto x) {
			auto CenterPosition = x / Scale + (1 - Scale) / (2. * Scale);
			auto [LeftBound, RightBound] = std::tuple{ std::ceil(CenterPosition - Radius), std::floor(CenterPosition + Radius) };
			return std::tuple{ CenterPosition, static_cast<std::ptrdiff_t>(LeftBound), static_cast<std::ptrdiff_t>(RightBound) + 1 };
		};
		for (auto c : Range{ ProcessedFrame.PlaneCount })
			for (auto y : Range{ ProcessedFrame[c].Height })
				for (auto x : Range{ ProcessedFrame[c].Width }) {
					auto [WeightedSum, NormalizationFactor] = std::tuple{ 0., 0. };
					for (auto [CenterPosition, LeftBound, RightBound] = BackMap(x); auto p : Range{ LeftBound, RightBound }) {
						WeightedSum += TriangleKernel(p - CenterPosition) * SourceFrame[c][y][p];
						NormalizationFactor += TriangleKernel(p - CenterPosition);
					}
					ProcessedFrame[c][y][x] = WeightedSum / NormalizationFactor;
				}
		return ProcessedFrame.Finalize();
	}
	constexpr auto MedianKernel = [](auto&& Center) {
		auto Samples = std::array{
			Center[-1][-1], Center[-1][0], Center[-1][1],
			Center[0][-1], Center[0][0], Center[0][1],
			Center[1][-1], Center[1][0], Center[1][1]
		};
		std::nth_element(Samples.begin(), Samples.begin() + 4, Samples.end());
		return Samples[4];
	};
}