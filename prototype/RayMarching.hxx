#pragma once
#include "Ray.hxx"

namespace ViewPlane {
	auto ConfigureRayCaster(auto&& LookVector, auto&& UpVector, auto FocalLength, auto Height, auto Width) {
		auto zAxis = -glm::vec3{ LookVector };
		auto xAxis = glm::normalize(glm::cross(zAxis, glm::vec3{ UpVector }));
		auto yAxis = glm::normalize(glm::cross(xAxis, zAxis));
		auto AspectRatio = static_cast<float>(Width) / Height;
		return [=, FocalLength = static_cast<float>(FocalLength)](auto y, auto x) {
			auto NormalizedX = static_cast<float>(2. * ((x + 0.5) / Width - 0.5));
			auto NormalizedY = static_cast<float>(2. * (0.5 - (y + 0.5) / Height));
			return glm::vec4{ glm::normalize(NormalizedX * AspectRatio * xAxis + NormalizedY * yAxis + FocalLength * zAxis), 0 };
		};
	}
}

namespace DistanceField {
	auto Synthesize(auto& ObjectRecords) {
		return [&](auto&& Position) {
			using ObjectRecordType = std::decay_t<decltype(*std::begin(ObjectRecords))>;
			auto NearestObjectRecord = std::tuple{ std::numeric_limits<double>::infinity(), static_cast<ObjectRecordType*>(nullptr) };
			for (auto& [NearestDistance, _] = NearestObjectRecord; auto& x : ObjectRecords)
				if (auto& [DistanceFunction, __, ___] = x; std::abs(DistanceFunction(Position)) < std::abs(NearestDistance))
					NearestObjectRecord = std::tuple{ DistanceFunction(Position), const_cast<ObjectRecordType*>(&x) };
			return NearestObjectRecord;
		};
	}
	auto 𝛁(auto&& DistanceFunction, auto&& Position) {
		constexpr auto ε = 1e-4;
		auto [dx, dy, dz] = std::tuple{ glm::vec4{ ε, 0, 0, 0 }, glm::vec4{ 0, ε, 0, 0 }, glm::vec4{ 0, 0, ε, 0 } };
		return glm::vec4{ glm::normalize(glm::vec3{ DistanceFunction(Position + dx) - DistanceFunction(Position - dx), DistanceFunction(Position + dy) - DistanceFunction(Position - dy), DistanceFunction(Position + dz) - DistanceFunction(Position - dz) }), 0 };
	}
}

namespace Ray {
	auto MaximumMarchingSteps = 1000;
	auto FarthestMarchingDistance = 100.;
	auto IntersectionThreshold = 1e-3;
	auto RecursiveMarchingDepth = 4;

