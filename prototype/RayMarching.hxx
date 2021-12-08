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
			for (auto& [NearestDistance, _] = NearestObjectRecord; auto & x : ObjectRecords)
				if (auto& [DistanceFunction, __, ___] = x; DistanceFunction(Position) < NearestDistance)
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
	auto FarthestMarchingDistance = 50.;
	auto DistanceThresholdForIntersection = 1e-3;

	auto Intersect(auto&& DistanceField, auto&& EyePoint, auto&& RayDirection) {
		using ObjectRecordPointerType = decltype([&] {
			auto [_, PointerToObjectRecord] = DistanceField(EyePoint + 0.f * RayDirection);
			return PointerToObjectRecord;
		}());
		for (auto TraveledDistance = 1e-3; auto _ : Range{ MaximumMarchingSteps }) {
			auto [NearestDistance, PointerToObjectRecord] = DistanceField(EyePoint + static_cast<float>(TraveledDistance) * RayDirection);
			TraveledDistance += NearestDistance;
			if (NearestDistance < DistanceThresholdForIntersection)
				return std::tuple{ TraveledDistance, PointerToObjectRecord };
			if (TraveledDistance > FarthestMarchingDistance)
				return std::tuple{ NoIntersection, static_cast<ObjectRecordPointerType>(nullptr) };
		}
		return std::tuple{ NoIntersection, static_cast<ObjectRecordPointerType>(nullptr) };
	}
	auto March(auto&& EyePoint, auto&& RayDirection, auto&& DistanceField) {
		if (auto [TraveledDistance, PointerToObjectRecord] = Intersect(DistanceField, EyePoint, RayDirection); TraveledDistance != NoIntersection) {
			auto& [DistanceFunction, ObjectMaterial, IlluminationModel] = *PointerToObjectRecord;
			auto SurfacePosition = EyePoint + static_cast<float>(TraveledDistance) * RayDirection;
			return IlluminationModel(SurfacePosition, DistanceField::𝛁(DistanceFunction, SurfacePosition), EyePoint, ObjectMaterial);
		}
		return glm::vec4{ 0, 0, 0, 0 };
	}
}

namespace Illuminations {
	auto ConfigureIlluminationModel(auto& Lights, auto Ka, auto Kd, auto Ks) {
		return [=, &Lights](auto&& SurfacePosition, auto&& SurfaceNormal, auto&& EyePoint, auto&& ObjectMaterial) {
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
				AccumulatedIntensity += Diffuse(LightDirection, SurfaceNormal, LightColor, Kd * ObjectMaterial.cDiffuse);
				AccumulatedIntensity += Specular(LightDirection, SurfaceNormal, glm::normalize(EyePoint - SurfacePosition), LightColor, Ks * ObjectMaterial.cSpecular, ObjectMaterial.shininess);
			}
			return AccumulatedIntensity;
		};
	}
}