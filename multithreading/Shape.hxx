#pragma once
#include "Infrastructure.hxx"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/transform.hpp"

namespace Shape {
    struct Vertex {
        field(Coordinates, std::array{ 0., 0., 0. });
        field(Normal, std::array{ 0., 0., 0. });
    };

    auto operator*(auto&& Transformation, SubtypeOf<Vertex> auto&& Object) requires requires { { Transformation * glm::vec4{} }->SubtypeOf<glm::vec4>; } {
        auto TransformedCoordinates = Transformation * glm::vec4{ Object.Coordinates[0], Object.Coordinates[1], Object.Coordinates[2], 1 };
        auto TransformedNormal = glm::normalize(glm::inverse(glm::transpose(glm::mat3{ Transformation })) * glm::vec3{ Object.Normal[0], Object.Normal[1], Object.Normal[2] });
        return Vertex{
            .Coordinates = { TransformedCoordinates.x, TransformedCoordinates.y, TransformedCoordinates.z },
            .Normal = { TransformedNormal.x, TransformedNormal.y, TransformedNormal.z }
        };
    }
    auto operator*(auto&& Transformations, Iterable auto&& Object) requires requires { { *std::begin(Object) }->SubtypeOf<Vertex>; } {
        auto Apply = [&](auto&& Transformation) {
            auto ObjectReplica = Object;
            for (auto& x : ObjectReplica)
                x = Transformation * x;
            return ObjectReplica;
        };
        if constexpr (requires { { Transformations }->Iterable; })
            return Utility::ContainerManipulators::fMap(Apply, Transformations);
        else
            return Apply(Transformations);
    }
    auto Export(Iterable auto&& Object) requires requires { { *std::begin(Object) }->SubtypeOf<Vertex>; } {
        auto LowLevelRepresentation = std::vector<float>{};
        for (auto&& [Coordinates, Normal] : Object) {
            LowLevelRepresentation += Coordinates;
            LowLevelRepresentation += Normal;
        }
        return LowLevelRepresentation;
    }

    using std::numbers::pi;
}

namespace Shape::ImplementationDetail {
    auto Tile(auto&& VertexGenerator, auto&& ...Arguments) requires (sizeof...(Arguments) == 4) {
        auto [TopLeftVertex, TopRightVertex, BottomLeftVertex, BottomRightVertex] = std::tuple{ std::apply(VertexGenerator, Forward(Arguments))... };
        return std::array{ TopLeftVertex, BottomLeftVertex, TopRightVertex, BottomLeftVertex, BottomRightVertex, TopRightVertex };
    }
    auto PolarToCartesian(Real auto Radius, Real auto θ) {
        return std::array{ Radius * std::cos(θ), Radius * std::sin(θ) };
    }
}

namespace Shape::inline Transformations {
    constexpr auto TranslateY = [](auto Displacement) {
        return glm::translate(glm::vec3{ 0., Displacement, 0. });
    };
    constexpr auto RotateX = [](auto θ) {
        return glm::rotate(static_cast<float>(θ), glm::vec3{ 1., 0., 0. });
    };
    constexpr auto RotateZ = [](auto θ) {
        return glm::rotate(static_cast<float>(θ), glm::vec3{ 0., 0., 1. });
    };
}

namespace Shape::inline Planar {
    auto DrawSquare(std::integral auto TessellationFactor) {
        auto TessellatedSquare = std::vector<Vertex>{};
        auto VertexGenerator = [&](auto x, auto z) {
            auto SamplingStep = 1. / TessellationFactor;
            return Vertex{ .Coordinates = { -0.5 + x * SamplingStep, 0., -0.5 + z * SamplingStep }, .Normal = { 0., 1., 0. } };
        };
        for (auto x : Range{ TessellationFactor })
            for (auto z : Range{ TessellationFactor })
                TessellatedSquare += ImplementationDetail::Tile(VertexGenerator, std::array{ x, z }, std::array{ x + 1, z }, std::array{ x, z + 1 }, std::array{ x + 1, z + 1 });
        return TessellatedSquare;
    }
    auto DrawCircle(std::integral auto TessellationFactor, std::integral auto QuantizationFactor) {
        auto TessellatedCircle = std::vector<Vertex>{};
        auto VertexGenerator = [&](auto r, auto θ) {
            auto [AngularSamplingStep, DisplacementSamplingStep] = std::array{ 2 * pi / QuantizationFactor, 0.5 / TessellationFactor };
            auto [x, z] = ImplementationDetail::PolarToCartesian(r * DisplacementSamplingStep, θ * AngularSamplingStep);
            return Vertex{ .Coordinates = { x, 0., z }, .Normal = { 0., 1., 0. } };
        };
        for (auto r : Range{ TessellationFactor })
            for (auto θ : Range{ QuantizationFactor })
                TessellatedCircle += ImplementationDetail::Tile(VertexGenerator, std::array{ r, θ }, std::array{ r + 1, θ }, std::array{ r, θ + 1 }, std::array{ r + 1, θ + 1 });
        return TessellatedCircle;
    }
}

