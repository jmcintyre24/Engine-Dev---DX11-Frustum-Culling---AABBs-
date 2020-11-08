#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <dxgi1_2.h>
#include <d3d11_2.h>
#include <DirectXMath.h>
#include <time.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "DXGI.lib")

#include "renderer.h"
#include "debug_renderer.h"
#include "view.h"
#include "pools.h"
#include "XTime.h"
#include "view.h"
#include "blob.h"
#include "../Renderer/shaders/mvp.hlsli"

#include "frustum_culling.h"

// NOTE: This header file must *ONLY* be included by renderer.cpp

#define EMITTERNUM 4
#define PARTICLESAMT 2
#define MOVESPEED 1.5f

#define numOfAABB 5

namespace
{
	template<typename T>
	void safe_release(T* t)
	{ 
		if (t)
			t->Release();
	}

	struct particle {
		end::float3 pos;
		end::float3 prev_pos;
		end::float4 color;
	};

	// Sorted Pool
	end::sorted_pool_t<particle, 254> sp;
	struct emitter {
		end::float3 spawn_pos;
		end::float4 spawn_color;
		end::sorted_pool_t<int16_t, 256> indices;
	};

	// Shared Pool
	end::pool_t<particle, 1024> sharedpool;
	std::vector<emitter> emitters;

	float delay = 0;
	XTime xtime;

	matrix pWorld = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, -0.90f, 0, 1,
	};
	matrix lWorld = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		5.0f, 1.0f, 2.5f, 1,
	};
	matrix tWorld = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		-5.0f, 1.0f, -2.5f, 1,
	};
	end::view_t pView;
	end::frustum_t frus;
	end::aabb_t arrayAABB[numOfAABB] = {
		{{-5.0f, -0.5f, -5.0f}, {-2.5f, 0.5f, -2.5f}},
		{{ 5.0f, -0.75f, -2.5f}, { 7.0f, 1.0f, 0.0f }},
		{{ -1.0f, -0.25f, 5.0f}, { 1.0f, 0.25f, 7.0f }},
		{{ -7.5f, 0.5f, 5.0f}, { -5.0f, 1.25f, 7.0f }},
		{{ -9.5f, -0.25f, -1.0f}, { -5.0f, 0.0f, 1.0f }},
	};

	end::float4 AABBColors[numOfAABB] = {
		{1.0f, 1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f, 1.0f},
		{1.0f, 1.0f, 1.0f, 1.0f},
	};
}


namespace end
{
	using namespace DirectX;

	struct renderer_t::impl_t
	{
		// platform/api specific members, functions, etc.
		// Device, swapchain, resource views, states, etc. can be members here
		HWND hwnd;

		ID3D11Device *device = nullptr;
		ID3D11DeviceContext *context = nullptr;
		IDXGISwapChain *swapchain = nullptr;

		ID3D11RenderTargetView*		render_target[VIEW_RENDER_TARGET::COUNT]{};

		ID3D11DepthStencilView*		depthStencilView[VIEW_DEPTH_STENCIL::COUNT]{};

		ID3D11DepthStencilState*	depthStencilState[STATE_DEPTH_STENCIL::COUNT]{};

		ID3D11RasterizerState*		rasterState[STATE_RASTERIZER::COUNT]{};

		ID3D11Buffer*				vertex_buffer[VERTEX_BUFFER::COUNT]{};

		ID3D11Buffer*				index_buffer[INDEX_BUFFER::COUNT]{};
		
		ID3D11InputLayout*			input_layout[INPUT_LAYOUT::COUNT]{};

		ID3D11VertexShader*			vertex_shader[VERTEX_SHADER::COUNT]{};

		ID3D11PixelShader*			pixel_shader[PIXEL_SHADER::COUNT]{};

		ID3D11Buffer*				constant_buffer[CONSTANT_BUFFER::COUNT]{};

		D3D11_VIEWPORT				view_port[VIEWPORT::COUNT]{};

		/* Add more as needed...
		ID3D11SamplerState*			sampler_state[STATE_SAMPLER::COUNT]{};

		ID3D11BlendState*			blend_state[STATE_BLEND::COUNT]{};
		*/

