#pragma once
#include <DirectXMath.h>
#include "macros.h"

namespace XRwy
{
    using DirectX::XMFLOAT3;
    using DirectX::XMFLOAT4X4;

    struct HairGeometry
    {
        size_t              nParticle;
        size_t              nStrand;
        size_t              particlePerStrand;

        XMFLOAT3*           position = nullptr;
        XMFLOAT3*           direction = nullptr;
        float               rigidTrans[16];

        ~HairGeometry()
        {
            SAFE_DELETE_ARRAY(position);
            SAFE_DELETE_ARRAY(direction);
        }
    };

    struct HairColorsPerStrand
    {
        size_t      nStrand;
        XMFLOAT3*   color = nullptr;

        ~HairColorsPerStrand()
        {
            SAFE_DELETE_ARRAY(color);
        }
    };

	struct DynamicHair
	{

	};
}