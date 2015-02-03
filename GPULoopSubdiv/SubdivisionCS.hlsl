// Written by P.Wu, email: pxiangwu@gmail.com.

cbuffer cbCS : register( b1 )
{
	uint g_NumFaces; 
	uint g_NumEdges; 
	uint g_NumVertices; 
	uint DEST_OFFSET_1;
	uint DEST_OFFSET_2;
	uint DEST_OFFSET_3;
	uint SRC_OFFSET;
	uint pad1;
};

#define M_PI 3.14159265

RWBuffer<float> VB			: register( u0 );
RWBuffer<float> VB_DEST		: register( u1 );

Buffer<uint> E0_IT	: register( t0 );
Buffer<int>  V0_ITa			: register( t1 );
Buffer<uint> V0_IT	: register( t2 );

[numthreads(64, 4, 1)]
void EdgePointCS(		uint3 blockIdx : SV_GroupID, 
						uint3 DTid : SV_DispatchThreadID, 
						uint3 threadIdx : SV_GroupThreadID,
						uint GI : SV_GroupIndex )
{
	uint EdgeIdx = 64 * blockIdx.x + threadIdx.x;
	if (EdgeIdx < g_NumEdges){
		float p1, p2, p3, p4;
		p1 = VB[SRC_OFFSET + 4*E0_IT[4*EdgeIdx+0] + threadIdx.y];
		p2 = VB[SRC_OFFSET + 4*E0_IT[4*EdgeIdx+1] + threadIdx.y];
		p3 = VB[SRC_OFFSET + 4*E0_IT[4*EdgeIdx+2] + threadIdx.y];
		p4 = VB[SRC_OFFSET + 4*E0_IT[4*EdgeIdx+3] + threadIdx.y];

		float q;
		q = 0.375 * (p1 + p2) + 0.125 * (p3 + p4);

		VB[DEST_OFFSET_2 + 4*EdgeIdx + threadIdx.y] = q;
	}
}


[numthreads(8, 4, 1)]
void VertexPointCS(	uint3 blockIdx : SV_GroupID, 
						uint3 DTid : SV_DispatchThreadID, 
						uint3 threadIdx : SV_GroupThreadID,
						uint GI : SV_GroupIndex )
{
	int VertexIdx = 8 * blockIdx.x + threadIdx.x;
	if (VertexIdx < g_NumVertices){
		uint h = V0_ITa[3*VertexIdx];
		uint n = V0_ITa[3*VertexIdx+1];
		int vertexSourceIdx = V0_ITa[3*VertexIdx+2];
		float fn = (float)n;
		float q = 0.0f;	

		float v = VB[SRC_OFFSET + 4*vertexSourceIdx + threadIdx.y];

		for (uint j = 0; j < n; j++) {
			q += VB[SRC_OFFSET + 4*V0_IT[h+j] + threadIdx.y];
		}

		float result;
		float alpha;
		alpha = (0.625 - (3+2*cos(2*M_PI/fn)) * (3+2*cos(2*M_PI/fn))/64)/fn;

		result = (1-fn*alpha)*v + alpha*q;
		VB[DEST_OFFSET_1 + 4*VertexIdx + threadIdx.y] = result;
	}
}




