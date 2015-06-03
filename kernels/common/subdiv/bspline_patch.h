// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#pragma once

#include "catmullclark_patch.h"

namespace embree
{
  class CubicBSplineCurve
  {
  public:

    template<class T>
      static __forceinline Vec4<T>  eval(const T& u)
    {
      const T t  = u;
      const T s  = T(1.0f) - u;
      const T n0 = s*s*s;
      const T n1 = (4.0f*s*s*s+t*t*t) + (12.0f*s*t*s + 6.0*t*s*t);
      const T n2 = (4.0f*t*t*t+s*s*s) + (12.0f*t*s*t + 6.0*s*t*s);
      const T n3 = t*t*t;
      return Vec4<T>(n0,n1,n2,n3);
    }
    
    template<class T>
      static __forceinline Vec4<T>  derivative(const T& u)
    {
      const T t  =  u;
      const T s  =  1.0f - u;
      const T n0 = -s*s;
      const T n1 = -t*t - 4.0f*t*s;
      const T n2 =  s*s + 4.0f*s*t;
      const T n3 =  t*t;
      /*const T n0 = -3.0f*s*s;
      const T n1 = -3.0f*t*t - 12.0f*t*s;
      const T n2 = +3.0f*s*s + 12.0f*t*s;
      const T n3 = +3.0f*t*t;*/
      return Vec4<T>(3.0f*n0,3.0f*n1,3.0f*n2,3.0f*n3);
    }
  };

  template<typename Vertex, typename Vertex_t = Vertex>
    class __aligned(64) BSplinePatchT
    {
      typedef CatmullClark1RingT<Vertex,Vertex_t> CatmullClarkRing;
      typedef CatmullClarkPatchT<Vertex,Vertex_t> CatmullClarkPatch;
      
    public:
      Vertex v[4][4];
      
      __forceinline Vertex computeFaceVertex(const unsigned int y,const unsigned int x) const
      {
        return (v[y][x] + v[y][x+1] + v[y+1][x+1] + v[y+1][x]) * 0.25f;
      }
      
      __forceinline Vertex computeQuadVertex(const unsigned int y, const unsigned int x, const Vertex face[3][3]) const
      {
	const Vertex P = v[y][x]; 
	const Vertex Q = face[y-1][x-1] + face[y-1][x] + face[y][x] + face[y][x-1];
	const Vertex R = v[y-1][x] + v[y+1][x] + v[y][x-1] + v[y][x+1];
	const Vertex res = (Q + R) * 0.0625f + P * 0.5f;
	return res;
      }
      
      __forceinline Vertex computeLimitVertex(const int y, const int x) const // FIXME: 
      {
	const Vertex P = v[y][x];
	const Vertex Q = v[y-1][x-1] + v[y-1][x+1] + v[y+1][x-1] + v[y+1][x+1];
	const Vertex R = v[y-1][x] + v[y+1][x] + v[y][x-1] + v[y][x+1];
	const Vertex res = (P * 16.0f + R * 4.0f + Q) * 1.0f / 36.0f;
	return res;
      }
      
      __forceinline Vertex computeLimitTangentX(const int y, const int x) const // FIXME:
      {
	/* --- tangent X --- */
	const Vertex Qx = v[y-1][x+1] - v[y-1][x-1] + v[y+1][x+1] - v[y+1][x-1];
	const Vertex Rx = v[y][x-1] - v[y][x+1];
	const Vertex tangentX = (Rx * 4.0f + Qx) * 1.0f / 12.0f;
        return tangentX;
      };
      
      __forceinline Vertex computeLimitTangentY(const int y, const int x) const // FIXME:
      {
	const Vertex Qy = v[y-1][x-1] - v[y+1][x-1] + v[y-1][x+1] - v[y+1][x+1];
	const Vertex Ry = v[y-1][x] - v[y+1][x];
	const Vertex tangentY = (Ry * 4.0f + Qy) * 1.0f / 12.0f;
        
	return tangentY;
      }
      
      __forceinline Vertex computeLimitNormal(const int y, const int x) const // FIXME:
      {
	/* --- tangent X --- */
	const Vertex Qx = v[y-1][x+1] - v[y-1][x-1] + v[y+1][x+1] - v[y+1][x-1];
	const Vertex Rx = v[y][x-1] - v[y][x+1];
	const Vertex tangentX = (Rx * 4.0f + Qx) * 1.0f / 12.0f;
        
	/* --- tangent Y --- */
	const Vertex Qy = v[y-1][x-1] - v[y+1][x-1] + v[y-1][x+1] - v[y+1][x+1];
	const Vertex Ry = v[y-1][x] - v[y+1][x];
	const Vertex tangentY = (Ry * 4.0f + Qy) * 1.0f / 12.0f;
        
	return cross(tangentY,tangentX);
      }
      
