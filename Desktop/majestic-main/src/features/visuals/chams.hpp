#pragma once

#include "core/imports.h"
#include "config/interface.hpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <cstring>
#pragma comment(lib, "d3dcompiler.lib")

// Engine chams on the D3D11 draw path: the world render stays vanilla, only
// ped draws are re-issued. Peds are identified by their render state
// signature (public FiveM reversal: vertex stride 48/72 with the character
// constant buffers at VS slot 4 = 12240 bytes and PS slot 13 = 304 bytes).
// Each matched draw runs twice - solid color with depth off (visible through
// walls), then the original state (normal ped where not occluded).
namespace chams {
    using draw_fn = HRESULT(__stdcall*)(ID3D11DeviceContext*, UINT, UINT, INT);

    inline bool s_enabled = false;
    inline float s_color[4] = { 1.f, 0.f, 0.f, 1.f };

    inline ID3D11PixelShader* s_ps = nullptr;
    inline ID3D11BlendState* s_blend = nullptr;
    inline ID3D11DepthStencilState* s_ds = nullptr;
    inline bool s_failed = false;

    // refreshed once per frame from game_render; the draw hook only touches
    // these cached values, never the config map
    inline void tick() {
        s_enabled = config::get("visual", "chams_enable", 0) != 0;
        if (s_enabled) {
            s_color[0] = config::get("visual", "chams_r", 1.f);
            s_color[1] = config::get("visual", "chams_g", 0.f);
            s_color[2] = config::get("visual", "chams_b", 0.f);
            s_color[3] = config::get("visual", "chams_a", 1.f);
        }
    }

    inline bool is_ped_draw(ID3D11DeviceContext* ctx) {
        ID3D11Buffer* buf = nullptr;
        UINT stride = 0, off = 0;
        ctx->IAGetVertexBuffers(0, 1, &buf, &stride, &off);
        if (buf) buf->Release();
        if (stride != 48 && stride != 72) return false;

        UINT width = 0;
        ctx->VSGetConstantBuffers(4, 1, &buf);
        if (buf) { D3D11_BUFFER_DESC d{}; buf->GetDesc(&d); width = d.ByteWidth; buf->Release(); }
        if (width != 12240) return false;

        ctx->PSGetConstantBuffers(13, 1, &buf);
        if (buf) { D3D11_BUFFER_DESC d{}; buf->GetDesc(&d); width = d.ByteWidth; buf->Release(); }
        return width == 304;
    }

    inline bool prepare(ID3D11DeviceContext* ctx) {
        if (s_ps) return true;
        if (s_failed) return false;

        ID3D11Device* dev = nullptr;
        ctx->GetDevice(&dev);
        if (!dev) { s_failed = true; return false; }

        // the solid color comes from the blend factor, so the shader is
        // compiled once and the color picker never triggers a recompile
        const char* hlsl = "float4 main() : SV_Target { return 1; }";
        ID3DBlob* blob = nullptr;
        ID3DBlob* err = nullptr;
        bool ok = SUCCEEDED(D3DCompile(hlsl, strlen(hlsl), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &blob, &err));
        if (err) err->Release();
        if (ok) ok = SUCCEEDED(dev->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &s_ps));
        if (blob) blob->Release();

        D3D11_BLEND_DESC bd{};
        bd.RenderTarget[0].BlendEnable = TRUE;
        bd.RenderTarget[0].SrcBlend = D3D11_BLEND_BLEND_FACTOR;
        bd.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
        bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
        bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
        bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (ok) ok = SUCCEEDED(dev->CreateBlendState(&bd, &s_blend));

        D3D11_DEPTH_STENCIL_DESC dd{};
        dd.DepthEnable = FALSE;
        dd.StencilEnable = FALSE;
        if (ok) ok = SUCCEEDED(dev->CreateDepthStencilState(&dd, &s_ds));

        dev->Release();
        if (!ok) {
            s_failed = true;
            if (s_ps) { s_ps->Release(); s_ps = nullptr; }
            if (s_blend) { s_blend->Release(); s_blend = nullptr; }
            if (s_ds) { s_ds->Release(); s_ds = nullptr; }
            return false;
        }
        return true;
    }

    inline void draw_chams(ID3D11DeviceContext* ctx, draw_fn orig, UINT count, UINT start, INT base) {
        ID3D11PixelShader* orig_ps = nullptr;
        ID3D11ClassInstance* inst[16] = {};
        UINT inst_n = 16;
        ctx->PSGetShader(&orig_ps, inst, &inst_n);
        ID3D11BlendState* bs = nullptr;
        FLOAT bf[4] = {};
        UINT mask = 0;
        ctx->OMGetBlendState(&bs, bf, &mask);
        ID3D11DepthStencilState* dss = nullptr;
        UINT dref = 0;
        ctx->OMGetDepthStencilState(&dss, &dref);

        ctx->OMSetDepthStencilState(s_ds, 0);
        ctx->OMSetBlendState(s_blend, s_color, 0xFFFFFFFF);
        ctx->PSSetShader(s_ps, nullptr, 0);
        orig(ctx, count, start, base);

        ctx->OMSetBlendState(bs, bf, mask);
        ctx->OMSetDepthStencilState(dss, dref);
        ctx->PSSetShader(orig_ps, inst, inst_n);
        orig(ctx, count, start, base);

        for (UINT i = 0; i < inst_n; ++i) if (inst[i]) inst[i]->Release();
        if (bs) bs->Release();
        if (dss) dss->Release();
        if (orig_ps) orig_ps->Release();
    }
}