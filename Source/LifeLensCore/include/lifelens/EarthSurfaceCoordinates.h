#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include "WorldHierarchy.h"

namespace lifelens {

// EARTH-0: Pure, read-only spherical coordinate adapter. This is NOT a
// replacement for the flat Core grid or a geography/weather generator.
// Earth terrain truth will be supplied by a separately versioned data pack.
inline constexpr double EarthMeanRadiusMeters=6371008.8;
inline constexpr double EarthMathPi=3.1415926535897932384626433832795;
inline constexpr double EarthMaxLocalProjectionMeters=250000.0;

struct EarthGeoPoint {
    double latitudeDeg=0.0;
    double longitudeDeg=0.0;
};

struct EarthVector3 {
    double x=0.0,y=0.0,z=0.0;
};

struct EarthLocalOffsetMeters {
    double east=0.0,north=0.0;
};

// Explicit opt-in mapping. A legacy chunk has NO geographical position
// unless an EarthScenario provides a validated anchor for its region.
// metersPerGridCell cannot be inferred from the current abstract grid.
struct EarthRegionGeoAnchor {
    SurfaceRegionId regionId=0;
    EarthGeoPoint origin{};
    GridPos gridOrigin{};
    double metersPerGridCell=0.0;
};

enum class EarthCubeFace : std::uint8_t {
    PositiveX=0,NegativeX,PositiveY,NegativeY,PositiveZ,NegativeZ
};

struct EarthCubeSurfaceUV {
    EarthCubeFace face=EarthCubeFace::PositiveX;
    double u=0.0,v=0.0;
};

inline double earthRadians(double degrees) {return degrees*(EarthMathPi/180.0);}
inline double earthDegrees(double radians) {return radians*(180.0/EarthMathPi);}

inline bool earthValidGeo(EarthGeoPoint p)
{
    return std::isfinite(p.latitudeDeg)&&std::isfinite(p.longitudeDeg)
        &&p.latitudeDeg>=-90.0&&p.latitudeDeg<=90.0
        &&p.longitudeDeg>=-180.0&&p.longitudeDeg<=180.0;
}

inline double earthWrapLongitude(double longitudeDeg)
{
    if(!std::isfinite(longitudeDeg)) return std::numeric_limits<double>::quiet_NaN();
    double result=std::fmod(longitudeDeg+180.0,360.0);
    if(result<0.0) result+=360.0;
    return result-180.0;
}

inline EarthVector3 earthAdd(EarthVector3 a,EarthVector3 b)
{
    return {a.x+b.x,a.y+b.y,a.z+b.z};
}
inline EarthVector3 earthScale(EarthVector3 a,double scalar)
{
    return {a.x*scalar,a.y*scalar,a.z*scalar};
}
inline double earthDot(EarthVector3 a,EarthVector3 b)
{
    return a.x*b.x+a.y*b.y+a.z*b.z;
}
inline double earthLength(EarthVector3 a)
{
    return std::hypot(std::hypot(a.x,a.y),a.z);
}
inline EarthVector3 earthUnit(EarthVector3 a)
{
    const double length=earthLength(a);
    if(!(length>0.0)||!std::isfinite(length)) return {};
    return earthScale(a,1.0/length);
}
inline EarthVector3 earthGlobeUnitVector(EarthGeoPoint point)
{
    if(!earthValidGeo(point)) return {};
    const double latitude=earthRadians(point.latitudeDeg);
    const double longitude=earthRadians(earthWrapLongitude(point.longitudeDeg));
    const double cosLat=std::cos(latitude);
    return {cosLat*std::cos(longitude),
            cosLat*std::sin(longitude),std::sin(latitude)};
}

inline bool earthGeoForUnitVector(EarthVector3 vector,EarthGeoPoint& result)
{
    const double length=earthLength(vector);
    if(!(length>0.0)||!std::isfinite(length)) return false;
    vector=earthScale(vector,1.0/length);
    const double horizontal=std::hypot(vector.x,vector.y);
    const double latitude=earthDegrees(std::atan2(vector.z,horizontal));
    // Longitude at the exact poles is physically undefined; canonicalize to 0.
    const double longitude=horizontal<1e-14 ? 0.0
        : earthWrapLongitude(earthDegrees(std::atan2(vector.y,vector.x)));
    result={latitude,longitude};
    return true;
}

inline EarthVector3 earthEastAt(EarthGeoPoint point)
{
    const double longitude=earthRadians(earthWrapLongitude(point.longitudeDeg));
    return {-std::sin(longitude),std::cos(longitude),0.0};
}
inline EarthVector3 earthNorthAt(EarthGeoPoint point)
{
    const double latitude=earthRadians(point.latitudeDeg);
    const double longitude=earthRadians(earthWrapLongitude(point.longitudeDeg));
    return {-std::sin(latitude)*std::cos(longitude),
            -std::sin(latitude)*std::sin(longitude),
            std::cos(latitude)};
}

// Geodesic exponential/logarithm map on a reference sphere. Distances are
// measured along the surface (not an invented flat Earth). Limited to local
// patches so a local grid never wraps to an unrelated region/antipodal point.
inline bool earthGeoAtLocalOffset(
    EarthGeoPoint anchor,EarthLocalOffsetMeters offset,EarthGeoPoint& result)
{
    if(!earthValidGeo(anchor)||!std::isfinite(offset.east)
       ||!std::isfinite(offset.north)) return false;
    const double distance=std::hypot(offset.east,offset.north);
    if(!std::isfinite(distance)||distance>EarthMaxLocalProjectionMeters)
        return false;
    if(distance==0.0){
        result={anchor.latitudeDeg,earthWrapLongitude(anchor.longitudeDeg)};
        return true;
    }
    const double angle=distance/EarthMeanRadiusMeters;
    const EarthVector3 origin=earthGlobeUnitVector(anchor);
    const EarthVector3 heading=earthScale(
        earthAdd(earthScale(earthEastAt(anchor),offset.east),
                 earthScale(earthNorthAt(anchor),offset.north)),1.0/distance);
    const EarthVector3 target=earthAdd(
        earthScale(origin,std::cos(angle)),
        earthScale(heading,std::sin(angle)));
    return earthGeoForUnitVector(target,result);
}

inline bool earthLocalOffsetForGeo(
    EarthGeoPoint anchor,EarthGeoPoint target,EarthLocalOffsetMeters& result)
{
    if(!earthValidGeo(anchor)||!earthValidGeo(target)) return false;
    const EarthVector3 origin=earthGlobeUnitVector(anchor);
    const EarthVector3 destination=earthGlobeUnitVector(target);
    const double cosine=std::clamp(earthDot(origin,destination),-1.0,1.0);
    const EarthVector3 tangent=earthAdd(
        destination,earthScale(origin,-cosine));
    const double sine=earthLength(tangent);
    const double distance=EarthMeanRadiusMeters*std::atan2(sine,cosine);
    if(!std::isfinite(distance)||distance>EarthMaxLocalProjectionMeters)
        return false;
    if(distance==0.0){result={};return true;}
    const double scale=distance/sine;
    result={
        earthDot(tangent,earthEastAt(anchor))*scale,
        earthDot(tangent,earthNorthAt(anchor))*scale
    };
    return std::isfinite(result.east)&&std::isfinite(result.north);
}

inline bool earthValidGeoAnchor(const EarthRegionGeoAnchor& anchor)
{
    return anchor.regionId!=0&&earthValidGeo(anchor.origin)
        &&std::isfinite(anchor.metersPerGridCell)
        &&anchor.metersPerGridCell>0.0;
}

inline bool earthGeoForGrid(
    const EarthRegionGeoAnchor& anchor,GridPos grid,EarthGeoPoint& result)
{
    if(!earthValidGeoAnchor(anchor)) return false;
    const EarthLocalOffsetMeters offset={
        (static_cast<double>(grid.x)-anchor.gridOrigin.x)*anchor.metersPerGridCell,
        (static_cast<double>(grid.y)-anchor.gridOrigin.y)*anchor.metersPerGridCell
    };
    return earthGeoAtLocalOffset(anchor.origin,offset,result);
}

inline bool earthContinuousGridForGeo(
    const EarthRegionGeoAnchor& anchor,EarthGeoPoint point,
    double& gridX,double& gridY)
{
    if(!earthValidGeoAnchor(anchor)) return false;
    EarthLocalOffsetMeters offset{};
    if(!earthLocalOffsetForGeo(anchor.origin,point,offset)) return false;
    const double x=anchor.gridOrigin.x+offset.east/anchor.metersPerGridCell;
    const double y=anchor.gridOrigin.y+offset.north/anchor.metersPerGridCell;
    if(!std::isfinite(x)||!std::isfinite(y)) return false;
    gridX=x;gridY=y;
    return true;
}

// Six face coordinates are presentation/data-pack tile addressing only;
// they do not replace the existing WorldGenesis SurfaceRegionId scheme.
inline bool earthCubeUVForGeo(EarthGeoPoint point,EarthCubeSurfaceUV& result)
{
    if(!earthValidGeo(point)) return false;
    const EarthVector3 p=earthGlobeUnitVector(point);
    const double ax=std::abs(p.x),ay=std::abs(p.y),az=std::abs(p.z);
    if(ax>=ay&&ax>=az){
        result={p.x>=0.0?EarthCubeFace::PositiveX:EarthCubeFace::NegativeX,
            p.y/ax,p.z/ax};
    }else if(ay>=az){
        result={p.y>=0.0?EarthCubeFace::PositiveY:EarthCubeFace::NegativeY,
            p.x/ay,p.z/ay};
    }else{
        result={p.z>=0.0?EarthCubeFace::PositiveZ:EarthCubeFace::NegativeZ,
            p.x/az,p.y/az};
    }
    return true;
}
inline bool earthGeoForCubeUV(EarthCubeSurfaceUV cube,EarthGeoPoint& result)
{
    if(!std::isfinite(cube.u)||!std::isfinite(cube.v)
       ||std::abs(cube.u)>1.0||std::abs(cube.v)>1.0) return false;
    EarthVector3 v{};
    switch(cube.face){
        case EarthCubeFace::PositiveX: v={1.0,cube.u,cube.v}; break;
        case EarthCubeFace::NegativeX: v={-1.0,cube.u,cube.v}; break;
        case EarthCubeFace::PositiveY: v={cube.u,1.0,cube.v}; break;
        case EarthCubeFace::NegativeY: v={cube.u,-1.0,cube.v}; break;
        case EarthCubeFace::PositiveZ: v={cube.u,cube.v,1.0}; break;
        case EarthCubeFace::NegativeZ: v={cube.u,cube.v,-1.0}; break;
        default:return false;
    }
    return earthGeoForUnitVector(earthUnit(v),result);
}

} // namespace lifelens