		// Constructor for renderer implementation
		// 
		impl_t(native_handle_type window_handle, view_t& default_view)
		{
			hwnd = (HWND)window_handle;

			create_device_and_swapchain();

			create_main_render_target();

			setup_depth_stencil();

			setup_rasterizer();

			create_shaders();

			create_constant_buffers();

			create_vertex_buffers();

			float aspect = view_port[VIEWPORT::DEFAULT].Width / view_port[VIEWPORT::DEFAULT].Height;

			XMVECTOR eyepos = XMVectorSet(0.0f, 15.0f, -15.0f, 1.0f);
			XMVECTOR focus = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
			XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

			default_view.view_mat = (float4x4_a&)XMMatrixInverse(nullptr, XMMatrixLookAtLH(eyepos, focus, up));
			default_view.proj_mat = (float4x4_a&)XMMatrixPerspectiveFovLH(3.1415926f / 4.0f, aspect, 0.01f, 100.0f);

			eyepos = XMVectorSet(0.0f, 0.0f, -0.1f, 1.0f);
			focus = XMVectorSet(0.0f, 0.0f, -0.0f, 1.0f);

			pView.view_mat = (float4x4_a&)XMMatrixInverse(nullptr, XMMatrixLookAtLH(eyepos, focus, up));
			pView.proj_mat = (float4x4_a&)XMMatrixPerspectiveFovLH(3.1415926f / 4.0f, aspect, 1.0f, 10.0f);

			/* initialize random seed: */
			srand((unsigned int)time(NULL));

			// Create emitters
			for (int i = 0; i < EMITTERNUM; i++)
			{
				emitter mit;

				mit.spawn_pos = { 6.0f, -1.0f, 6.0f };

				mit.spawn_color = { 0.75f, 0.5f, 1.0f, 1.0f };

				if (i == 0)
				{
					mit.spawn_pos = { -6.0f, -1.0f, -6.0f };
					mit.spawn_color = { 1.0f, 1.0f, 0.0f, 1.0f };
				}
				else if (i == 1)
				{
					mit.spawn_pos = { -6.0f, -1.0f, 6.0f };
					mit.spawn_color = { 1.0f, 0.0f, 1.0f, 1.0f };
				}
				else if (i == 2)
				{
					mit.spawn_pos = { 6.0f, -1.0f, -6.0f };
					mit.spawn_color = { 0.0f, 1.0f, 1.0f, 1.0f };
				}

				emitters.push_back(mit);
			}

			// Reset the timer;
			xtime.Restart();
		}

		void update(view_t& view) // Passed in view so I can update it accordingly to how the player looks around.
		{
			// Signal that a frame has passed.
			xtime.Signal();

			// Increment the delay by the time passed by frame.
			delay += (float)xtime.Delta();

			// Input
			player_input((XMMATRIX&)view.view_mat);

			// Update the look_at world
			lWorld = look_at(lWorld.r[3], pWorld.r[3], { 0.0f, 1.0f, 0.0f });

			// Update the turn_to world
			tWorld = turn_to(tWorld, pWorld.r[3], 1.0f);

			// Calculate Frustum
			calculate_frustum(frus, pView, pWorld);

			// Check if aabb is in frustum
			for (int i = 0; i < numOfAABB; i++)
			{
				bool didHit = aabb_to_frustum(arrayAABB[i], frus);

				if (didHit)
				{
					AABBColors[i] = { 1.0f, 1.0f, 0.0f, 1.0f };
				}
				else
				{
					AABBColors[i] = { 0.0f, 1.0f, 1.0f, 1.0f };
				}
			}

			// 'Spawn' particles every frame w/ delay
			//spawn_particles();

			// Update every active particle.
			//update_particles();
		}

