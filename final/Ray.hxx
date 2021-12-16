#pragma once
#include "Infrastructure.hxx"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"
#include <QImage>
#include <QString>

namespace ViewPlane {
	auto ConfigureProjectorFromScreenSpaceIntoWorldSpace(auto&& Camera, auto Height, auto Width) {
		auto FocalLength = Camera.focalLength;
		auto V = 2 * Camera.focalLength * std::tan(glm::radians(Camera.heightAngle) / 2);
		auto U = V * Camera.aspectRatio;
		auto TransformationIntoWorldSpace = [&] {
			auto w = glm::normalize(-glm::vec3{ Camera.look });
			auto v = glm::normalize(glm::vec3{ Camera.up } - glm::dot(glm::vec3{ Camera.up }, w) * w);
			auto u = glm::cross(v, w);
			return glm::translate(glm::vec3{ Camera.pos }) * glm::mat4{
				u.x, u.y, u.z, 0.f,
				v.x, v.y, v.z, 0.f,
				w.x, w.y, w.z, 0.f,
				0.f, 0.f, 0.f, 1.f
			};
		}();
		return [=](auto y, auto x) {
			auto NormalizedX = (x + 0.5) / Width - 0.5;
			auto NormalizedY = 0.5 - (y + 0.5) / Height;
			return TransformationIntoWorldSpace * glm::vec4{ U * NormalizedX, V * NormalizedY, -FocalLength, 1. };
		};
	}
}

namespace Ray {
	auto NoIntersection = std::numeric_limits<double>::lowest();
	auto SelfIntersectionDisplacement = 1e-2f;
	auto DefaultRecursiveTracingDepth = 4;
	auto RecursiveTracingDepth = DefaultRecursiveTracingDepth;

	auto Reflect(auto&& IncomingDirection, auto&& SurfaceNormal) {
		return glm::normalize(IncomingDirection + 2 * glm::dot(SurfaceNormal, -IncomingDirection) * SurfaceNormal);
	}
	auto Refract(auto&& IncomingDirection, auto&& SurfaceNormal, auto η) {
		auto cosθ1 = glm::dot(-SurfaceNormal, IncomingDirection);
		if (auto Discriminant = 1 - η * η * (1 - cosθ1 * cosθ1); Discriminant < 0)
			return std::tuple{ true, glm::vec4{} };
		else
			return std::tuple{ false, glm::normalize(η * IncomingDirection + (η * cosθ1 - std::sqrt(Discriminant)) * SurfaceNormal) };
	}
	auto Intersect(auto&& ImplicitFunction, auto&& ObjectTransformation, auto&& EyePoint, auto&& RayDirection) {
		auto [ObjectSpaceEyePoint, ObjectSpaceRayDirection] = [&] {
			auto InverseTransformation = glm::inverse(ObjectTransformation);
			return std::array{ InverseTransformation * EyePoint, InverseTransformation * RayDirection };
		}();
		if (auto [t, v, u, SurfaceNormal] = ImplicitFunction(ObjectSpaceEyePoint, ObjectSpaceRayDirection); t >= 0)
			return std::tuple{ t, v, u, glm::vec4{ glm::normalize(glm::inverse(glm::transpose(glm::mat3{ ObjectTransformation })) * SurfaceNormal), 0 } };
		else
			return std::tuple{ NoIntersection, 0., 0., glm::vec4{} };
	}
	auto DetectOcclusion(auto&& EyePoint, auto&& RayDirection, auto&& ObjectRecords) {
		for (auto&& [ImplicitFunction, ObjectTransformation, _, __] : ObjectRecords)
			if (auto&& [t, ___, ____, _____] = Intersect(ImplicitFunction, ObjectTransformation, EyePoint + SelfIntersectionDisplacement * RayDirection, RayDirection); t != NoIntersection)
				return true;
		return false;
	}
	auto Trace(auto&& EyePoint, auto&& RayDirection, auto ReflectionIntensity, auto&& ObjectRecords, auto RecursionDepth)->glm::vec4 {
		auto CurrentIntersectionRecord = std::tuple{ NoIntersection, glm::vec4{}, glm::vec4{}, glm::vec4{}, glm::vec4{ 0, 0, 0, 0 } };
		auto& [CurrentIntersectionDistance, CurrentIntersectionPosition, CurrentSurfaceNormal, CurrentReflectionCoefficients, CurrentTracedColor] = CurrentIntersectionRecord;
		for (auto&& [ImplicitFunction, ObjectTransformation, ObjectMaterial, IlluminationModel] : ObjectRecords)
			if (auto [t, v, u, SurfaceNormal] = Intersect(ImplicitFunction, ObjectTransformation, EyePoint, RayDirection); t != NoIntersection && (CurrentIntersectionDistance == NoIntersection || t < CurrentIntersectionDistance)) {
				auto NewIntersectionPosition = EyePoint + static_cast<float>(t) * RayDirection;
				CurrentIntersectionRecord = std::tuple{ t, NewIntersectionPosition, SurfaceNormal, ObjectMaterial.cReflective, IlluminationModel(NewIntersectionPosition, SurfaceNormal, EyePoint, v, u, ObjectMaterial) };
			}
		if (RecursionDepth < RecursiveTracingDepth && CurrentIntersectionDistance != NoIntersection) {
			auto ReflectedRayDirection = Reflect(RayDirection, CurrentSurfaceNormal);
			auto ReflectedLightColor = Trace(CurrentIntersectionPosition + SelfIntersectionDisplacement * ReflectedRayDirection, ReflectedRayDirection, ReflectionIntensity, ObjectRecords, RecursionDepth + 1);
			CurrentTracedColor += ReflectionIntensity * glm::vec4{ ReflectedLightColor.x * CurrentReflectionCoefficients.x, ReflectedLightColor.y * CurrentReflectionCoefficients.y, ReflectedLightColor.z * CurrentReflectionCoefficients.z, ReflectedLightColor.w * CurrentReflectionCoefficients.w };
		}
		return CurrentTracedColor;
	}
}