      __forceinline void initSubPatches(const Vertex edge[12],
					const Vertex face[3][3],
					const Vertex newQuadVertex[2][2],
					BSplinePatchT child[4]) const
      {
	BSplinePatchT& subTL = child[0];
	BSplinePatchT& subTR = child[1];
	BSplinePatchT& subBR = child[2];
	BSplinePatchT& subBL = child[3];
        
	// top-left
	subTL.v[0][0] = face[0][0];
	subTL.v[0][1] = edge[0];
	subTL.v[0][2] = face[0][1];
	subTL.v[0][3] = edge[1];
        
	subTL.v[1][0] = edge[2];
	subTL.v[1][1] = newQuadVertex[0][0];
	subTL.v[1][2] = edge[3];
	subTL.v[1][3] = newQuadVertex[0][1];
        
	subTL.v[2][0] = face[1][0];
	subTL.v[2][1] = edge[5];
	subTL.v[2][2] = face[1][1];
	subTL.v[2][3] = edge[6];
        
	subTL.v[3][0] = edge[7];
	subTL.v[3][1] = newQuadVertex[1][0];
	subTL.v[3][2] = edge[8];
	subTL.v[3][3] = newQuadVertex[1][1];
        
	// top-right
	subTR.v[0][0] = edge[0];
	subTR.v[0][1] = face[0][1];
	subTR.v[0][2] = edge[1];
	subTR.v[0][3] = face[0][2];
        
	subTR.v[1][0] = newQuadVertex[0][0];
	subTR.v[1][1] = edge[3];
	subTR.v[1][2] = newQuadVertex[0][1];
	subTR.v[1][3] = edge[4];
        
	subTR.v[2][0] = edge[5];
	subTR.v[2][1] = face[1][1];
	subTR.v[2][2] = edge[6];
	subTR.v[2][3] = face[1][2];
        
	subTR.v[3][0] = newQuadVertex[1][0];
	subTR.v[3][1] = edge[8];
	subTR.v[3][2] = newQuadVertex[1][1];
	subTR.v[3][3] = edge[9];
        
	// buttom-right
	subBR.v[0][0] = newQuadVertex[0][0];
	subBR.v[0][1] = edge[3];
	subBR.v[0][2] = newQuadVertex[0][1];
	subBR.v[0][3] = edge[4];
        
	subBR.v[1][0] = edge[5];
	subBR.v[1][1] = face[1][1];
	subBR.v[1][2] = edge[6];
	subBR.v[1][3] = face[1][2];
        
	subBR.v[2][0] = newQuadVertex[1][0];
	subBR.v[2][1] = edge[8];
	subBR.v[2][2] = newQuadVertex[1][1];
	subBR.v[2][3] = edge[9];
        
	subBR.v[3][0] = edge[10];
	subBR.v[3][1] = face[2][1];
	subBR.v[3][2] = edge[11];
	subBR.v[3][3] = face[2][2];
        
	// buttom-left
	subBL.v[0][0] = edge[2];
	subBL.v[0][1] = newQuadVertex[0][0];
	subBL.v[0][2] = edge[3];
	subBL.v[0][3] = newQuadVertex[0][1];
        
	subBL.v[1][0] = face[1][0];
	subBL.v[1][1] = edge[5];
	subBL.v[1][2] = face[1][1];
	subBL.v[1][3] = edge[6];
        
	subBL.v[2][0] = edge[7];
	subBL.v[2][1] = newQuadVertex[1][0];
	subBL.v[2][2] = edge[8];
	subBL.v[2][3] = newQuadVertex[1][1];
        
	subBL.v[3][0] = face[2][0];
	subBL.v[3][1] = edge[10];
	subBL.v[3][2] = face[2][1];
	subBL.v[3][3] = edge[11];
      }
      
