// EARTH-0 1000일 감사 브랜치: simulation truth는 PR #674와 완전히 동일.
#include "lifelens/EarthSurfaceCoordinates.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

using namespace lifelens;
namespace {
bool close(double a,double b,double tolerance=1e-7)
{return std::abs(a-b)<=tolerance;}
void samePlace(EarthGeoPoint a,EarthGeoPoint b,double tol=1e-9)
{
    const EarthVector3 x=earthGlobeUnitVector(a);
    const EarthVector3 y=earthGlobeUnitVector(b);
    assert(earthLength(earthAdd(x,earthScale(y,-1.0)))<tol);
}
}

int main()
{
    // Real coordinates are supplied explicitly by a scenario, never inferred
    // from legacy WorldSeed, a webcam, or an observer's camera location.
    assert(!earthValidGeo({91.0,0.0}));
    assert(!earthValidGeo({0.0,181.0}));
    assert(!earthValidGeo({std::numeric_limits<double>::quiet_NaN(),0.0}));
    assert(close(earthWrapLongitude(180.0),-180.0));
    assert(close(earthWrapLongitude(-181.0),179.0));

    const std::vector<EarthGeoPoint> places={
        {35.1796,129.0756}, // Busan (spatial reference, not a modern city)
        {-33.9249,18.4241},{0.0,179.999},{0.0,-179.999},
        {89.99999,30.0},{-89.99999,-135.0},
        {0.0,0.0},{37.0,-122.0},{20.0,90.0}
    };
    for(EarthGeoPoint point:places){
        EarthGeoPoint recovered{};
        assert(earthGeoForUnitVector(earthGlobeUnitVector(point),recovered));
        samePlace(point,recovered);
        EarthCubeSurfaceUV face{};
        assert(earthCubeUVForGeo(point,face));
        assert(face.u>=-1.0&&face.u<=1.0);
        assert(face.v>=-1.0&&face.v<=1.0);
        assert(earthGeoForCubeUV(face,recovered));
        samePlace(point,recovered);
    }
    EarthGeoPoint pole{};
    assert(earthGeoForUnitVector({0.0,0.0,1.0},pole));
    assert(close(pole.latitudeDeg,90.0)&&close(pole.longitudeDeg,0.0));
    assert(earthGeoForUnitVector({0.0,0.0,-1.0},pole));
    assert(close(pole.latitudeDeg,-90.0)&&close(pole.longitudeDeg,0.0));

    // Antimeridian crossing is continuous on the sphere.
    EarthGeoPoint across{};
    EarthLocalOffsetMeters inverse{};
    assert(earthGeoAtLocalOffset({0.0,179.999},{1000.0,0.0},across));
    assert(across.longitudeDeg<0.0);
    assert(earthLocalOffsetForGeo({0.0,179.999},across,inverse));
    assert(close(inverse.east,1000.0,1e-5));
    assert(close(inverse.north,0.0,1e-5));

    // Crossing either pole must not explode or force a longitude singularity.
    EarthGeoPoint beyondPole{};
    assert(earthGeoAtLocalOffset({89.999,15.0},{0.0,2000.0},beyondPole));
    assert(earthValidGeo(beyondPole));
    assert(earthLocalOffsetForGeo({89.999,15.0},beyondPole,inverse));
    assert(close(inverse.east,0.0,1e-5));
    assert(close(inverse.north,2000.0,1e-5));

    const auto identity=makeWorldGenesisIdentity(874213954ULL,111ULL);
    const auto alternate=makeWorldGenesisIdentity(874213954ULL,222ULL);
    const auto region=deriveSurfaceRegionIdentityForChunk(identity,{1,-2});
    const auto regionOtherPopulation=
        deriveSurfaceRegionIdentityForChunk(alternate,{1,-2});
    assert(region.id==regionOtherPopulation.id);
    // No default meters-per-cell and no default Earth location.
    EarthRegionGeoAnchor anchor{};
    EarthGeoPoint output{};
    assert(!earthGeoForGrid(anchor,{0,0},output));
    anchor={region.id,{35.1796,129.0756},{-100,-100},25.0};
    assert(earthValidGeoAnchor(anchor));
    for(GridPos grid:std::vector<GridPos>{
        {-100,-100},{-200,-150},{0,0},{100,200},{1000,1000}}){
        assert(earthGeoForGrid(anchor,grid,output));
        double x=0.0,y=0.0;
        assert(earthContinuousGridForGeo(anchor,output,x,y));
        assert(close(x,grid.x,2e-6));
        assert(close(y,grid.y,2e-6));
    }
    assert(chunkCoordForGrid({-100,-100})==(ChunkCoord{-4,-4}));
    assert(surfaceRegionCoordForChunk({-4,-4})
        ==(SurfaceRegionCoord{-1,-1}));

    // Using an anchor for distant Earth points is an error, not accidental
    // teleportation through a flat plane or a fabricated local geography.
    assert(!earthGeoForGrid(anchor,{20000,0},output));
    double x=0.0,y=0.0;
    assert(!earthContinuousGridForGeo(anchor,{-35.0,-50.0},x,y));
    anchor.metersPerGridCell=0.0;
    assert(!earthValidGeoAnchor(anchor));
    assert(!earthGeoForCubeUV({EarthCubeFace::PositiveX,2.0,0.0},output));
    assert(!earthGeoForUnitVector({0.0,0.0,0.0},output));
    return 0;
}
