// This code is based on the implementation from Microsoft Corporation.
// It has been adapted to triangular mesh.
// The mesh structure used here is Winged-Edge.
// Written by P.Wu, email: pxiangwu@gmail.com.

#pragma once

#include "DefaultInclude.h"

#include <vector>
#include <list>
#include <algorithm>
#include <functional>

using namespace std;

namespace qemesh
{
	class BaseVertex;

	class TriEdge 
	{
	public:

		BaseVertex*		vert[4];
		TriEdge* 	next[4];
		unsigned char	flags[4];
		unsigned int Idx;
		unsigned int newIdx;
		float	sharpness;
		bool	isTriangleHead;
		bool	isRightSuccessor;

		TriEdge ()
		{
			next[0] = next[1] = next[2] = next[3] = this;
			vert[0] = vert[1] = vert[2] = vert[3] = 0;
			flags[0] = 0;
			flags[1] = 3;
			flags[2] = 2;
			flags[3] = 1;
			sharpness = 0.0f;
			isTriangleHead = false;
		}

		virtual ~TriEdge() {}

	};//class TriEdge


	struct Edge
	{
	private:
		void rotOp()//rotation op
		{
			r = (unsigned int)((r + ((f==1) ? 3 : 1)) % 4);
		}

		void flipOp()//flip op
		{
			f ^= 1;
		}

		void symOp()//symmetric op
		{
			r = (unsigned int)((r + 2) % 4);
		}

		void rotInvOp()
		{
			r = (unsigned int)((r + ((f==1) ? 5 : 3)) % 4);
		}

		void oNextOp()
		{
			int tmp;
			if (f != 0) 
			{
				tmp = (r+1)&3;
				f = (unsigned int)(((e->flags[tmp]&4) == 4)?1:0);
				r = (unsigned int)(e->flags[tmp]&3);
				e = e->next[tmp];
				rotOp();
				flipOp();
			} 
			else 
			{
				tmp = r;
				f = (unsigned int)(((e->flags[tmp]&4) == 4)?1:0);
				r = (unsigned int)(e->flags[tmp]&3);
				e = e->next[tmp];
			}
		}											

		void lNextOp()//Get left-side edge
		{
			rotInvOp(); 
			oNextOp();
			rotOp();
		}

		void rNextOp()//Get right-side edge
		{
			rotOp();
			oNextOp();
			rotInvOp();
		}

		void dNextOp()
		{
			symOp();
			oNextOp();
			symOp();
		}

		void oPrevOp()
		{
			rotOp();
			oNextOp();
			rotOp();
		}

		void lPrevOp()
		{
			oNextOp(); 
			symOp();
		}

		void rPrevOp()
		{
			symOp();
			oNextOp();
		}

		void dPrevOp()
		{
			rotInvOp();
			oNextOp();
			rotInvOp();
		}

	public:
		TriEdge* e;
		unsigned int	r : 2; // triangle index
		unsigned int	f : 1; // 'rot', in {0,1,2}

		Edge(TriEdge* e, unsigned int r, unsigned int f)
		{	
			this->e = e;
			this->r = r;
			this->f = f;
		}

		Edge(Edge* e) 
		{
			this->e = e->e;
			this->r = e->r;
			this->f = e->f;
		}

		Edge() 
		{
			this->e = (TriEdge *)0;
			this->r = 0;
			this->f = 0;
		}

		bool Null()
		{
			return (e == 0);
		}

		Edge Rot()
		{
			Edge e = Edge(this);
			e.rotOp();
			return e;
		}

		Edge Flip()
		{
			Edge e = Edge(this);
			e.flipOp();
			return e;
		}	

		Edge Sym()
		{
			Edge e = Edge(this);
			e.symOp();
			return e;
		}

		Edge RotInv()
		{
			Edge e = Edge(this);
			e.rotInvOp();
			return e;
		}

		Edge ONext()
		{
			Edge e = Edge(this);
			e.oNextOp();
			return e;
		}

		Edge LNext()
		{
			Edge e = Edge(this);
			e.lNextOp();
			return e;
		}

		Edge RNext()
		{
			Edge e = Edge(this);
			e.rNextOp();
			return e;
		}

		Edge DNext()
		{
			Edge e = Edge(this);
			e.dNextOp();
			return e;
		}

		Edge OPrev()
		{
			Edge e = Edge(this);
			e.oPrevOp();
			return e;
		}

		Edge LPrev()
		{
			Edge e = Edge(this);
			e.lPrevOp();
			return e;
		}

		Edge RPrev()
		{
			Edge e = Edge(this);
			e.rPrevOp();
			return e;
		}

		Edge DPrev()
		{
			Edge e = Edge(this);
			e.dPrevOp();
			return e;
		}

		BaseVertex* Org()	//	Origin point of an edge
		{
			return e->vert[r];
		}

		BaseVertex* Dest()	//	Destination point of an edge
		{
			return e->vert[(r+2)&3];	//	Since it is a triangle£¬r+2 is appropriate
		}