      __forceinline void subdivide(BSplinePatchT child[4]) const
      {
	Vertex face[3][3];
	face[0][0] = computeFaceVertex(0,0);
	face[0][1] = computeFaceVertex(0,1);
	face[0][2] = computeFaceVertex(0,2);
	face[1][0] = computeFaceVertex(1,0);
	face[1][1] = computeFaceVertex(1,1);
	face[1][2] = computeFaceVertex(1,2);
	face[2][0] = computeFaceVertex(2,0);
	face[2][1] = computeFaceVertex(2,1);
	face[2][2] = computeFaceVertex(2,2);
        
	Vertex edge[12];
	edge[0]  = 0.25f * (v[0][1] + v[1][1] + face[0][0] + face[0][1]);
	edge[1]  = 0.25f * (v[0][2] + v[1][2] + face[0][1] + face[0][2]);
	edge[2]  = 0.25f * (v[1][0] + v[1][1] + face[0][0] + face[1][0]);
	edge[3]  = 0.25f * (v[1][1] + v[1][2] + face[0][1] + face[1][1]);
	edge[4]  = 0.25f * (v[1][2] + v[1][3] + face[0][2] + face[1][2]);
	edge[5]  = 0.25f * (v[1][1] + v[2][1] + face[1][0] + face[1][1]);
	edge[6]  = 0.25f * (v[1][2] + v[2][2] + face[1][1] + face[1][2]);
	edge[7]  = 0.25f * (v[2][0] + v[2][1] + face[1][0] + face[2][0]);
	edge[8]  = 0.25f * (v[2][1] + v[2][2] + face[1][1] + face[2][1]);
	edge[9]  = 0.25f * (v[2][2] + v[2][3] + face[1][2] + face[2][2]);
	edge[10] = 0.25f * (v[2][1] + v[3][1] + face[2][0] + face[2][1]);
	edge[11] = 0.25f * (v[2][2] + v[3][2] + face[2][1] + face[2][2]);
        
	Vertex newQuadVertex[2][2];
	newQuadVertex[0][0] = computeQuadVertex(1,1,face);
	newQuadVertex[0][1] = computeQuadVertex(1,2,face);
	newQuadVertex[1][1] = computeQuadVertex(2,2,face);
	newQuadVertex[1][0] = computeQuadVertex(2,1,face);
        
	initSubPatches(edge,face,newQuadVertex,child);
      }
    public:
      
      BSplinePatchT () {}
      
      __forceinline void init( FinalQuad& quad ) const
      {
        quad.vtx[0] = v[1][1];
        quad.vtx[1] = v[1][2];
        quad.vtx[2] = v[2][2];
        quad.vtx[3] = v[2][1];
      };
      
      __forceinline void init_limit( FinalQuad& quad ) const
      {
        
        const Vertex limit_v0 = computeLimitVertex(1,1);
        const Vertex limit_v1 = computeLimitVertex(1,2);
        const Vertex limit_v2 = computeLimitVertex(2,2);
        const Vertex limit_v3 = computeLimitVertex(2,1);
        
        quad.vtx[0] = limit_v0;
        quad.vtx[1] = limit_v1;
        quad.vtx[2] = limit_v2;
        quad.vtx[3] = limit_v3;
      };

      __forceinline Vertex hard_corner(const                    Vertex& v01, const Vertex& v02, 
                                       const Vertex& v10, const Vertex& v11, const Vertex& v12, 
                                       const Vertex& v20, const Vertex& v21, const Vertex& v22)
      {
        return -4.0f*((v01+v21)+(v10+v12)) - ((v02+v20)+v22) + 20.0f*v11;
      }

      __forceinline Vertex soft_concave_corner(const                    Vertex& v01, const Vertex& v02, 
                                               const Vertex& v10, const Vertex& v11, const Vertex& v12, 
                                               const Vertex& v20, const Vertex& v21, const Vertex& v22)
      {
        return 2.0f*(v01+v10)+8.0f*v11-(v20+v02)-4.0f*(v21+v12)-v22;
      }

      __forceinline Vertex soft_convex_corner( const                    Vertex& v01, const Vertex& v02, 
                                               const Vertex& v10, const Vertex& v11, const Vertex& v12, 
                                               const Vertex& v20, const Vertex& v21, const Vertex& v22)
      {
        return -4.0f*(v10+v01) - (v20 + v02) - v22 + 2.0f*(v12 + v21) + 8.0f*v11;
      }

      __forceinline Vertex concave_corner(const float vertex_crease_weight, 
                                          const                    Vertex& v01, const Vertex& v02, 
                                          const Vertex& v10, const Vertex& v11, const Vertex& v12, 
                                          const Vertex& v20, const Vertex& v21, const Vertex& v22)
      {
        if      (vertex_crease_weight == 0.0f    ) return soft_concave_corner(v01,v02,v10,v11,v12,v20,v21,v22);
        else if (std::isinf(vertex_crease_weight)) return hard_corner(v01,v02,v10,v11,v12,v20,v21,v22);
        else assert(false);
      }

