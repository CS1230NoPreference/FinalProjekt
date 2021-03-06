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
	auto MaximumMarchingSteps = 10000;
	auto FarthestMarchingDistance = 1000.;
        auto IntersectionThreshold = 1e-3;
	auto RecursiveMarchingDepth = 4;
	auto RelativeStepSizeForIntersection = 1.;
	auto RelativeStepSizeForOcclusionEstimation = 0.1;

	auto Intersect(auto&& DistanceField, auto&& EyePoint, auto&& RayDirection) {
		using ObjectRecordPointerType = decltype([&] {
			auto [_, PointerToObjectRecord] = DistanceField(EyePoint + 0.f * RayDirection);
			return PointerToObjectRecord;
			}());
		for (auto TraveledDistance = 1e-3; auto _ : Range{ MaximumMarchingSteps }) {
			auto [UnboundingRadius, PointerToObjectRecord] = DistanceField(EyePoint + static_cast<float>(TraveledDistance) * RayDirection);
			TraveledDistance += RelativeStepSizeForIntersection * std::abs(UnboundingRadius);
			if (0 <= UnboundingRadius && UnboundingRadius < IntersectionThreshold)
				return std::tuple{ TraveledDistance, PointerToObjectRecord };
			if (TraveledDistance > FarthestMarchingDistance)
				return std::tuple{ NoIntersection, static_cast<ObjectRecordPointerType>(nullptr) };
		}
		return std::tuple{ NoIntersection, static_cast<ObjectRecordPointerType>(nullptr) };
	}
	auto EstimateOccludedIntensity(auto&& EyePoint, auto&& RayDirection, auto&& DistanceField, auto Hardness) {
		auto OccludedIntensity = 1.;
		for (auto TraveledDistance = 1e-3; auto _ : Range{ MaximumMarchingSteps }) {
			auto [UnboundingRadius, __] = DistanceField(EyePoint + static_cast<float>(SelfIntersectionDisplacement + TraveledDistance) * RayDirection);
			OccludedIntensity = std::min(OccludedIntensity, std::abs(Hardness * UnboundingRadius / TraveledDistance));
			TraveledDistance += RelativeStepSizeForOcclusionEstimation * UnboundingRadius;
			if (UnboundingRadius < IntersectionThreshold)
				return 0.;
			if (TraveledDistance > FarthestMarchingDistance)
				return OccludedIntensity;
		}
		return OccludedIntensity;
	}
	auto March(auto&& EyePoint, auto&& RayDirection, auto ReflectionIntensity, auto RefractionIntensity, auto&& DistanceField, auto&& InterruptHandler, auto RecursionDepth)->glm::vec4 {
		if (auto [TraveledDistance, PointerToObjectRecord] = Intersect(DistanceField, EyePoint, RayDirection); TraveledDistance != NoIntersection) {
			auto& [DistanceFunction, ObjectMaterial, IlluminationModel] = *PointerToObjectRecord;
			auto SurfacePosition = EyePoint + static_cast<float>(TraveledDistance) * RayDirection;
			auto SurfaceNormal = DistanceField::𝛁(DistanceFunction, SurfacePosition);
			auto EstimateReflectedIntensity = [&] {
				auto ReflectedRayDirection = Reflect(RayDirection, SurfaceNormal);
				auto ReflectedLightColor = March(SurfacePosition + SelfIntersectionDisplacement * ReflectedRayDirection, ReflectedRayDirection, ReflectionIntensity, RefractionIntensity, DistanceField, InterruptHandler, RecursionDepth + 1);
				return glm::vec4{ ReflectedLightColor.x * ObjectMaterial.cReflective.x, ReflectedLightColor.y * ObjectMaterial.cReflective.y, ReflectedLightColor.z * ObjectMaterial.cReflective.z, ReflectedLightColor.w * ObjectMaterial.cReflective.w };
			};
			auto EstimateRefractedIntensity = [&] {
				auto [RefractionNormal, η] = [&] {
					if (glm::dot(RayDirection, SurfaceNormal) > 0)
						return std::tuple{ -SurfaceNormal, ObjectMaterial.ior };
					else
						return std::tuple{ SurfaceNormal, 1 / ObjectMaterial.ior };
				}();
				if (auto [TotalInternalReflection, RefractedRayDirection] = Refract(RayDirection, RefractionNormal, η); TotalInternalReflection == false) {
					auto RefractedLightColor = March(SurfacePosition - SelfIntersectionDisplacement * RefractionNormal, RefractedRayDirection, ReflectionIntensity, RefractionIntensity, DistanceField, InterruptHandler, RecursionDepth + 1);
					return glm::vec4{ RefractedLightColor.x * ObjectMaterial.cTransparent.x, RefractedLightColor.y * ObjectMaterial.cTransparent.y, RefractedLightColor.z * ObjectMaterial.cTransparent.z, RefractedLightColor.w * ObjectMaterial.cTransparent.w };
				}
				else
					return glm::vec4{ 0, 0, 0, 0 };
			};
			auto EstimateReflectance = [&] {
				auto cosθi = glm::dot(RayDirection, SurfaceNormal);
				auto [η1, η2] = [&] {
					if (cosθi > 0)
						return std::tuple{ 1., static_cast<double>(ObjectMaterial.ior) };
					else
						return std::tuple{ static_cast<double>(ObjectMaterial.ior), 1. };
				}();
				if (auto sinθt = η2 / η1 * std::sqrt(std::max(0., 1. - cosθi * cosθi)); sinθt >= 1)
					return 1.;
				else {
					auto cosθt = std::sqrt(std::max(0., 1. - sinθt * sinθt));
					auto RootOfRs = (η1 * std::abs(cosθi) - η2 * cosθt) / (η1 * std::abs(cosθi) + η2 * cosθt);
					auto RootOfRp = (η2 * std::abs(cosθi) - η1 * cosθt) / (η2 * std::abs(cosθi) + η1 * cosθt);
					return (RootOfRs * RootOfRs + RootOfRp * RootOfRp) / 2;
				}
			};
			InterruptHandler(SurfacePosition, SurfaceNormal, *PointerToObjectRecord);
			auto AccumulatedIntensity = IlluminationModel(SurfacePosition, SurfaceNormal, EyePoint, ObjectMaterial);
			if (RecursionDepth < RecursiveMarchingDepth)
				if (ObjectMaterial.IsReflective && ObjectMaterial.IsTransparent) {
					auto Reflectance = static_cast<float>(EstimateReflectance());
					AccumulatedIntensity += ReflectionIntensity * Reflectance * EstimateReflectedIntensity();
					AccumulatedIntensity += RefractionIntensity * (1 - Reflectance) * EstimateRefractedIntensity();
				}
				else if (ObjectMaterial.IsReflective)
					AccumulatedIntensity += ReflectionIntensity * EstimateReflectedIntensity();
				else if (ObjectMaterial.IsTransparent)
					AccumulatedIntensity += RefractionIntensity * EstimateRefractedIntensity();
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
