#include "precompiled.h"
#include "wrMeshRenderer.h"
#include <SDKmisc.h>
#include <Effects.h>
#include <DXUTcamera.h>
#include <GeometricPrimitive.h>
#include "CFBXRendererDX11.h"
#include <wrl\client.h>

using namespace DirectX;
std::unique_ptr<GeometricPrimitive> shape;
FBX_LOADER::CFBXRenderDX11*    g_pFbxDX11;

ID3DUserDefinedAnnotation*            g_pUserAnotation = nullptr;


wrMeshRenderer::wrMeshRenderer()
{
    DirectX::XMStoreFloat4x4(&translation, DirectX::XMMatrixIdentity());
}


wrMeshRenderer::~wrMeshRenderer()
{
}


bool wrMeshRenderer::init()
{
    HRESULT hr;

    pd3dDevice = DXUTGetD3D11Device();
    pd3dImmediateContext = DXUTGetD3D11DeviceContext();

    pEffect.reset(new BasicEffect(pd3dDevice));
    shape = GeometricPrimitive::CreateTeapot(pd3dImmediateContext, 1, 8u, false);

    g_pFbxDX11 = new FBX_LOADER::CFBXRenderDX11;
    V_RETURN(g_pFbxDX11->LoadFBX("../../models/headdemo.fbx", pd3dDevice, pd3dImmediateContext));

    pEffect->EnableDefaultLighting();

    const void* pVSBufferPtr = nullptr;
    size_t nVSBufferSz = 0;
    ID3D11InputLayout * pInputLayout;

    pEffect->GetVertexShaderBytecode(&pVSBufferPtr, &nVSBufferSz);

    V_RETURN(g_pFbxDX11->CreateInputLayout(pd3dDevice, pVSBufferPtr, nVSBufferSz,
        const_cast<D3D11_INPUT_ELEMENT_DESC*>(VertexPositionNormalTexture::InputElements), VertexPositionNormalTexture::InputElementCount));

    //pd3dImmediateContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&g_pUserAnotation);
    return true;
}

void wrMeshRenderer::release()
{
    g_pFbxDX11->Release();
    SAFE_DELETE(g_pFbxDX11);
    SAFE_RELEASE(g_pUserAnotation);
    shape.reset();
    pEffect.reset();
}

void wrMeshRenderer::render(double fTime, float fTimeElapsed)
{
    pEffect->SetWorld(enableControl ? pCamera->GetWorldMatrix()*DirectX::XMLoadFloat4x4(&translation) : DirectX::XMLoadFloat4x4(&translation));
    pEffect->SetView(pCamera->GetViewMatrix());
    pEffect->SetProjection(pCamera->GetProjMatrix());

    pEffect->Apply(pd3dImmediateContext);

    size_t nodeCount = g_pFbxDX11->GetNodeCount();

    for (int j = 0; j < nodeCount; j++)
    {
        FBX_LOADER::MATERIAL_DATA material = g_pFbxDX11->GetNodeMaterial(j);

        if (material.pMaterialCb)
            pd3dImmediateContext->UpdateSubresource(material.pMaterialCb, 0, NULL, &material.materialConstantData, 0, 0);

        pd3dImmediateContext->PSSetShaderResources(0, 1, &material.pSRV);
        pd3dImmediateContext->PSSetConstantBuffers(0, 1, &material.pMaterialCb);

        g_pFbxDX11->RenderNode(pd3dImmediateContext, j);
    }

    //pEffect->SetTextureEnabled(false);
    //Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout;
    //shape->CreateInputLayout(pEffect.get(), inputLayout.GetAddressOf());
    //shape->Draw(pEffect.get(), inputLayout.Get());

    //shape->Draw(pCamera->GetWorldMatrix(), pCamera->GetViewMatrix(), pCamera->GetProjMatrix(), Colors::CornflowerBlue);

    //pd3dImmediateContext->IASetInputLayout(...);
    //pd3dImmediateContext->IASetVertexBuffers(...);
    //pd3dImmediateContext->IASetIndexBuffer(...);
    //pd3dImmediateContext->IASetPrimitiveTopology(...);
    //pd3dImmediateContext->PSSetSamplers(...);

    //pd3dImmediateContext->DrawIndexed(...);
}

void wrMeshRenderer::setTransformation(const float* trans4x4)
{
    using namespace DirectX;
    auto target0 = XMFLOAT4X4(trans4x4);
    auto trans0 = XMFLOAT3(0.0f, -0.643f, 0.282f);
    auto scale0 = XMFLOAT3(5.346f, 5.346f, 5.346f);
    XMStoreFloat4x4(&translation, XMMatrixAffineTransformation(XMLoadFloat3(&scale0), XMVectorZero(), XMVectorZero(), XMLoadFloat3(&trans0))*XMMatrixTranspose(XMLoadFloat4x4(&target0)));
}

void wrMeshRenderer::setOffset(const float* vec)
{
    using namespace DirectX;
    XMStoreFloat4x4(&translation, XMLoadFloat4x4(&translation) * XMMatrixTranslation(vec[0], vec[1], vec[2]));
}