      __forceinline Vertex convex_corner(const float vertex_crease_weight, 
                                         const                    Vertex& v01, const Vertex& v02, 
                                         const Vertex& v10, const Vertex& v11, const Vertex& v12, 
                                         const Vertex& v20, const Vertex& v21, const Vertex& v22)
      {
        if      (vertex_crease_weight == 0.0f    ) return soft_convex_corner(v01,v02,v10,v11,v12,v20,v21,v22);
        else if (std::isinf(vertex_crease_weight)) return hard_corner(v01,v02,v10,v11,v12,v20,v21,v22);
        else assert(false);
      }

      __forceinline void init_border(const CatmullClarkRing& edge0,
                                     Vertex& v01, Vertex& v02,
                                     const Vertex& v11, const Vertex& v12,
                                     const Vertex& v21, const Vertex& v22)
      {
        if (likely(edge0.has_opposite_back(0)))
        {
          v01 = edge0.back(2);
          v02 = edge0.back(1);
        } else {
          v01 = 2.0f*v11-v21;
          v02 = 2.0f*v12-v22;
        }
      }

      __forceinline void init_border(const CatmullClarkRing& edge0,
                                          const Vertex& v00,       Vertex& v01, const Vertex& v02, 
                                          Vertex& v10,       const Vertex& v11, const Vertex& v12, 
                                          const Vertex& v20, const Vertex& v21, const Vertex& v22)
      {
        const bool opposite0 = edge0.has_opposite_back(0);
        const bool opposite1 = edge0.has_opposite_front(1);
        const bool opposite2 = edge0.has_opposite_front(2);
        const bool opposite3 = edge0.has_opposite_back(1);

        if (likely(opposite1)) v10 = edge0.front(4);
        else if (likely(opposite0 && opposite3)) v10 = edge0.back(4);
        else v10 = 2.0f*v11-v12;

        if (likely(opposite0)) v01 = edge0.back(2);
        else if (likely(opposite1 && opposite2)) v01 = edge0.front(6);
        else v01 = 2.0f*v11-v21;
      }

      __forceinline void init_corner(const CatmullClarkRing& edge0,
                                     Vertex& v00,       const Vertex& v01, const Vertex& v02, 
                                     const Vertex& v10, const Vertex& v11, const Vertex& v12, 
                                     const Vertex& v20, const Vertex& v21, const Vertex& v22)
      {
        const bool opposite0 = edge0.has_opposite_back(0);
        const bool opposite1 = edge0.has_opposite_front(1);
        const bool opposite2 = edge0.has_opposite_front(2);
        const bool opposite3 = edge0.has_opposite_back(1);
        if (likely(opposite0))
        {
          if (likely(opposite1))
          {
            if (likely(opposite3)) { assert(opposite2); v00 = edge0.back(3); }
            else { assert(!opposite2); v00 = concave_corner(edge0.vertex_crease_weight,v01,v02,v10,v11,v12,v20,v21,v22); }
          }
          else {
            if (likely(opposite3)) v00 = edge0.back(3);
            else v00 = 2.0f*v01-v02;
          }
        }
        else
        {
          if (likely(opposite1)) {
            if (likely(opposite2)) v00 = edge0.front(5);
            else v00 = 2.0f*v10-v20;
          }
          else
            v00 = convex_corner(edge0.vertex_crease_weight,v01,v02,v10,v11,v12,v20,v21,v22);
        }
      }
      
      void init(const CatmullClarkPatch& patch)
      {
        //assert( patch.isRegular() );
        
        /* fill inner vertices */
        const Vertex v11 = v[1][1] = patch.ring[0].vtx;
        const Vertex v12 = v[1][2] = patch.ring[1].vtx;
        const Vertex v22 = v[2][2] = patch.ring[2].vtx; 
        const Vertex v21 = v[2][1] = patch.ring[3].vtx; 
        
        /* fill border vertices */
        //init_border(patch.ring[0],v[0][1],v[0][2],v11,v12,v21,v22);
        //init_border(patch.ring[1],v[1][3],v[2][3],v12,v22,v11,v21);
        //init_border(patch.ring[2],v[3][2],v[3][1],v22,v21,v12,v11);
        //init_border(patch.ring[3],v[2][0],v[1][0],v21,v11,v22,v12);
        init_border(patch.ring[0],v[0][0],v[0][1],v[0][2],v[1][0],v11,v12,v[2][0],v21,v22);
        init_border(patch.ring[1],v[0][3],v[1][3],v[2][3],v[0][2],v12,v22,v[0][1],v11,v21);
        init_border(patch.ring[2],v[3][3],v[3][2],v[3][1],v[2][3],v22,v21,v[1][3],v12,v11);
        init_border(patch.ring[3],v[3][0],v[2][0],v[1][0],v[3][1],v21,v11,v[3][2],v22,v12);
        
        /* fill corner vertices */
        init_corner(patch.ring[0],v[0][0],v[0][1],v[0][2],v[1][0],v11,v12,v[2][0],v21,v22);
        init_corner(patch.ring[1],v[0][3],v[1][3],v[2][3],v[0][2],v12,v22,v[0][1],v11,v21);
        init_corner(patch.ring[2],v[3][3],v[3][2],v[3][1],v[2][3],v22,v21,v[1][3],v12,v11);
        init_corner(patch.ring[3],v[3][0],v[2][0],v[1][0],v[3][1],v21,v11,v[3][2],v22,v12);
      }
      