		BaseVertex* Left()
		{
			return e->vert[((f==1)?r+1:r+3)&3];
		}

		BaseVertex* Right()
		{
			return e->vert[((f==1)?r+3:r+1)&3];
		}

		bool operator==(Edge e0) const { return (e == e0.e) && (r == e0.r) && (f == e0.f);	}

		bool operator!=(Edge e0) const { return (e != e0.e) || (r != e0.r) || (f != e0.f);	}

		void setONext(Edge& b) 
		{
			if (f != 0) 
			{
				unsigned int tmpf = (unsigned int)(b.f^1);
				unsigned int tmpr = (unsigned int)((b.r + ((tmpf==1) ? 1 : 3))%4);
				e->next[(r+1)&3] = b.e;
				e->flags[(r+1)&3] = (unsigned char)(tmpr + ((tmpf==1)?4:0));
			} 
			else 
			{
				e->next[r] = b.e;
				e->flags[r] = (unsigned char)(b.r + ((b.f==1)?4:0));
			}
		}

		void splice(Edge& a, Edge& b) 
		{
			if (a == b) return;

			Edge 	tmpa = a.ONext();
			Edge 	tmpb = b.ONext();
			Edge 	alpha = tmpa.Rot();
			Edge 	beta = tmpb.Rot();
			Edge 	tmpalpha = alpha.ONext();
			Edge 	tmpbeta = beta.ONext();

			a.setONext(tmpb);
			b.setONext(tmpa);
			alpha.setONext(tmpbeta);
			beta.setONext(tmpalpha);
		}
	};	//	struct edge


	class BaseVertex
	{
	public:
		Edge	rep;
		unsigned int Idx;
		short	n;
		unsigned int newIdx;

		BaseVertex()
		{
			rep = Edge();
		}

		short getN()
		{
			if (rep.e == (TriEdge*)0) return 0;
			if (n == 0){
				Edge e = rep;
				bool boundary = false;
				do{
					if (e.Right() == 0) boundary = true;
					//if (e.e->sharpness == -1.0f) boundary = true;	//we treat infinite sharpness as a boundary

					n++;
					e = e.ONext();
				} while (e != rep);
				if (boundary) n *= -1;
			}
			return n;
		}

		bool isExtraordinary() {
			if (n == 0) getN();

			if (n == -4 || n == 6)	return false;	// some revisions
			else return true;
		}

	};	//	class BaseVertex


	template<class T>
	class Vertex : public BaseVertex
	{
	public:
		T p;
		bool isTagged;
		bool wasPreviousTagged;
		UINT vertexSubdivisions;	// record the level of subdivisions
		bool isEdgePoint;

		Vertex()
		{
			p = T();
			n = 0;;
			isTagged = false;
			wasPreviousTagged = true;
			vertexSubdivisions = 0;
			isEdgePoint = false;
		}

		Vertex(T q)
		{
			p = q;
			n = 0;
			isTagged = false;
			wasPreviousTagged = true;
			vertexSubdivisions = 0;
			isEdgePoint = false;
		}
	};	//	class Vertex


	class Face : public BaseVertex
	{
	public:
		Face(short numPoints)
		{
			n = numPoints;
			m_bIsMarkedForSubdivision = false;
			m_Father = NULL;
			m_Rotations = 0;
			m_isPartialFace = false;
		}

		bool IsRegular() {	//	this code needs to update, since it does not take into consideration the sharpness of edge and vertex
			Edge e = rep;

			do {
				BaseVertex* v0 = e.e->vert[0];
				BaseVertex* v1 = e.e->vert[2];

				if (v0->isExtraordinary() || v1->isExtraordinary())	{
					return false;
				}
				e = e.ONext();
			} while (e != rep);

			return true;
		}

		UINT GetNumBoundaryVertices()
		{
			UINT numBoundaries = 0;
			Edge e = rep;
			for (UINT i = 0; i < 3; i++, e = e.ONext())	{	//	triangular mesh : 3
				if (e.Right()->getN() < 0)	numBoundaries++;
			}

			return numBoundaries;
		}

		bool m_bIsMarkedForSubdivision;
		bool m_isPartialFace;
		Face* m_Father;
		UINT m_Rotations;
		std::list<float2>	m_TexCoords;
	};


	struct MeshError : public std::exception
	{
		MeshError(const char* s) : exception(s) {}
	};


	template<class T>
	class Mesh
	{
	public:
		unsigned int numVertices;
		unsigned int numFaces;
		unsigned int numEdges;

		vector<Vertex<T>*> vlist;
		vector<Face*> flist;
		vector<TriEdge*> elist;
		bool m_bIsTriMesh;
		bool g_ContainsTexCoords;

		Mesh() {
			numVertices = 0;
			numFaces = 0;
			numEdges = 0;
			m_bIsTriMesh = true;
			g_ContainsTexCoords = false;
		}

		virtual ~Mesh() {}

		bool IsQuadMesh() {
			return m_bIsTriMesh;
		}