	auto Intersect(auto&& DistanceField, auto&& EyePoint, auto&& RayDirection) {
		using ObjectRecordPointerType = decltype([&] {
			auto [_, PointerToObjectRecord] = DistanceField(EyePoint + 0.f * RayDirection);
			return PointerToObjectRecord;
		}());
		for (auto TraveledDistance = 1e-3; auto _ : Range{ MaximumMarchingSteps }) {
			auto [UnboundingRadius, PointerToObjectRecord] = DistanceField(EyePoint + static_cast<float>(TraveledDistance) * RayDirection);
			TraveledDistance += std::abs(UnboundingRadius);
			if (0 <= UnboundingRadius && UnboundingRadius < IntersectionThreshold)
				return std::tuple{ TraveledDistance, PointerToObjectRecord };
			if (TraveledDistance > FarthestMarchingDistance)
				return std::tuple{ NoIntersection, static_cast<ObjectRecordPointerType>(nullptr) };
		}
		return std::tuple{ NoIntersection, static_cast<ObjectRecordPointerType>(nullptr) };
	}
	auto EstimateOccludedIntensity(auto&& EyePoint, auto&& RayDirection, auto&& DistanceField, auto Hardness) {
		auto OccludedIntensity = 1.;
		for (auto [TraveledDistance, PreviousUnboundingRadius] = std::array{ 1e-3, 1e20 }; auto _ : Range{ MaximumMarchingSteps }) {
			auto [UnboundingRadius, __] = DistanceField(EyePoint + static_cast<float>(SelfIntersectionDisplacement + TraveledDistance) * RayDirection);
			auto δ = UnboundingRadius * UnboundingRadius / (2.0 * PreviousUnboundingRadius);
			OccludedIntensity = std::min(OccludedIntensity, Hardness * std::sqrt(UnboundingRadius * UnboundingRadius - δ * δ) / std::max(0., TraveledDistance - δ));
			PreviousUnboundingRadius = UnboundingRadius;
			TraveledDistance += UnboundingRadius;
			if (UnboundingRadius < IntersectionThreshold)
				return 0.;
			if (TraveledDistance > FarthestMarchingDistance)
				return OccludedIntensity;
		}
		return OccludedIntensity;
	}
	auto March(auto&& EyePoint, auto&& RayDirection, auto ReflectionIntensity, auto RefractionIntensity, auto&& DistanceField, auto RecursionDepth)->glm::vec4 {
		if (auto [TraveledDistance, PointerToObjectRecord] = Intersect(DistanceField, EyePoint, RayDirection); TraveledDistance != NoIntersection) {
			auto& [DistanceFunction, ObjectMaterial, IlluminationModel] = *PointerToObjectRecord;
			auto SurfacePosition = EyePoint + static_cast<float>(TraveledDistance) * RayDirection;
			auto SurfaceNormal = DistanceField::𝛁(DistanceFunction, SurfacePosition);
			auto AccumulatedIntensity = IlluminationModel(SurfacePosition, SurfaceNormal, EyePoint, ObjectMaterial);
			if (RecursionDepth < RecursiveMarchingDepth) {
				auto [RefractionNormal, η] = [&] {
					if (glm::dot(RayDirection, SurfaceNormal) > 0)
						return std::tuple{ -SurfaceNormal, ObjectMaterial.ior };
					else
						return std::tuple{ SurfaceNormal, 1 / ObjectMaterial.ior };
				}();
				auto ReflectedRayDirection = Reflect(RayDirection, SurfaceNormal);
				auto ReflectedLightColor = March(SurfacePosition + SelfIntersectionDisplacement * ReflectedRayDirection, ReflectedRayDirection, ReflectionIntensity, RefractionIntensity, DistanceField, RecursionDepth + 1);
				AccumulatedIntensity += ReflectionIntensity * glm::vec4{ ReflectedLightColor.x * ObjectMaterial.cReflective.x, ReflectedLightColor.y * ObjectMaterial.cReflective.y, ReflectedLightColor.z * ObjectMaterial.cReflective.z, ReflectedLightColor.w * ObjectMaterial.cReflective.w };
				if (auto [TotalInternalReflection, RefractedRayDirection] = Refract(RayDirection, RefractionNormal, η); TotalInternalReflection == false) {
					auto RefractedLightColor = March(SurfacePosition - SelfIntersectionDisplacement * RefractionNormal, RefractedRayDirection, ReflectionIntensity, RefractionIntensity, DistanceField, RecursionDepth + 1);
					AccumulatedIntensity += RefractionIntensity * glm::vec4{ RefractedLightColor.x * ObjectMaterial.cTransparent.x, RefractedLightColor.y * ObjectMaterial.cTransparent.y, RefractedLightColor.z * ObjectMaterial.cTransparent.z, RefractedLightColor.w * ObjectMaterial.cTransparent.w };
				}
			}
			return AccumulatedIntensity;
		}
		return glm::vec4{ 0, 0, 0, 0 };
	}
}

namespace Illuminations {
	auto ConfigureIlluminationModel(auto& Lights, auto Ka, auto Kd, auto Ks, auto& DistanceField, auto Hardness) {
		return [=, &Lights, &DistanceField](auto&& SurfacePosition, auto&& SurfaceNormal, auto&& EyePoint, auto&& ObjectMaterial) {
			auto AccumulatedIntensity = Ka * ObjectMaterial.cAmbient;
			for (auto&& Light : Lights) {
				auto LightDirection = [&] {
					if (Light.type == LightType::LIGHT_POINT)
						return glm::normalize(SurfacePosition - Light.pos);
					else if (Light.type == LightType::LIGHT_DIRECTIONAL)
						return glm::normalize(Light.dir);
					else
						throw std::runtime_error{ "Unrecognized light type detected!" };
				}();
				auto LightColor = [&] {
					if (Light.type == LightType::LIGHT_POINT) {
						auto LightDisplacement = SurfacePosition - Light.pos;
						auto SquaredLightDistance = glm::dot(LightDisplacement, LightDisplacement);
						auto LightDistance = std::sqrt(SquaredLightDistance);
						return std::min(1 / (Light.function.x + Light.function.y * LightDistance + Light.function.z * SquaredLightDistance), 1.f) * Light.color;
					}
					else
						return Light.color;
				}();
				auto OccludedIntensity = static_cast<float>(Ray::EstimateOccludedIntensity(SurfacePosition, -LightDirection, DistanceField, Hardness));
				AccumulatedIntensity += OccludedIntensity * Diffuse(LightDirection, SurfaceNormal, LightColor, Kd * ObjectMaterial.cDiffuse);
				AccumulatedIntensity += OccludedIntensity * Specular(LightDirection, SurfaceNormal, glm::normalize(EyePoint - SurfacePosition), LightColor, Ks * ObjectMaterial.cSpecular, ObjectMaterial.shininess);
			}
			return AccumulatedIntensity;
		};
	}
}