      template<typename Loader>
      __forceinline void init_border(const SubdivMesh::HalfEdge* edge0, Loader& load,
                                     Vertex& v01, Vertex& v02,
                                     const Vertex& v11, const Vertex& v12,
                                     const Vertex& v21, const Vertex& v22)
      {
        if (likely(edge0->hasOpposite())) 
        {
          const SubdivMesh::HalfEdge* e = edge0->opposite()->next()->next(); 
          v01 = load(e); 
          v02 = load(e->next());
        } else {
          v01 = 2.0f*v11-v21;
          v02 = 2.0f*v12-v22;
        }
      }
      
      template<typename Loader>
      __forceinline void init_corner(const SubdivMesh::HalfEdge* edge0, Loader& load,
                                     Vertex& v00, const Vertex& v01, const Vertex& v02, 
                                     const Vertex& v10, const Vertex& v11, const Vertex& v12, 
                                     const Vertex& v20, const Vertex& v21, const Vertex& v22)
      {
        const bool opposite0 = edge0->hasOpposite();
        const bool opposite1 = edge0->prev()->hasOpposite();
        if (likely(opposite0 && opposite1))
        {
          const SubdivMesh::HalfEdge* e = edge0->opposite()->next();
          if (likely(e->hasOpposite())) {
            assert(edge0->prev()->opposite()->prev()->hasOpposite());
            v00 = load(e->opposite()->prev());
          }
          else {
            assert(!edge0->prev()->opposite()->prev()->hasOpposite());
            assert(false); // concave corner not supported
          }
        } 
        else if (opposite1) v00 = 2.0f*v10-v20; // border rule
        else if (opposite0) v00 = 2.0f*v01-v02; // border rule
        else {
          if (edge0->vertex_crease_weight == 0.0f) 
            //v00 = -8.0f*v11 + 4.0f*(v12+v21) + v22; // soft corner rule
            v00 = -4.0f*(v10+v01) - v20 - v02 - v22; // soft corner rule
          else if (std::isinf(edge0->vertex_crease_weight))
            v00 =  4.0f*v11 - 2.0f*(v12+v21) + v22; // hard corner rule
          else
            assert(false);
        }
      }
      
      template<typename Loader>
      void init(const SubdivMesh::HalfEdge* edge0, Loader& load)
      {
        assert( edge0->isRegularFace() );
        
        /* fill inner vertices */
        const Vertex v11 = v[1][1] = load(edge0); const SubdivMesh::HalfEdge* edge1 = edge0->next();
        const Vertex v12 = v[1][2] = load(edge1); const SubdivMesh::HalfEdge* edge2 = edge1->next();
        const Vertex v22 = v[2][2] = load(edge2); const SubdivMesh::HalfEdge* edge3 = edge2->next();
        const Vertex v21 = v[2][1] = load(edge3); assert(edge0  == edge3->next());
        
        /* fill border vertices */
        init_border(edge0,load,v[0][1],v[0][2],v11,v12,v21,v22);
        init_border(edge1,load,v[1][3],v[2][3],v12,v22,v11,v21);
        init_border(edge2,load,v[3][2],v[3][1],v22,v21,v12,v11);
        init_border(edge3,load,v[2][0],v[1][0],v21,v11,v22,v12);
        
        /* fill corner vertices */
        init_corner(edge0,load,v[0][0],v[0][1],v[0][2],v[1][0],v11,v12,v[2][0],v21,v22);
        init_corner(edge1,load,v[0][3],v[1][3],v[2][3],v[0][2],v12,v22,v[0][1],v11,v21);
        init_corner(edge2,load,v[3][3],v[3][2],v[3][1],v[2][3],v22,v21,v[1][3],v12,v11);
        init_corner(edge3,load,v[3][0],v[2][0],v[1][0],v[3][1],v21,v11,v[3][2],v22,v12);
      }
      