		void draw_view(view_t& view)
		{
			const float4 black{ 0.0f, 0.0f, 0.0f, 1.0f };

			context->OMSetDepthStencilState(depthStencilState[STATE_DEPTH_STENCIL::DEFAULT], 1);
			context->OMSetRenderTargets(1, &render_target[VIEW_RENDER_TARGET::DEFAULT], depthStencilView[VIEW_DEPTH_STENCIL::DEFAULT]);

			context->ClearRenderTargetView(render_target[VIEW_RENDER_TARGET::DEFAULT], black.data());
			context->ClearDepthStencilView(depthStencilView[VIEW_DEPTH_STENCIL::DEFAULT], D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

			context->RSSetState(rasterState[STATE_RASTERIZER::DEFAULT]);
			context->RSSetViewports(1, &view_port[VIEWPORT::DEFAULT]);

			MVP_t mvp;

			// ** Player World ** //

			mvp.modeling = XMMatrixTranspose(pWorld);
			mvp.projection = XMMatrixTranspose((XMMATRIX&)view.proj_mat);
			mvp.view = XMMatrixTranspose(XMMatrixInverse(nullptr, (XMMATRIX&)view.view_mat));
			context->UpdateSubresource(constant_buffer[CONSTANT_BUFFER::MVP], 0, NULL, &mvp, 0, 0);

			// Since there's no logic here, specifing these lines out are fine.
			debug_renderer::add_line({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
			debug_renderer::add_line({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f });
			debug_renderer::add_line({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f });

			render_debug_lines();

			// Create the grid.
			create_grid_perframe();
			
			// And render the AABB with their respective color
			for (int i = 0; i < numOfAABB; i++)
			{
				render_aabb(arrayAABB[i], AABBColors[i]);
			}

			mvp.modeling = XMMatrixTranspose(XMMatrixIdentity());

			context->UpdateSubresource(constant_buffer[CONSTANT_BUFFER::MVP], 0, NULL, &mvp, 0, 0);

			render_debug_lines();

			// ** Look-At World ** //
			mvp.modeling = XMMatrixTranspose(lWorld);
			context->UpdateSubresource(constant_buffer[CONSTANT_BUFFER::MVP], 0, NULL, &mvp, 0, 0);

			// Since there's no logic here, specifing these lines out are fine.
			debug_renderer::add_line({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
			debug_renderer::add_line({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f });
			debug_renderer::add_line({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f });

			render_debug_lines();

			// ** Turn-To World ** //
			mvp.modeling = XMMatrixTranspose(tWorld);
			context->UpdateSubresource(constant_buffer[CONSTANT_BUFFER::MVP], 0, NULL, &mvp, 0, 0);

			// Since there's no logic here, specifing these lines out are fine.
			debug_renderer::add_line({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f });
			debug_renderer::add_line({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f });
			debug_renderer::add_line({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f });

			render_debug_lines();

			swapchain->Present(0, 0);
		}

		~impl_t()
		{
			// TODO:
			//Clean-up
			//
			// In general, release objects in reverse order of creation

			for (auto& ptr : vertex_buffer)
				safe_release(ptr);

			for (auto& ptr : constant_buffer)
				safe_release(ptr);

			for (auto& ptr : pixel_shader)
				safe_release(ptr);

			for (auto& ptr : vertex_shader)
				safe_release(ptr);

			for (auto& ptr : input_layout)
				safe_release(ptr);

			for (auto& ptr : index_buffer)
				safe_release(ptr);

			for (auto& ptr : rasterState)
				safe_release(ptr);

			for (auto& ptr : depthStencilState)
				safe_release(ptr);

			for (auto& ptr : depthStencilView)
				safe_release(ptr);

			for (auto& ptr : render_target)
				safe_release(ptr);

			safe_release(context);
			safe_release(swapchain);
			safe_release(device);
		}

		// INITIALIZATION
		void create_device_and_swapchain()
		{
			RECT crect;
			GetClientRect(hwnd, &crect);

			// Setup the viewport
			D3D11_VIEWPORT &vp = view_port[VIEWPORT::DEFAULT];

			vp.Width = (float)crect.right;
			vp.Height = (float)crect.bottom;
			vp.MinDepth = 0.0f;
			vp.MaxDepth = 1.0f;
			vp.TopLeftX = 0;
			vp.TopLeftY = 0;

			// Setup swapchain
			DXGI_SWAP_CHAIN_DESC sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.BufferCount = 1;
			sd.BufferDesc.Width = crect.right;
			sd.BufferDesc.Height = crect.bottom;
			sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			sd.BufferDesc.RefreshRate.Numerator = 60;
			sd.BufferDesc.RefreshRate.Denominator = 1;
			sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			sd.OutputWindow = hwnd;
			sd.SampleDesc.Count = 1;
			sd.SampleDesc.Quality = 0;
			sd.Windowed = TRUE;

			D3D_FEATURE_LEVEL  FeatureLevelsSupported;

			const D3D_FEATURE_LEVEL lvl[] = 
			{
				D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
				D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
				D3D_FEATURE_LEVEL_9_3, D3D_FEATURE_LEVEL_9_2, D3D_FEATURE_LEVEL_9_1
			};

			UINT createDeviceFlags = 0;

			#ifdef _DEBUG
						createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
			#endif

			HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, lvl, _countof(lvl), D3D11_SDK_VERSION, &sd, &swapchain, &device, &FeatureLevelsSupported, &context);

			if (hr == E_INVALIDARG)
			{
				hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, &lvl[1], _countof(lvl) - 1, D3D11_SDK_VERSION, &sd, &swapchain, &device, &FeatureLevelsSupported, &context);
			}

			assert(!FAILED(hr));
		}

		void create_main_render_target()
		{
			ID3D11Texture2D* pBackBuffer;
			// Get a pointer to the back buffer
			HRESULT hr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D),
				(LPVOID*)&pBackBuffer);

			assert(!FAILED(hr));

			// Create a render-target view
			device->CreateRenderTargetView(pBackBuffer, NULL,
				&render_target[VIEW_RENDER_TARGET::DEFAULT]);

			pBackBuffer->Release();
		}

		void setup_depth_stencil()
		{
			/* DEPTH_BUFFER */
			D3D11_TEXTURE2D_DESC depthBufferDesc;
			ID3D11Texture2D *depthStencilBuffer;

			ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

			depthBufferDesc.Width = (UINT)view_port[VIEWPORT::DEFAULT].Width;
			depthBufferDesc.Height = (UINT)view_port[VIEWPORT::DEFAULT].Height;
			depthBufferDesc.MipLevels = 1;
			depthBufferDesc.ArraySize = 1;
			depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depthBufferDesc.SampleDesc.Count = 1;
			depthBufferDesc.SampleDesc.Quality = 0;
			depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
			depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			depthBufferDesc.CPUAccessFlags = 0;
			depthBufferDesc.MiscFlags = 0;

			HRESULT hr = device->CreateTexture2D(&depthBufferDesc, NULL, &depthStencilBuffer);

			assert(!FAILED(hr));

			/* DEPTH_STENCIL */
			D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;

			ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

			depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			depthStencilViewDesc.Texture2D.MipSlice = 0;

			hr = device->CreateDepthStencilView(depthStencilBuffer, &depthStencilViewDesc, &depthStencilView[VIEW_DEPTH_STENCIL::DEFAULT]);
			
			assert(!FAILED(hr));

			depthStencilBuffer->Release();

			/* DEPTH_STENCIL_DESC */
			D3D11_DEPTH_STENCIL_DESC depthStencilDesc;

			ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

			depthStencilDesc.DepthEnable = true;
			depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;

			hr = device->CreateDepthStencilState(&depthStencilDesc, &depthStencilState[STATE_DEPTH_STENCIL::DEFAULT]);

			assert(!FAILED(hr));
		}

		void setup_rasterizer()
		{
			D3D11_RASTERIZER_DESC rasterDesc;

			ZeroMemory(&rasterDesc, sizeof(rasterDesc));

			rasterDesc.AntialiasedLineEnable = true;
			rasterDesc.CullMode = D3D11_CULL_BACK;
			rasterDesc.DepthBias = 0;
			rasterDesc.DepthBiasClamp = 0.0f;
			rasterDesc.DepthClipEnable = false;
			rasterDesc.FillMode = D3D11_FILL_SOLID;
			rasterDesc.FrontCounterClockwise = false;
			rasterDesc.MultisampleEnable = false;
			rasterDesc.ScissorEnable = false;
			rasterDesc.SlopeScaledDepthBias = 0.0f;

			HRESULT hr = device->CreateRasterizerState(&rasterDesc, &rasterState[STATE_RASTERIZER::DEFAULT]);

			assert(!FAILED(hr));
		}

		void create_shaders()
		{
			binary_blob_t vs_blob = load_binary_blob("vs_cube.cso");
			binary_blob_t ps_blob = load_binary_blob("ps_cube.cso");

			HRESULT hr = device->CreateVertexShader(vs_blob.data(), vs_blob.size(), NULL, &vertex_shader[VERTEX_SHADER::BUFFERLESS_CUBE]);

			assert(!FAILED(hr));

			vs_blob = load_binary_blob("vs_line.cso");

			hr = device->CreateVertexShader(vs_blob.data(), vs_blob.size(), NULL, &vertex_shader[VERTEX_SHADER::COLORED_VERTEX]);

			// Create input layout.
			{
				// Define the input layout
				D3D11_INPUT_ELEMENT_DESC layout[] =
				{
					{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
					{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				};
				UINT numElements = ARRAYSIZE(layout);

				// Create the input layout
				hr = device->CreateInputLayout(layout, numElements, vs_blob.data(), vs_blob.size(), &input_layout[INPUT_LAYOUT::COLORED_VERTEX]);
			}

			assert(!FAILED(hr));

			hr = device->CreatePixelShader(ps_blob.data(), ps_blob.size(), NULL, &pixel_shader[PIXEL_SHADER::BUFFERLESS_CUBE]);

			assert(!FAILED(hr));

			ps_blob = load_binary_blob("ps_line.cso");

			hr = device->CreatePixelShader(ps_blob.data(), ps_blob.size(), NULL, &pixel_shader[PIXEL_SHADER::COLORED_VERTEX]);

			assert(!FAILED(hr));
		}

		void create_constant_buffers()
		{
			D3D11_BUFFER_DESC mvp_bd;
			ZeroMemory(&mvp_bd, sizeof(mvp_bd));

			mvp_bd.Usage = D3D11_USAGE_DEFAULT;
			mvp_bd.ByteWidth = sizeof(MVP_t);
			mvp_bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			mvp_bd.CPUAccessFlags = 0;

			HRESULT hr = device->CreateBuffer(&mvp_bd, NULL, &constant_buffer[CONSTANT_BUFFER::MVP]);
		}

		void create_vertex_buffers()
		{
			D3D11_BUFFER_DESC vert_bd;
			// Create Vertex Buffer
			ZeroMemory(&vert_bd, sizeof(vert_bd));

			vert_bd.Usage = D3D11_USAGE_DEFAULT;
			vert_bd.ByteWidth = sizeof(end::colored_vertex) * (UINT)debug_renderer::get_line_vert_capacity();
			vert_bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vert_bd.CPUAccessFlags = 0;
			vert_bd.MiscFlags = 0;

			D3D11_SUBRESOURCE_DATA InitData = {};
			InitData.pSysMem = debug_renderer::get_line_verts();

			HRESULT hr = device->CreateBuffer(&vert_bd, &InitData, &vertex_buffer[VERTEX_BUFFER::COLORED_VERTEX]);

		}

		// MATH
		matrix look_at(XMVECTOR pos, XMVECTOR target, float3 up)
		{
			XMVECTOR z = target - pos;
			z = XMVector3Normalize(z);

			XMVECTOR x = XMVector3Cross({ up.x, up.y, up.z }, z);
			x = XMVector3Normalize(x);

			XMVECTOR y = XMVector3Cross(z, x);
			y = XMVector3Normalize(y);

			
			return {
				XMVectorGetX(x), XMVectorGetY(x), XMVectorGetZ(x), 0.0f,
				XMVectorGetX(y), XMVectorGetY(y), XMVectorGetZ(y), 0.0f,
				XMVectorGetX(z), XMVectorGetY(z), XMVectorGetZ(z), 0.0f,
				XMVectorGetX(pos), XMVectorGetY(pos), XMVectorGetZ(pos), 1.0f
			};
		}

		matrix turn_to(XMMATRIX m, XMVECTOR target, float speed)
		{
			XMVECTOR v = XMVector4Normalize(target - m.r[3]);
			float dX = XMVectorGetX(XMVector4Dot(m.r[0], v)); // How far left/right is the target away from me.
			float dY = XMVectorGetX(XMVector4Dot(m.r[1], v)); // How far up/down is the target away from me.

			// Turn left or right
			if (dX > 0.0f || dX < 0.0f)
			{
				m = XMMatrixMultiply(XMMatrixRotationY(dX * speed * (float)xtime.Delta()), m);
			}

			// Turn up or down
			if (dY > 0.0f || dY < 0.0f)
			{
				m = XMMatrixMultiply(XMMatrixRotationX(-dY * speed * (float)xtime.Delta()), m);
			}

			//m = XMMatrixRotationRollPitchYaw(0.0f, dX, -dY) * m;

			return m;
		}

		matrix mouse_look(XMMATRIX view, int diffX, int diffY)
		{
			// Block input outside of 125 pixels away from center.
			if (abs(diffX) < 125 && abs(diffY) < 125)
			{
				// Create the rotation matrix based on mouse input.
				XMMATRIX rot = XMMatrixRotationRollPitchYaw(-diffY / 150.0f, -diffX / 150.0f, 0);

				(XMMATRIX&)view = XMMatrixMultiply(rot, (XMMATRIX&)view);

				XMVECTOR vExistingZ = { XMVectorGetX(view.r[2]), XMVectorGetY(view.r[2]), XMVectorGetZ(view.r[2]) };

				// Parallel to the world's horizon 
				XMVECTOR vNewX = XMVector3Cross({ 0.0f, 1.0f, 0.0f }, vExistingZ);
				XMVECTOR vNewY = XMVector3Cross(vExistingZ, vNewX);
				vExistingZ = XMVector3Normalize(vExistingZ);
				vNewY = XMVector3Normalize(vNewY);
				vNewX = XMVector3Normalize(vNewX);

				XMMATRIX newView = {
					XMVectorGetX(vNewX), XMVectorGetY(vNewX), XMVectorGetZ(vNewX), XMVectorGetW(view.r[0]),
					XMVectorGetX(vNewY), XMVectorGetY(vNewY), XMVectorGetZ(vNewY), XMVectorGetW(view.r[1]),
					XMVectorGetX(vExistingZ), XMVectorGetY(vExistingZ), XMVectorGetZ(vExistingZ), XMVectorGetW(view.r[2]),
					XMVectorGetX(view.r[3]), XMVectorGetY(view.r[3]), XMVectorGetZ(view.r[3]), XMVectorGetW(view.r[3])
				};

				return newView;
			}
			return view;
		}

		// UPDATES || Per-Frame Functions
		void create_grid_perframe()
		{
			// Draw Vertical Grid Lines
			for (int x = -12; x < 13; x++)
			{
				debug_renderer::add_line({ (float)x, -1.0f, 12.0f }, { (float)x, -1.0f, -12.0f }, { sinf((float)xtime.TotalTime()), 1.0f, sinf(-(float)xtime.TotalTime()), 1.0f });
			}

			// Draw Horizontal Grid Lines
			for (int z = -12; z < 13; z++)
			{
				debug_renderer::add_line({ 12.0f, -1.0f, (float)z }, { -12.0f, -1.0f, (float)z }, { sinf((float)xtime.TotalTime()), 1.0f, sinf(-(float)xtime.TotalTime()), 1.0f });
			}
		}

		void spawn_particles()
		{
			// -Sorted Pool Only-
			// Check to see if a particle can be allocated.
			int16_t index = -1;
			if (delay > 0.05f)
			{
				// Spawn multiple particles.
				for (int x = 0; x < PARTICLESAMT; x++)
				{
					index = sp.alloc();

					// If the index was valid, initialize the particle.
					if (index > -1)
					{
						// Create random X and Z value;
						float randX = (((rand() % 100 + 1) / 10.0f) - 5.0f) * 0.30f;
						float randZ = (((rand() % 100 + 1) / 10.0f) - 5.0f) * 0.30f;

						sp[index].pos = { randX * (float)xtime.Delta(), -1.0f, randZ * (float)xtime.Delta()};
						sp[index].pos.y += 10.0f * (float)xtime.SmoothDelta();
						sp[index].prev_pos = { 0.0f, -1.0f, 0.0f };
						sp[index].color = { 1.0f, 1.0f, 1.0f, 1.0f };
					}
				}
			}

			// -Free List Pool-
			index = -1;
			if (delay > 0.05f && emitters.size() > 0)
			{
				// Loop through all the emitters
				for (int emitIndex = 0; emitIndex < EMITTERNUM; emitIndex++)
				{
					for (int x = 0; x < PARTICLESAMT; x++)
					{
						int eIndex = emitters[emitIndex].indices.alloc();

						// Does the emitter have valid space?
						if (eIndex > -1)
						{
							index = sharedpool.alloc();

							// Check if a particle was able to be allocated.
							if (index > -1)
							{
								// Create random X and Z value;
								float randX = (((rand() % 100 + 1) / 10.0f) - 5.0f) * 0.30f;
								float randZ = (((rand() % 100 + 1) / 10.0f) - 5.0f) * 0.30f;

								sharedpool[index].pos = emitters[emitIndex].spawn_pos;
								sharedpool[index].pos = { sharedpool[index].pos.x + (randX * (float)xtime.Delta()), emitters[emitIndex].spawn_pos.y, sharedpool[index].pos.z + (randZ * (float)xtime.Delta())};
								sharedpool[index].pos.y += 10.0f * (float)xtime.SmoothDelta();
								sharedpool[index].prev_pos = { emitters[emitIndex].spawn_pos.x, emitters[emitIndex].spawn_pos.y, emitters[emitIndex].spawn_pos.z };
								sharedpool[index].color = emitters[emitIndex].spawn_color;

								// Set the particle's index to the incdices list of the emitter.
								emitters[emitIndex].indices[eIndex] = index;
							}
						}
					}
				}

				// Reset the delay.
				delay = 0.0f;
			}
		}

		void update_particles()
		{
			// -Sorted Pool-
			// Update the particle's position
			for (int i = 0; i < sp.size(); i++)
			{
				if (sp[i].pos.y < -1.0f)
				{
					sp.free(i);
					continue;
				}

				float3 delta = sp[i].pos - sp[i].prev_pos;
				float3 grav = { sinf((float)xtime.TotalTime()) * (float)xtime.Delta() * (float)xtime.Delta(), -9.14f * (float)xtime.Delta() * (float)xtime.Delta(), cosf((float)xtime.TotalTime()) * (float)xtime.Delta() * (float)xtime.Delta() };
				//float3 grav = { 0, -9.14f * time.Delta() * time.Delta(), 0 };
				sp[i].prev_pos = sp[i].pos;
				//sp[i].pos += delta + ( grav * time.Delta() * time.Delta());
				sp[i].pos += delta + grav;
			}

			// Add the particles to the debugger's lines to render.
			for (int i = 0; i < sp.size(); i++)
			{
				float3 tLength = { 0.95f, 0.95f, 0.95f };
				float3 prevPos = sp[i].prev_pos * tLength;
				debug_renderer::add_line(sp[i].pos, prevPos, sp[i].color); // I use this to create the 'line' effect, it definitly would be better to do it in the pixel shader/geo shader.
			}

			// -Shared Pool-
			if (emitters.size() > 0)
			{
				// Loop through all the emitters
				for (int z = 0; z < EMITTERNUM; z++)
				{
					// Update the particle's position based on the indices list from the emitter
					for (int i = 0; i < emitters[z].indices.size(); i++)
					{
						// Grab the index number of the particle being updated in the shared pool.
						int16_t index = emitters[z].indices[i];

						// If the particle is below -1.0f y, free it up for reuse.
						if (sharedpool[index].pos.y < -1.0f)
						{
							sharedpool.free(index); // Free in shared pool
							emitters[z].indices.free(i); // Free in the indices list.
							continue;
						}

						// Update the positional data.
						float3 delta = sharedpool[index].pos - sharedpool[index].prev_pos;
						float3 grav = { sinf((float)xtime.TotalTime()) * (float)xtime.Delta() * (float)xtime.Delta(), -9.14f * (float)xtime.Delta() * (float)xtime.Delta(), cosf((float)xtime.TotalTime()) * (float)xtime.Delta() * (float)xtime.Delta() };
						sharedpool[index].prev_pos = sharedpool[index].pos;
						sharedpool[index].pos += delta + grav;
					}

					// Add the particles to the debugger's lines to render.
					for (int i = 0; i < emitters[z].indices.size(); i++)
					{
						int16_t index = emitters[z].indices[i];
						float3 tLength = { 1.0f, 1.0f, 1.0f };
						float3 prevPos = sharedpool[index].prev_pos * tLength;
						tLength = { 1.0f, 0.95f, 1.0f };
						float3 pos = sharedpool[index].pos * tLength; // I use this to create the 'line' effect, it definitly would be better to do it in the pixel shader/geo shader.

						debug_renderer::add_line(pos, prevPos, sharedpool[index].color);
					}
				}
			}
		}

		void player_input(XMMATRIX& view)
		{
			// pMatrix - The Matrix on the grid floor
			{
				if (GetAsyncKeyState(VK_UP))
				{
					XMMATRIX translate = {
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, MOVESPEED * (float)xtime.Delta(), 1
					};
					pWorld = XMMatrixMultiply(translate, pWorld);
				}

				if (GetAsyncKeyState(VK_DOWN))
				{
					XMMATRIX translate = {
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, -MOVESPEED * (float)xtime.Delta(), 1
					};
					pWorld = XMMatrixMultiply(translate, pWorld);
				}

				if (GetAsyncKeyState(VK_LEFT))
				{
					pWorld = XMMatrixRotationY(-MOVESPEED * (float)xtime.Delta()) * pWorld;
				}

				if (GetAsyncKeyState(VK_RIGHT))
				{
					pWorld = XMMatrixRotationY(MOVESPEED * (float)xtime.Delta()) * pWorld;
				}
			}

			// Actual View Matrix movement
			{
				// Look around movement
				if ((GetKeyState(VK_RBUTTON) & 0x100) != 0)
				{
					POINT cursorPos;
					GetCursorPos(&cursorPos);

					unsigned int screenX = 1280, screenY = 720;

					int diffX = (screenX - cursorPos.x);
					int diffY = (screenY - cursorPos.y);

					view = mouse_look(view, diffX, diffY);

					//Set it back to the center
					SetCursorPos(screenX, screenY);
				}

				if (GetAsyncKeyState('W'))
				{
					XMMATRIX translate = {
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, 5.0f * (float)xtime.Delta(), 1
					};
					view = XMMatrixMultiply(translate, view);
				}

				if (GetAsyncKeyState('S'))
				{
					XMMATRIX translate = {
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					0, 0, -5.0f * (float)xtime.Delta(), 1
					};
					view = XMMatrixMultiply(translate, view);
				}

				if (GetAsyncKeyState('A'))
				{
					XMMATRIX translate = {
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					-5.0f * (float)xtime.Delta(), 0, 0, 1
					};
					view = XMMatrixMultiply(translate, view);
				}

				if (GetAsyncKeyState('D'))
				{
					XMMATRIX translate = {
					1, 0, 0, 0,
					0, 1, 0, 0,
					0, 0, 1, 0,
					5.0f * (float)xtime.Delta(), 0, 0, 1
					};
					view = XMMatrixMultiply(translate, view);
				}
			}
		}

		// RENDERING
		void render_aabb(aabb_t aabb, float4 clr = { 1.0f, 1.0f, 1.0f, 1.0f })
		{

			// Side 1 ->
			debug_renderer::add_line({ aabb.min.x, aabb.max.y, aabb.min.z },
				{ aabb.min.x, aabb.max.y, aabb.max.z }, clr); // Horizontal
			debug_renderer::add_line({ aabb.min.x, aabb.min.y, aabb.min.z },
				{ aabb.min.x, aabb.min.y, aabb.max.z }, clr); // Horizontal

			// Side 2 <-
			debug_renderer::add_line({ aabb.max.x, aabb.max.y, aabb.min.z },
				{ aabb.max.x, aabb.max.y, aabb.max.z }, clr); // Horizontal
			debug_renderer::add_line({ aabb.max.x, aabb.min.y, aabb.min.z },
				{ aabb.max.x, aabb.min.y, aabb.max.z }, clr); // Horizontal

			// Side 3 v
			debug_renderer::add_line({ aabb.max.x, aabb.max.y, aabb.max.z },
				{ aabb.min.x, aabb.max.y, aabb.max.z}, clr); // Horizontal
			debug_renderer::add_line({ aabb.max.x, aabb.min.y, aabb.max.z },
				{ aabb.min.x, aabb.min.y, aabb.max.z }, clr); // Horizontal
			debug_renderer::add_line({ aabb.max.x, aabb.max.y, aabb.max.z },
				{ aabb.max.x, aabb.min.y, aabb.max.z}, clr); // Vertical
			debug_renderer::add_line({ aabb.min.x, aabb.max.y, aabb.max.z },
				{ aabb.min.x, aabb.min.y, aabb.max.z }, clr); // Vertical

			// Side 4 ^
			debug_renderer::add_line({ aabb.max.x, aabb.max.y, aabb.min.z },
				{ aabb.min.x, aabb.max.y, aabb.min.z }, clr); // Horizontal
			debug_renderer::add_line({ aabb.max.x, aabb.min.y, aabb.min.z },
				{ aabb.min.x, aabb.min.y, aabb.min.z }, clr); // Horizontal
			debug_renderer::add_line({ aabb.max.x, aabb.max.y, aabb.min.z },
				{ aabb.max.x, aabb.min.y, aabb.min.z }, clr); // Vertical
			debug_renderer::add_line({ aabb.min.x, aabb.max.y, aabb.min.z },
				{ aabb.min.x, aabb.min.y, aabb.min.z }, clr); // Vertical
		}

		void render_debug_lines()
		{
			// Debug Line Rendering
			context->IASetInputLayout(input_layout[INPUT_LAYOUT::COLORED_VERTEX]);
			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
			context->VSSetShader(vertex_shader[VERTEX_SHADER::COLORED_VERTEX], nullptr, 0);
			context->PSSetShader(pixel_shader[PIXEL_SHADER::COLORED_VERTEX], nullptr, 0);

			context->VSSetConstantBuffers(0, 1, &constant_buffer[CONSTANT_BUFFER::MVP]);

			// Set vertex buffer
			const UINT stride[] = { sizeof(end::colored_vertex) };
			const UINT offset[] = { 0 };

			context->IASetVertexBuffers(0, 1, &vertex_buffer[VERTEX_BUFFER::COLORED_VERTEX], stride, offset);
			context->UpdateSubresource(vertex_buffer[VERTEX_BUFFER::COLORED_VERTEX], 0, NULL, debug_renderer::get_line_verts(), 0, 0);

			context->Draw((UINT)debug_renderer::get_line_vert_count(), 0);
			debug_renderer::clear_lines();
		}
	};
}