namespace Texture {
	struct Image {
		field(Data, std::vector<glm::vec4>{});
		field(Height, 0);
		field(Width, 0);
		field(Unusable, true);

	public:
		Image() = default;
		Image(auto&& FilePath) requires requires { QString{ FilePath }; } {
			if (auto ReferenceImage = QImage{ QString{ FilePath } }; ReferenceImage.isNull() == false) {
				this->Unusable = false;
				this->Height = ReferenceImage.height();
				this->Width = ReferenceImage.width();
				this->Data.resize(Height * Width);
				for (auto y : Range{ this->Height })
					for (auto x : Range{ this->Width }) {
						auto PixelData = ReferenceImage.pixel(x, y);
						Data[y * Width + x] = glm::vec4{ qRed(PixelData) / 255., qGreen(PixelData) / 255., qBlue(PixelData) / 255., qAlpha(PixelData) / 255. };
					}
			}
		}

	public:
		auto operator[](auto y) const {
			return Data.data() + y * Width;
		}
	};

	auto Map(auto&& ReferenceImage, auto VerticalTiling, auto HorizontalTiling, auto v, auto u) {
		auto MappedY = static_cast<int>(v * VerticalTiling * ReferenceImage.Height + 0.5) % ReferenceImage.Height;
		auto MappedX = static_cast<int>(u * HorizontalTiling * ReferenceImage.Width + 0.5) % ReferenceImage.Width;
		return ReferenceImage[MappedY][MappedX];
	}
}