      __forceinline BBox<Vertex> bounds() const
      {
        const Vertex* const cv = &v[0][0];
        BBox<Vertex> bounds (cv[0]);
        for (size_t i=1; i<16 ; i++)
          bounds.extend( cv[i] );
        return bounds;
      }
      
      __noinline Vertex eval(const float uu, const float vv) const // this has to be noinline to work around likely compiler bug in feature_adaptive_eval
      {
        const Vec4f v_n = CubicBSplineCurve::eval(vv);
        
        const Vertex_t curve0 = v_n[0] * v[0][0] + v_n[1] * v[1][0] + v_n[2] * v[2][0] + v_n[3] * v[3][0];
        const Vertex_t curve1 = v_n[0] * v[0][1] + v_n[1] * v[1][1] + v_n[2] * v[2][1] + v_n[3] * v[3][1];
        const Vertex_t curve2 = v_n[0] * v[0][2] + v_n[1] * v[1][2] + v_n[2] * v[2][2] + v_n[3] * v[3][2];
        const Vertex_t curve3 = v_n[0] * v[0][3] + v_n[1] * v[1][3] + v_n[2] * v[2][3] + v_n[3] * v[3][3];
        
        const Vec4f u_n = CubicBSplineCurve::eval(uu);
        
        return (u_n[0] * curve0 + u_n[1] * curve1 + u_n[2] * curve2 + u_n[3] * curve3) * (1.0f/36.0f);
      }
      
      __forceinline Vertex tangentU(const float uu, const float vv) const
      {
        const Vec4f v_n = CubicBSplineCurve::eval(vv);
        
        const Vertex_t curve0 = v_n[0] * v[0][0] + v_n[1] * v[1][0] + v_n[2] * v[2][0] + v_n[3] * v[3][0];
        const Vertex_t curve1 = v_n[0] * v[0][1] + v_n[1] * v[1][1] + v_n[2] * v[2][1] + v_n[3] * v[3][1];
        const Vertex_t curve2 = v_n[0] * v[0][2] + v_n[1] * v[1][2] + v_n[2] * v[2][2] + v_n[3] * v[3][2];
        const Vertex_t curve3 = v_n[0] * v[0][3] + v_n[1] * v[1][3] + v_n[2] * v[2][3] + v_n[3] * v[3][3];
        
        const Vec4f u_n = CubicBSplineCurve::derivative(uu);
        
        return (u_n[0] * curve0 + u_n[1] * curve1 + u_n[2] * curve2 + u_n[3] * curve3) * (1.0f/36.0f);
      }
      
      __forceinline Vertex tangentV(const float uu, const float vv) const
      {
        const Vec4f v_n = CubicBSplineCurve::derivative(vv);
        
        const Vertex_t curve0 = v_n[0] * v[0][0] + v_n[1] * v[1][0] + v_n[2] * v[2][0] + v_n[3] * v[3][0];
        const Vertex_t curve1 = v_n[0] * v[0][1] + v_n[1] * v[1][1] + v_n[2] * v[2][1] + v_n[3] * v[3][1];
        const Vertex_t curve2 = v_n[0] * v[0][2] + v_n[1] * v[1][2] + v_n[2] * v[2][2] + v_n[3] * v[3][2];
        const Vertex_t curve3 = v_n[0] * v[0][3] + v_n[1] * v[1][3] + v_n[2] * v[2][3] + v_n[3] * v[3][3];
        
        const Vec4f u_n = CubicBSplineCurve::eval(uu);
        
        return (u_n[0] * curve0 + u_n[1] * curve1 + u_n[2] * curve2 + u_n[3] * curve3) * (1.0f/36.0f);
      }
      
      __forceinline Vertex normal(const float uu, const float vv) const
      {
        const Vertex tu = tangentU(uu,vv);
        const Vertex tv = tangentV(uu,vv);
        return cross(tv,tu);
      }   
      