		bool HasFourBoundaryFaces() {
			for (UINT i = 0; i < flist.size(); i++) {
				if (flist[i]->GetNumBoundaryVertices() == 3) return true;
			}
			return false;
		}

		Vertex<T>* addVertex(T point)
		{
			Vertex<T>* v = new Vertex<T>(point);
			v->Idx = (int)numVertices++;
			vlist.push_back(v);
			return v;
		}

		Vertex<T>* addVertex()
		{
			Vertex<T>* v = new Vertex<T>();
			v->Idx = (int)numVertices++;
			vlist.push_back(v);
			return v;
		}

		void setOrg(Edge e, BaseVertex* vert)
		{
			e.e->vert[e.r] = vert;
			vert->rep = e;
		}

		Edge MakeEdge(Vertex<T>* a, Vertex<T>* b)
		{
			TriEdge* qe = new TriEdge();
			Edge e = Edge(qe, 0, 0);
			setOrg(e, a);
			setOrg(e.Sym(), b);
			qe->Idx = (int)numEdges++;
			elist.push_back(qe);
			return e;
		}

		void splice(Edge a, Edge b) 
		{
			if (a == b) return;

			Edge tmpa = a.ONext();
			Edge tmpb = b.ONext();
			Edge alpha = tmpa.Rot();
			Edge beta = tmpb.Rot();
			Edge tmpalpha = alpha.ONext();
			Edge tmpbeta = beta.ONext();

			a.setONext(tmpb);
			b.setONext(tmpa);
			alpha.setONext(tmpbeta);
			beta.setONext(tmpalpha);
		}

		Face* addFace(Vertex<T>* vert[], int numPoints)
		{
			Face* f = new Face((short)numPoints);

			int i, j;
			Edge* g = new Edge[numPoints];

			Edge a, b, c;

			// Search for existing edges.  		
			for (i = 0, j = numPoints; j != 0; i = (i + 1) % numPoints, j--)
			{
				Edge e = vert[i]->rep;
				if (!e.Null())
				{
					a = Edge();
					b = Edge();
					c = Edge();
					Edge e0 = e;
					do
					{
						Vertex<T>* w = (Vertex<T>*)e.Dest();
						if (w == vert[(i + numPoints - 1) % numPoints])
						{
							if (e.Right() != 0) {
								SAFE_DELETE(f);
								SAFE_DELETE_ARRAY(g);
								throw MeshError("invalid edge 1");
							}
							b = e;
						}
						else if (w == vert[(i + 1) % numPoints])
						{
							if (e.Left() != 0) {
								SAFE_DELETE(f);
								SAFE_DELETE_ARRAY(g);
								throw MeshError("invalid edge 2");
							}
							a = e;
						}
						else if (e.Right() == 0) c = e;
						e = e.ONext();
					} while (e != e0);

					if (!a.Null())
					{
						if (!b.Null())
						{
							if (a.ONext() != b) //wedge
							{
								e = b.ONext();
								do
								{
									if (e.Right() == 0)
										break;
									e = e.ONext();
								} while (e != a);

								if (e == a) {
									SAFE_DELETE(f);
									SAFE_DELETE_ARRAY(g);
									throw MeshError("non-manifold vertex 1");
								}

								// this is the new version 4-11-02
								Edge h = b.OPrev();
								splice(h, a);
								splice(h, e.OPrev());
							}
							g[i] = b;
						}
						else g[i] = a.ONext();
					}
					else if (!b.Null()) g[i] = b;
					else if (!c.Null()) g[i] = c;
					else {
						SAFE_DELETE(f);
						SAFE_DELETE_ARRAY(g);
						throw MeshError("non-manifold vertex 2");
					}
				}
				else g[i] = Edge();
			}

			//phase two
			for (i = 0, j = numPoints; j != 0; i = (i + 1) % numPoints, j--)
			{
				int im = (i + numPoints - 1) % numPoints;
				if (g[i].Null() || g[i].Dest() != vert[im])
				{
					Edge e = MakeEdge(vert[i], vert[im]);

					if (!g[im].Null())
						splice(e.Sym(), g[im].OPrev());
					else
						g[im] = e.Sym();

					if (!g[i].Null())
						splice(e, g[i].OPrev());

					g[i] = e;
				}
				setOrg(g[i].Rot(), f);
			}

			f->rep = g[1].Rot();

			SAFE_DELETE_ARRAY ( g );
			f->Idx = (unsigned int)numFaces++;
			flist.push_back(f);
			return f;
		}


		Face* addTriangle(Vertex<T>* v0, Vertex<T>* v1, Vertex<T>* v2)
		{
			Vertex<T>* tri[3];
			tri[0] = v0;
			tri[1] = v1;
			tri[2] = v2;
			return addFace(tri, 3);
		}

		Face* addQuad(Vertex<T>* v0, Vertex<T>* v1, Vertex<T>* v2, Vertex<T>* v3)
		{
			Vertex<T>* quad[4];
			quad[0] = v0;
			quad[1] = v1;
			quad[2] = v2;
			quad[3] = v3;
			return addFace(quad, 4);
		}

	};
}