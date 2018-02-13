// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //
 
#include "curve_intersector.h"
#include "intersector_epilog.h"
#include "bezierNi_intersector.h"
#include "bezierNv_intersector.h"

namespace embree
{
  namespace isa
  {
    template<typename Curve3fa, int N>
    static VirtualCurvePrimitive::Intersectors RibbonNiIntersectors()
    {
      VirtualCurvePrimitive::Intersectors intersectors;
      intersectors.intersect1 = (VirtualCurvePrimitive::Intersect1Ty)&BezierNiIntersector1<N>::template intersect<Ribbon1Intersector1<Curve3fa>, Intersect1EpilogMU<VSIZEX,true> >;
      intersectors.occluded1  = (VirtualCurvePrimitive::Occluded1Ty) &BezierNiIntersector1<N>::template occluded <Ribbon1Intersector1<Curve3fa>, Occluded1EpilogMU<VSIZEX,true> >;
      intersectors.intersect4 = (VirtualCurvePrimitive::Intersect4Ty)&BezierNiIntersectorK<N,4>::template intersect<Ribbon1IntersectorK<Curve3fa,4>, Intersect1KEpilogMU<VSIZEX,4,true> >;
      intersectors.occluded4  = (VirtualCurvePrimitive::Occluded4Ty) &BezierNiIntersectorK<N,4>::template occluded <Ribbon1IntersectorK<Curve3fa,4>, Occluded1KEpilogMU<VSIZEX,4,true> >;
#if defined(__AVX__)
      intersectors.intersect8 = (VirtualCurvePrimitive::Intersect8Ty)&BezierNiIntersectorK<N,8>::template intersect<Ribbon1IntersectorK<Curve3fa,8>, Intersect1KEpilogMU<VSIZEX,8,true> >;
      intersectors.occluded8  = (VirtualCurvePrimitive::Occluded8Ty) &BezierNiIntersectorK<N,8>::template occluded <Ribbon1IntersectorK<Curve3fa,8>, Occluded1KEpilogMU<VSIZEX,8,true> >;
#endif
#if defined(__AVX512F__)
      intersectors.intersect16 = (VirtualCurvePrimitive::Intersect16Ty)&BezierNiIntersectorK<N,16>::template intersect<Ribbon1IntersectorK<Curve3fa,16>, Intersect1KEpilogMU<VSIZEX,16,true> >;
      intersectors.occluded16  = (VirtualCurvePrimitive::Occluded16Ty) &BezierNiIntersectorK<N,16>::template occluded <Ribbon1IntersectorK<Curve3fa,16>, Occluded1KEpilogMU<VSIZEX,16,true> >;
#endif
      return intersectors;
    }

    template<typename Curve3fa, int N>
    static VirtualCurvePrimitive::Intersectors CurveNiIntersectors()
    {
      VirtualCurvePrimitive::Intersectors intersectors;
      intersectors.intersect1 = (VirtualCurvePrimitive::Intersect1Ty)&BezierNiIntersector1<N>::template intersect<BezierCurve1Intersector1<Curve3fa>, Intersect1Epilog1<true> >;
      intersectors.occluded1  = (VirtualCurvePrimitive::Occluded1Ty) &BezierNiIntersector1<N>::template occluded <BezierCurve1Intersector1<Curve3fa>, Occluded1Epilog1<true> >;
      intersectors.intersect4 = (VirtualCurvePrimitive::Intersect4Ty)&BezierNiIntersectorK<N,4>::template intersect<BezierCurve1IntersectorK<Curve3fa,4>, Intersect1KEpilog1<4,true> >;
      intersectors.occluded4  = (VirtualCurvePrimitive::Occluded4Ty) &BezierNiIntersectorK<N,4>::template occluded <BezierCurve1IntersectorK<Curve3fa,4>, Occluded1KEpilog1<4,true> >;
#if defined(__AVX__)
      intersectors.intersect8 = (VirtualCurvePrimitive::Intersect8Ty)&BezierNiIntersectorK<N,8>::template intersect<BezierCurve1IntersectorK<Curve3fa,8>, Intersect1KEpilog1<8,true> >;
      intersectors.occluded8  = (VirtualCurvePrimitive::Occluded8Ty) &BezierNiIntersectorK<N,8>::template occluded <BezierCurve1IntersectorK<Curve3fa,8>, Occluded1KEpilog1<8,true> >;
#endif
#if defined(__AVX512F__)
      intersectors.intersect16 = (VirtualCurvePrimitive::Intersect16Ty)&BezierNiIntersectorK<N,16>::template intersect<BezierCurve1IntersectorK<Curve3fa,16>, Intersect1KEpilog1<16,true> >;
      intersectors.occluded16  = (VirtualCurvePrimitive::Occluded16Ty) &BezierNiIntersectorK<N,16>::template occluded <BezierCurve1IntersectorK<Curve3fa,16>, Occluded1KEpilog1<16,true> >;
#endif
      return intersectors;
    }
    
    VirtualCurvePrimitive* VirtualCurvePrimitiveIntersector4i()
    {
      static VirtualCurvePrimitive prim;
      prim.vtbl[RTC_GEOMETRY_TYPE_ROUND_BEZIER_CURVE] = CurveNiIntersectors <BezierCurve3fa,4>();
      prim.vtbl[RTC_GEOMETRY_TYPE_FLAT_BEZIER_CURVE ] = RibbonNiIntersectors<BezierCurve3fa,4>();
      prim.vtbl[RTC_GEOMETRY_TYPE_ROUND_BSPLINE_CURVE] = CurveNiIntersectors <BSplineCurve3fa,4>();
      prim.vtbl[RTC_GEOMETRY_TYPE_FLAT_BSPLINE_CURVE ] = RibbonNiIntersectors<BSplineCurve3fa,4>();
      return &prim;
    }
  }
}