namespace Illuminations {
	auto Diffuse(auto&& LightDirection, auto&& SurfaceNormal, auto&& LightColor, auto&& DiffuseCoefficients) {
		auto OverallIntensity = std::max(glm::dot(SurfaceNormal, -LightDirection), 0.f);
		return OverallIntensity * glm::vec4{ LightColor.x * DiffuseCoefficients.x, LightColor.y * DiffuseCoefficients.y, LightColor.z * DiffuseCoefficients.z, LightColor.w * DiffuseCoefficients.w };
	}
	auto Specular(auto&& LightDirection, auto&& SurfaceNormal, auto&& EyeDirection, auto&& LightColor, auto&& SpecularCoefficients, auto SpecularExponent) {
		auto OverallIntensity = std::pow(std::max(glm::dot(Ray::Reflect(LightDirection, SurfaceNormal), EyeDirection), 0.f), SpecularExponent);
		return OverallIntensity * glm::vec4{ LightColor.x * SpecularCoefficients.x, LightColor.y * SpecularCoefficients.y, LightColor.z * SpecularCoefficients.z, LightColor.w * SpecularCoefficients.w };
	}
	auto ConfigureIlluminationModel(auto& Lights, auto Ka, auto Kd, auto Ks, auto&& TextureImage, auto& ObjectRecords, auto EnablePointLight, auto EnableDirectionalLight, auto EnableShadows, auto EnableTextureMapping) {
		return [&, Ka, Kd, Ks, TextureImage = Forward(TextureImage), EnablePointLight, EnableDirectionalLight, EnableShadows, EnableTextureMapping](auto&& SurfacePosition, auto&& SurfaceNormal, auto&& EyePoint, auto v, auto u, auto&& ObjectMaterial) {
			auto AccumulatedIntensity = Ka * ObjectMaterial.cAmbient;
			for (auto&& Light : Lights) {
				if (Light.type == LightType::LIGHT_POINT && EnablePointLight == false)
					continue;
				if (Light.type == LightType::LIGHT_DIRECTIONAL && EnableDirectionalLight == false)
					continue;
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
				auto DiffuseCoefficients = [&] {
					if (TextureImage.Unusable || EnableTextureMapping == false)
						return Kd * ObjectMaterial.cDiffuse;
					else
						return ObjectMaterial.blend * Texture::Map(TextureImage, ObjectMaterial.textureMap.repeatV, ObjectMaterial.textureMap.repeatU, v, u) + (1 - ObjectMaterial.blend) * Kd * ObjectMaterial.cDiffuse;
				}();
				if (EnableShadows == false || Ray::DetectOcclusion(SurfacePosition, -LightDirection, ObjectRecords) == false) {
					AccumulatedIntensity += Diffuse(LightDirection, SurfaceNormal, LightColor, DiffuseCoefficients);
					AccumulatedIntensity += Specular(LightDirection, SurfaceNormal, glm::normalize(EyePoint - SurfacePosition), LightColor, Ks * ObjectMaterial.cSpecular, ObjectMaterial.shininess);
				}
			}
			return AccumulatedIntensity;
		};
	}
}

namespace Texture::SurfaceParameterizers {
	auto Circle = [](auto&& Coordinates) {
		if (auto θ = std::atan2(Coordinates.z, Coordinates.x); θ < 0)
			return -θ / (2 * std::numbers::pi);
		else
			return 1 - θ / (2 * std::numbers::pi);
	};
	auto Sphere = [](auto&& Coordinates) {
		return std::tuple{ 0.5 + std::asin(-Coordinates.y / 0.5) / std::numbers::pi, Circle(Coordinates) };
	};
	auto ConicalFrustum = [](auto&& Coordinates) {
		return std::tuple{ -Coordinates.y + 0.5, Circle(Coordinates) };
	};
	auto XYPlane = [](auto&& Coordinates) {
		return std::tuple{ -Coordinates.y + 0.5, Coordinates.z >= 0 ? Coordinates.x + 0.5 : -Coordinates.x + 0.5 };
	};
	auto YZPlane = [](auto&& Coordinates) {
		return std::tuple{ -Coordinates.y + 0.5, Coordinates.x >= 0 ? -Coordinates.z + 0.5 : Coordinates.z + 0.5 };
	};
	auto XZPlane = [](auto&& Coordinates) {
		return std::tuple{ Coordinates.y >= 0 ? Coordinates.z + 0.5 : -Coordinates.z + 0.5, Coordinates.x + 0.5 };
	};
}

namespace ImplicitFunctions {
    auto ε = std::numeric_limits<double>::min();
}