      template<class T>
      __forceinline Vec3<T> eval(const T& uu, const T& vv, const Vec4<T>& u_n, const Vec4<T>& v_n) const
      {
        const T curve0_x = v_n[0] * T(v[0][0].x) + v_n[1] * T(v[1][0].x) + v_n[2] * T(v[2][0].x) + v_n[3] * T(v[3][0].x);
        const T curve1_x = v_n[0] * T(v[0][1].x) + v_n[1] * T(v[1][1].x) + v_n[2] * T(v[2][1].x) + v_n[3] * T(v[3][1].x);
        const T curve2_x = v_n[0] * T(v[0][2].x) + v_n[1] * T(v[1][2].x) + v_n[2] * T(v[2][2].x) + v_n[3] * T(v[3][2].x);
        const T curve3_x = v_n[0] * T(v[0][3].x) + v_n[1] * T(v[1][3].x) + v_n[2] * T(v[2][3].x) + v_n[3] * T(v[3][3].x);
        const T x = (u_n[0] * curve0_x + u_n[1] * curve1_x + u_n[2] * curve2_x + u_n[3] * curve3_x) * T(1.0f/36.0f);
        
        
        const T curve0_y = v_n[0] * T(v[0][0].y) + v_n[1] * T(v[1][0].y) + v_n[2] * T(v[2][0].y) + v_n[3] * T(v[3][0].y);
        const T curve1_y = v_n[0] * T(v[0][1].y) + v_n[1] * T(v[1][1].y) + v_n[2] * T(v[2][1].y) + v_n[3] * T(v[3][1].y);
        const T curve2_y = v_n[0] * T(v[0][2].y) + v_n[1] * T(v[1][2].y) + v_n[2] * T(v[2][2].y) + v_n[3] * T(v[3][2].y);
        const T curve3_y = v_n[0] * T(v[0][3].y) + v_n[1] * T(v[1][3].y) + v_n[2] * T(v[2][3].y) + v_n[3] * T(v[3][3].y);
        const T y = (u_n[0] * curve0_y + u_n[1] * curve1_y + u_n[2] * curve2_y + u_n[3] * curve3_y) * T(1.0f/36.0f);
          
        const T curve0_z = v_n[0] * T(v[0][0].z) + v_n[1] * T(v[1][0].z) + v_n[2] * T(v[2][0].z) + v_n[3] * T(v[3][0].z);
        const T curve1_z = v_n[0] * T(v[0][1].z) + v_n[1] * T(v[1][1].z) + v_n[2] * T(v[2][1].z) + v_n[3] * T(v[3][1].z);
        const T curve2_z = v_n[0] * T(v[0][2].z) + v_n[1] * T(v[1][2].z) + v_n[2] * T(v[2][2].z) + v_n[3] * T(v[3][2].z);
        const T curve3_z = v_n[0] * T(v[0][3].z) + v_n[1] * T(v[1][3].z) + v_n[2] * T(v[2][3].z) + v_n[3] * T(v[3][3].z);
        const T z = (u_n[0] * curve0_z + u_n[1] * curve1_z + u_n[2] * curve2_z + u_n[3] * curve3_z) * T(1.0f/36.0f);
        
        return Vec3<T>(x,y,z);
      }
      
      template<typename T>
      __forceinline Vec3<T> eval(const T& uu, const T& vv) const
      {
        const Vec4<T> v_n = CubicBSplineCurve::eval(vv); // FIXME: precompute in table
        const Vec4<T> u_n = CubicBSplineCurve::eval(uu); // FIXME: precompute in table
        return eval(uu,vv,u_n,v_n);
      }
      
      template<typename T>
      __forceinline Vec3<T> tangentU(const T& uu, const T& vv) const
      {
        const Vec4<T> v_n = CubicBSplineCurve::derivative(vv); 
        const Vec4<T> u_n = CubicBSplineCurve::eval(uu); 
        return eval(uu,vv,u_n,v_n);      
      }
      
      template<typename T>
      __forceinline Vec3<T> tangentV(const T& uu, const T& vv) const
      {
        const Vec4<T> v_n = CubicBSplineCurve::eval(vv); 
        const Vec4<T> u_n = CubicBSplineCurve::derivative(uu); 
        return eval(uu,vv,u_n,v_n);      
      }
      
      template<typename T>
      __forceinline Vec3<T> normal(const T& uu, const T& vv) const
      {
        const Vec3<T> tU = tangentU(uu,vv);
        const Vec3<T> tV = tangentV(uu,vv);
        return cross(tU,tV);
      }
      
#if defined(__MIC__)
      
      __forceinline float16 getRow(const size_t i) const
      {
        return load16f(&v[i][0]);
      }
      
      __forceinline void prefetchData() const
      {
        prefetch<PFHINT_L1>(&v[0][0]);
        prefetch<PFHINT_L1>(&v[1][0]);
        prefetch<PFHINT_L1>(&v[2][0]);
        prefetch<PFHINT_L1>(&v[3][0]);
      }
      
