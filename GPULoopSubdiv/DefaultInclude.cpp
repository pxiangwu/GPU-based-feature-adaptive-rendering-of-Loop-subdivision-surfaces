// Written by P.Wu, email: pxiangwu@gmail.com.

#include "DefaultInclude.h"

#ifndef WIN_METRO_APP

#else

//Note that these functions are not implemented yet; they must be implemented when camera and UI on Win8 will be used

void MyMatrixTranspose( matrix4x4 *matrixOut, const matrix4x4 *matrixIn )
{
	*matrixOut = XMMatrixTranspose(*matrixIn);
}

void MyVec3TransformCoord( float3* vecOut, const float3* vecIn, const matrix4x4 *matrixIn )
{

}

void MyVec4Transform( float4* vecOut, const float4* vecIn, const matrix4x4 *matrixIn )
{

}

#endif