namespace ImplicitFunctions::Solvers {
	auto Quadratic(auto&& EyePoint, auto&& RayDirection, auto a, auto b, auto c, auto&& Constraint, auto&& Parameterizer) {
		if (auto Discriminant = b * b - 4 * a * c; std::abs(a) <= ε || Discriminant < 0)
			return std::tuple{ Ray::NoIntersection, glm::vec3{}, 0., 0. };
		else if (auto SmallerRoot = (-b - std::sqrt(Discriminant)) / (2. * a); SmallerRoot >= 0) {
			auto IntersectionPosition = EyePoint + static_cast<float>(SmallerRoot) * RayDirection;
			return Constraint(IntersectionPosition.x, IntersectionPosition.y, IntersectionPosition.z) ? std::tuple{ SmallerRoot, glm::vec3{ IntersectionPosition } } + Parameterizer(IntersectionPosition) : std::tuple{ Ray::NoIntersection, glm::vec3{}, 0., 0. };
		}
		else if (auto LargerRoot = (-b + std::sqrt(Discriminant)) / (2. * a); LargerRoot >= 0) {
			auto IntersectionPosition = EyePoint + static_cast<float>(LargerRoot) * RayDirection;
			return Constraint(IntersectionPosition.x, IntersectionPosition.y, IntersectionPosition.z) ? std::tuple{ LargerRoot, glm::vec3{ IntersectionPosition } } + Parameterizer(IntersectionPosition) : std::tuple{ Ray::NoIntersection, glm::vec3{}, 0., 0. };
		}
		else
			return std::tuple{ Ray::NoIntersection, glm::vec3{}, 0., 0. };
	}
	auto XZPlane(auto&& EyePoint, auto&& RayDirection, auto y, auto&& Constraint) {
		if (auto t = (y - EyePoint.y) / RayDirection.y; std::abs(RayDirection.y) > ε && Constraint(EyePoint.x + t * RayDirection.x, EyePoint.z + t * RayDirection.z))
			return std::tuple{ t } + Texture::SurfaceParameterizers::XZPlane(EyePoint + static_cast<float>(t) * RayDirection);
		else
			return std::tuple{ Ray::NoIntersection, 0., 0. };
	}
	auto XYPlane(auto&& EyePoint, auto&& RayDirection, auto z, auto&& Constraint) {
		if (auto t = (z - EyePoint.z) / RayDirection.z; std::abs(RayDirection.z) > ε && Constraint(EyePoint.x + t * RayDirection.x, EyePoint.y + t * RayDirection.y))
			return std::tuple{ t } + Texture::SurfaceParameterizers::XYPlane(EyePoint + static_cast<float>(t) * RayDirection);
		else
			return std::tuple{ Ray::NoIntersection, 0., 0. };
	}
	auto YZPlane(auto&& EyePoint, auto&& RayDirection, auto x, auto&& Constraint) {
		if (auto t = (x - EyePoint.x) / RayDirection.x; std::abs(RayDirection.x) > ε && Constraint(EyePoint.y + t * RayDirection.y, EyePoint.z + t * RayDirection.z))
			return std::tuple{ t } + Texture::SurfaceParameterizers::YZPlane(EyePoint + static_cast<float>(t) * RayDirection);
		else
			return std::tuple{ Ray::NoIntersection, 0., 0. };
	}
}

namespace ImplicitFunctions::StandardConstraints {
	auto BoundedPlane = [](auto x, auto y) {
		return -0.5 <= x && x <= 0.5 && -0.5 <= y && y <= 0.5;
	};
	auto CircularPlane = [](auto x, auto y) {
		return x * x + y * y <= 0.5 * 0.5;
	};
	auto BoundedHeight = [](auto, auto y, auto) {
		return -0.5 <= y && y <= 0.5;
	};
}

namespace ImplicitFunctions::ImplementationDetail {
	auto DetermineNearestIntersection(auto&& CurrentIntersectionRecord, auto&& CandidateIntersectionRecord, auto&& ...Arguments) {
		auto& [CurrentIntersectionDistance, _, __, ___] = CurrentIntersectionRecord;
		if (auto& [CandidateIntersectionDistance, ____, _____, ______] = CandidateIntersectionRecord; CandidateIntersectionDistance >= 0 && (CurrentIntersectionDistance < 0 || CandidateIntersectionDistance < CurrentIntersectionDistance))
			CurrentIntersectionRecord = CandidateIntersectionRecord;
		if constexpr (sizeof...(Arguments) != 0)
			return DetermineNearestIntersection(CurrentIntersectionRecord, Arguments...);
		else
			return CurrentIntersectionRecord;
	}
}

