#pragma once
#include <glm/gtc/noise.hpp>
#include "Settings.h"
#include "../RayMarching.hxx"



constexpr auto CreateSphere = [](auto&& Center, auto Radius) {
    return [=, Center = Forward(Center)](auto&& Position) { return glm::length(Position - Center) - Radius; };
};

 constexpr auto CreateMandelbulb=[](auto Power,auto scale, auto&& Center, auto&& rotation_matrix){


    return [=, Center = Forward(Center), rotation_matrix=Forward(rotation_matrix)](auto&& p) {

        auto ro =[](auto&& a) {
            float s = sin(a), c = cos(a);
            return glm::mat2(c,-s,s,c);
        };



        auto pos = p-Center;

        pos = static_cast<float>(1./scale) * pos;
        pos = glm::inverse(rotation_matrix) * pos;



        auto z = glm::vec3{ pos };
        auto dr = 1.0;
        auto r = 0.0;
        for (auto _ : Range{ 5 }) {
            r = glm::length(z);
            if (r > 4.) break;
            // convert to polar coordinates
            auto theta = std::acos(z.z / r);
            auto phi = std::atan2(z.y, z.x);
            dr = std::pow(r, Power - 1) * Power * dr + 1;

            // scale and rotate the point
            auto zr = static_cast<float>(std::pow(r, Power));
            theta = theta *Power;
            phi = phi * Power;

            // convert back to cartesian coordinates
            z = zr * glm::vec3{
                std::sin(theta) * std::cos(phi),
                std::sin(phi) * std::sin(theta),
                std::cos(theta) };
            z += glm::vec3{ pos };
        }
        return scale * 0.5 * std::log(r) * r / dr;
    };


};

constexpr auto CreateTerrain=[](){

    return [=](auto&& p) {
        if (p.y > 1.05f) {
            return static_cast<double>(p.y);
        } else {
            auto noise = glm::perlin(0.5f * p.xz()) + 0.5 * glm::perlin(0.75f * p.xz());
            noise /= 1.5;
            // ridges or no?
//            noise = glm::round(noise * 8) / 8.f;
            noise = 1.f / (1 + std::exp(-(2 * noise - 1)));
            return static_cast<double>(p.y - noise);
        }
    };

};

constexpr auto CreateTree=[](auto depth,auto height,auto width, auto rxy,auto rzx, auto&& Center){





    return [=, Center = Forward(Center)](auto&& p) {

        auto ln =[](auto&& p, auto&& a, auto&& b, auto&& R) {
            double r = glm::dot(p-a,b-a)/glm::dot(b-a,b-a);
            r = std::clamp(r,0.,1.);
            return glm::length(p-a-(float)r*(b-a))-R*(1.5-0.4*r);
        };
        auto ro =[](auto&& a) {
            float s = sin(a), c = cos(a);
            return glm::mat2(c,-s,s,c);
        };

        double l = glm::length(p);
        auto pos= p;

        pos.x -= Center.x;
        pos.y -= Center.y;
        pos.z -= Center.z;
        float pi =3.14159;
        //pos.xz *= 1.;
        glm::vec2 rl = glm::vec2(width,height);
//        leaf=0;
        for (int i = 1; i <depth; i++) {
            l = std::min(l,ln(pos,glm::vec4(0),glm::vec4(0,rl.y,0,0),rl.x));
            pos.y -= rl.y;
            pos.x = abs(pos.x);
            auto tmp = glm::vec2{pos.x,pos.y} * ro(rxy);
            pos.x = tmp.x;
            pos.y = tmp.y;
            tmp = glm::vec2{pos.z,pos.x}*ro(rzx);
            pos.z =tmp.x;
            pos.x =tmp.y;

            rl *= (.7+0.015*float(i));

//            if( glm::length(pos)-0.15*sqrt(rl.x) <l && i> settings.fractalDepth - 4){
//                leaf=1;
//            }

            l=std::min(l,glm::length(pos)-0.15*sqrt(rl.x));
        }
        return l;

    };

};
