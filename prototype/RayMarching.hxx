#pragma once
#include "Infrastructure.hxx"
#include "glm/glm.hpp"

enum struct Primitives {
	None,
	Sphere,
	Plane
};


namespace TestDrive {
	auto sphere(auto&& p) {
		auto center = glm::vec4{ 0, 0.25, 0, 1 };
		auto radius = 1.5;
		return glm::length(p - center) - radius;
	}

	auto plane(auto&& p) {
		return static_cast<double>(p.y);
	}



	constexpr auto Map = [](auto&& p) {
		if (auto [d_sphere, d_plane] = std::tuple{ sphere(p), plane(p) }; d_sphere < d_plane)
			return std::tuple{ d_sphere, Primitives::Sphere };
		else
			return std::tuple{ d_plane, Primitives::Plane };
	};
}

namespace Ray {
	auto Intersect(auto&& DistanceField, auto&& EyePoint, auto&& RayDirection) {

		auto TraveledDistance = 1e-3;
		auto DistanceBound = 50.;
		auto IntersectionThreshold = 1e-3;

		for (auto _ : Range{ 1000 }) {
			auto [Distance, Primitive] = DistanceField(EyePoint + static_cast<float>(TraveledDistance) * RayDirection);
			TraveledDistance += Distance;
			if (Distance < IntersectionThreshold)
				return std::tuple{ TraveledDistance, Primitive };
			if (TraveledDistance > DistanceBound)
				return std::tuple{ NoIntersection, Primitives::None };
		}

		return std::tuple{ NoIntersection, Primitives::None };

	}


	auto March(auto&& EyePoint, auto&& RayDirection, auto&& DistanceField) {
		auto [Distance, Primitive] = Intersect(DistanceField, EyePoint, RayDirection);
		if (Distance != NoIntersection)
			if (Primitive == Primitives::Sphere)
				return glm::vec4{ 1, 0, 0, 1 };
			else if (Primitive == Primitives::Plane)
				return glm::vec4{ 0.5, 0.5, 0.5, 1 };
		return glm::vec4{ 0, 0, 0, 0 };
	}




}