namespace ImplicitFunctions {
	auto Cube = [](auto&& EyePoint, auto&& RayDirection) {
		return ImplementationDetail::DetermineNearestIntersection(
			Solvers::YZPlane(EyePoint, RayDirection, -0.5, StandardConstraints::BoundedPlane) + std::tuple{ glm::vec3{ -1, 0, 0 } },
			Solvers::YZPlane(EyePoint, RayDirection, 0.5, StandardConstraints::BoundedPlane) + std::tuple{ glm::vec3{ 1, 0, 0 } },
			Solvers::XZPlane(EyePoint, RayDirection, 0.5, StandardConstraints::BoundedPlane) + std::tuple{ glm::vec3{ 0, 1, 0 } },
			Solvers::XZPlane(EyePoint, RayDirection, -0.5, StandardConstraints::BoundedPlane) + std::tuple{ glm::vec3{ 0, -1, 0 } },
			Solvers::XYPlane(EyePoint, RayDirection, 0.5, StandardConstraints::BoundedPlane) + std::tuple{ glm::vec3{ 0, 0, 1 } },
			Solvers::XYPlane(EyePoint, RayDirection, -0.5, StandardConstraints::BoundedPlane) + std::tuple{ glm::vec3{ 0, 0, -1 } }
		);
	};
	auto Sphere = [](auto&& EyePoint, auto&& RayDirection) {
		auto a = RayDirection.x * RayDirection.x + RayDirection.y * RayDirection.y + RayDirection.z * RayDirection.z;
		auto b = 2 * (RayDirection.x * EyePoint.x + RayDirection.y * EyePoint.y + RayDirection.z * EyePoint.z);
		auto c = EyePoint.x * EyePoint.x + EyePoint.y * EyePoint.y + EyePoint.z * EyePoint.z - 0.25;
		if (auto [t, IntersectionPosition, v, u] = Solvers::Quadratic(EyePoint, RayDirection, a, b, c, [](auto...) { return true; }, Texture::SurfaceParameterizers::Sphere); t >= 0)
			return std::tuple{ t, v, u, glm::normalize(IntersectionPosition) };
		else
			return std::tuple{ Ray::NoIntersection, 0., 0., glm::vec3{} };
	};
	auto Cylinder = [](auto&& EyePoint, auto&& RayDirection) {
		auto a = RayDirection.x * RayDirection.x + RayDirection.z * RayDirection.z;
		auto b = 2 * (RayDirection.x * EyePoint.x + RayDirection.z * EyePoint.z);
		auto c = EyePoint.x * EyePoint.x + EyePoint.z * EyePoint.z - 0.25;
		auto [t, SideIntersection, v, u] = Solvers::Quadratic(EyePoint, RayDirection, a, b, c, StandardConstraints::BoundedHeight, Texture::SurfaceParameterizers::ConicalFrustum);
		return ImplementationDetail::DetermineNearestIntersection(
			Solvers::XZPlane(EyePoint, RayDirection, 0.5, StandardConstraints::CircularPlane) + std::tuple{ glm::vec3{ 0, 1, 0 } },
			Solvers::XZPlane(EyePoint, RayDirection, -0.5, StandardConstraints::CircularPlane) + std::tuple{ glm::vec3{ 0, -1, 0 } },
			std::tuple{ t, v, u, glm::normalize(glm::vec3{ SideIntersection.x, 0, SideIntersection.z }) }
		);
	};
	auto Cone = [](auto&& EyePoint, auto&& RayDirection) {
		auto a = RayDirection.x * RayDirection.x + RayDirection.z * RayDirection.z - 0.25 * RayDirection.y * RayDirection.y;
		auto b = 2 * (RayDirection.x * EyePoint.x + RayDirection.z * EyePoint.z) - 0.5 * RayDirection.y * EyePoint.y + 0.25 * RayDirection.y;
		auto c = EyePoint.x * EyePoint.x + EyePoint.z * EyePoint.z - 0.25 * EyePoint.y * EyePoint.y + 0.25 * EyePoint.y - 0.0625;
		auto [t, SideIntersection, v, u] = Solvers::Quadratic(EyePoint, RayDirection, a, b, c, StandardConstraints::BoundedHeight, Texture::SurfaceParameterizers::ConicalFrustum);
		return ImplementationDetail::DetermineNearestIntersection(
			Solvers::XZPlane(EyePoint, RayDirection, -0.5, StandardConstraints::CircularPlane) + std::tuple{ glm::vec3{ 0, -1, 0 } },
			std::tuple{ t, v, u, glm::normalize(glm::vec3{ 2 * SideIntersection.x, 0.25 - 0.5 * SideIntersection.y, 2 * SideIntersection.z }) }
		);
	};
}