namespace Shape::inline Spatial {
    auto DrawCube(std::integral auto TessellationFactor) {
        auto TopSide = TranslateY(0.5) * DrawSquare(TessellationFactor);
        auto [FrontSide, BackSide, BottomSide, LeftSide, RightSide] = std::array{ RotateX(pi / 2), RotateX(-pi / 2), RotateX(pi), RotateZ(pi / 2), RotateZ(-pi / 2) } * TopSide;
        return TopSide + BottomSide + FrontSide + BackSide + RightSide + LeftSide;
    }
    auto DrawConicalFrustum(std::integral auto TessellationFactor, std::integral auto QuantizationFactor, Real auto TopBaseRadius, Real auto BottomBaseRadius) {
        auto TessellatedFrustum = std::vector<Vertex>{};
        auto VertexGenerator = [&](auto d, auto θ) {
            auto [AngularSamplingStep, ySamplingStep] = std::array{ 2 * pi / QuantizationFactor, 1. / TessellationFactor };
            auto [TopBaseX, TopBaseZ] = ImplementationDetail::PolarToCartesian(TopBaseRadius, θ * AngularSamplingStep);
            auto [BottomBaseX, BottomBaseZ] = ImplementationDetail::PolarToCartesian(BottomBaseRadius, θ * AngularSamplingStep);
            auto [xSamplingStep, zSamplingStep] = std::array{ (BottomBaseX - TopBaseX) / TessellationFactor, (BottomBaseZ - TopBaseZ) / TessellationFactor };
            auto VertexNormal = glm::cross(glm::cross(glm::vec3{ BottomBaseX, 0., BottomBaseZ }, glm::vec3{ 0., 1., 0. }), glm::vec3{ BottomBaseX - TopBaseX, -1., BottomBaseZ - TopBaseZ });
            VertexNormal = glm::normalize(VertexNormal);
            return Vertex{ .Coordinates = { TopBaseX + d * xSamplingStep, 0.5 - d * ySamplingStep, TopBaseZ + d * zSamplingStep }, .Normal = { VertexNormal.x, VertexNormal.y, VertexNormal.z } };
        };
        for (auto d : Range{ TessellationFactor })
            for (auto θ : Range{ QuantizationFactor })
                TessellatedFrustum += ImplementationDetail::Tile(VertexGenerator, std::array{ d, θ + 1 }, std::array{ d, θ }, std::array{ d + 1, θ + 1 }, std::array{ d + 1, θ });
        return TessellatedFrustum;
    }
    auto DrawCylinder(std::integral auto TessellationFactor, std::integral auto QuantizationFactor) {
        auto [TopBase, BottomBase] = std::array{ TranslateY(0.5), TranslateY(-0.5) * RotateX(pi) } * DrawCircle(TessellationFactor, QuantizationFactor);
        return TopBase + BottomBase + DrawConicalFrustum(TessellationFactor, QuantizationFactor, 0.5, 0.5);
    }
    auto DrawCone(std::integral auto TessellationFactor, std::integral auto QuantizationFactor) {
        return TranslateY(-0.5) * RotateX(pi) * DrawCircle(TessellationFactor, QuantizationFactor) + DrawConicalFrustum(TessellationFactor, QuantizationFactor, 0, 0.5);
    }
    auto DrawSphere(std::integral auto LatitudeQuantizationFactor, std::integral auto LongitudeQuantizationFactor) {
        auto TessellatedSphere = std::vector<Vertex>{};
        auto VertexGenerator = [&](auto ϕ, auto θ) {
            auto [LatitudeSamplingStep, LongitudeSamplingStep] = std::array{ pi / LatitudeQuantizationFactor, 2 * pi / LongitudeQuantizationFactor };
            auto [y, LongitudeRadius] = ImplementationDetail::PolarToCartesian(0.5, ϕ * LatitudeSamplingStep);
            auto [x, z] = ImplementationDetail::PolarToCartesian(LongitudeRadius, θ * LongitudeSamplingStep);
            return Vertex{ .Coordinates = { x, y, z }, .Normal = { 2 * x, 2 * y, 2 * z } };
        };
        for (auto ϕ : Range{ LatitudeQuantizationFactor })
            for (auto θ : Range{ LongitudeQuantizationFactor })
                TessellatedSphere += ImplementationDetail::Tile(VertexGenerator, std::array{ ϕ, θ + 1 }, std::array{ ϕ, θ }, std::array{ ϕ + 1, θ + 1 }, std::array{ ϕ + 1, θ });
        return TessellatedSphere;
    }
}