      static __forceinline Vec4f16 eval_derivative(const float16 u, const bool16 m_mask)
      {
        const Vec4f16 e = CubicBSplineCurve::eval(u);
        const Vec4f16 d = CubicBSplineCurve::derivative(u);
        return Vec4f16(select(m_mask,e[0],d[0]),select(m_mask,e[1],d[1]),select(m_mask,e[2],d[2]),select(m_mask,e[3],d[3]));
      }    
      
      __forceinline float16 normal4(const float uu, const float vv) const
      {
        const Vec4f16 v_e_d = eval_derivative(float16(vv),0x00ff);       // ev,ev,dv,dv
        
        const float16 curve0 = v_e_d[0] * broadcast4to16f(&v[0][0]) + v_e_d[1] * broadcast4to16f(&v[1][0]) + v_e_d[2] * broadcast4to16f(&v[2][0]) + v_e_d[3] * broadcast4to16f(&v[3][0]);
        const float16 curve1 = v_e_d[0] * broadcast4to16f(&v[0][1]) + v_e_d[1] * broadcast4to16f(&v[1][1]) + v_e_d[2] * broadcast4to16f(&v[2][1]) + v_e_d[3] * broadcast4to16f(&v[3][1]);
        const float16 curve2 = v_e_d[0] * broadcast4to16f(&v[0][2]) + v_e_d[1] * broadcast4to16f(&v[1][2]) + v_e_d[2] * broadcast4to16f(&v[2][2]) + v_e_d[3] * broadcast4to16f(&v[3][2]);
        const float16 curve3 = v_e_d[0] * broadcast4to16f(&v[0][3]) + v_e_d[1] * broadcast4to16f(&v[1][3]) + v_e_d[2] * broadcast4to16f(&v[2][3]) + v_e_d[3] * broadcast4to16f(&v[3][3]);
        
        const Vec4f16 u_e_d = eval_derivative(float16(uu),0xff00);       // du,du,eu,eu
        
        const float16 tangentUV = (u_e_d[0] * curve0 + u_e_d[1] * curve1 + u_e_d[2] * curve2 + u_e_d[3] * curve3); // tu, tu, tv, tv
        
        const float16 tangentU = permute<0,0,0,0>(tangentUV);
        const float16 tangentV = permute<2,2,2,2>(tangentUV);
        
        const float16 n = lcross_xyz(tangentU,tangentV);
        return n;
      }
      
      __forceinline float16 eval4(const float16 uu, const float16 vv) const
      {
        const Vec4f16 v_n = CubicBSplineCurve::eval(vv); 
        
        const float16 curve0 = v_n[0] * broadcast4to16f(&v[0][0]) + v_n[1] * broadcast4to16f(&v[1][0]) + v_n[2] * broadcast4to16f(&v[2][0]) + v_n[3] * broadcast4to16f(&v[3][0]);
        const float16 curve1 = v_n[0] * broadcast4to16f(&v[0][1]) + v_n[1] * broadcast4to16f(&v[1][1]) + v_n[2] * broadcast4to16f(&v[2][1]) + v_n[3] * broadcast4to16f(&v[3][1]);
        const float16 curve2 = v_n[0] * broadcast4to16f(&v[0][2]) + v_n[1] * broadcast4to16f(&v[1][2]) + v_n[2] * broadcast4to16f(&v[2][2]) + v_n[3] * broadcast4to16f(&v[3][2]);
        const float16 curve3 = v_n[0] * broadcast4to16f(&v[0][3]) + v_n[1] * broadcast4to16f(&v[1][3]) + v_n[2] * broadcast4to16f(&v[2][3]) + v_n[3] * broadcast4to16f(&v[3][3]);
        
        const Vec4f16 u_n = CubicBSplineCurve::eval(uu); 
        
        return (u_n[0] * curve0 + u_n[1] * curve1 + u_n[2] * curve2 + u_n[3] * curve3) * float16(1.0f/36.0f);
      }
      
#endif
      
      friend __forceinline std::ostream& operator<<(std::ostream& o, const BSplinePatchT& p)
      {
        for (size_t y=0; y<4; y++)
          for (size_t x=0; x<4; x++)
            o << "[" << y << "][" << x << "] " << p.v[y][x] << std::endl;
        return o;
      } 
    };
  
  typedef BSplinePatchT<Vec3fa,Vec3fa_t> BSplinePatch3fa;
}
