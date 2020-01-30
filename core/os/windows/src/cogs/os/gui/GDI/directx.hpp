//
//  Copyright (C) 2000-2019 - Colen M. Garoutte-Carson <colen at cogmine.com>, Cog Mine LLC
//


// Status: Placeholder

#ifndef COGS_HEADER_OS_GUI_OS_DIRECTX
#define COGS_HEADER_OS_GUI_OS_DIRECTX

#include "cogs/os.hpp"
#include <d3d11.h>
#include <d3d10.h>
#include <d3d9.h>
//#include <d3dx11.h>
//#include <d3dx10.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "d3d10.lib")
#pragma comment (lib, "d3d9.lib")
//#pragma comment (lib, "d3dx11.lib")
//#pragma comment (lib, "d3dx10.lib")

#include "cogs/hwnd.hpp"

namespace cogs {
namespace gui {
namespace os {

	/*
template <unsigned int version>
class directx : public hwnd_pane
{
private:
	// DX9
	LPDIRECT3D9 d3d; // the pointer to our Direct3D interface
	LPDIRECT3DDEVICE9 d3ddev; // the pointer to the device class

	// DX10
	ID3D10Device* device; // the pointer to our Direct3D device interface
	ID3D10RenderTargetView* rtv; // the pointer to the render target view
	IDXGISwapChain* swapchain; // the pointer to the swap chain class

	// DX11
	ID3D11Device *dev; // the pointer to our Direct3D device interface
	ID3D11DeviceContext *devcon; // the pointer to our Direct3D device context
	IDXGISwapChain *swapchain; // the pointer to the swap chain interface


public:
	directx(const rcref<frame>& owner)
		: hwnd_pane(owner, composite_string(), composite_string(), WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_NOPARENTNOTIFY, uiSubsystem)
	{ }

	~directx()
	{
		if (version == 9)
		{
			d3ddev->Release(); // close and release the 3D device
			d3d->Release(); // close and release Direct3D
		}

		if (version == 10)
		{
			swapchain->Release(); // close and release the swap chain
			rtv->Release(); // close and release the render target view
			device->Release(); // close and release the 3D device
		}

		if (version == 11)
		{
			// close and release all existing COM objects
			swapchain->Release();
			dev->Release();
			devcon->Release();
		}
	}

	//virtual color get_background_color() const { return color::constant::green; }

	virtual void installing()
	{
		if (version == 9)
		{
			d3d = Direct3DCreate9(D3D_SDK_VERSION); // create the Direct3D interface

			D3DPRESENT_PARAMETERS d3dpp; // create a struct to hold various device information

			ZeroMemory(&d3dpp, sizeof(d3dpp)); // clear out the struct for use
			d3dpp.Windowed = TRUE; // program windowed, not fullscreen
			d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD; // discard old frames
			d3dpp.hDeviceWindow = hWnd; // set the window to be used by Direct3D

			// create a device class using this information and information from the d3dpp stuct
			d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3ddev);
		}

		if (version == 10)
		{
			DXGI_SWAP_CHAIN_DESC scd; // create a struct to hold various swap chain information

			ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC)); // clear out the struct for use

			scd.BufferCount = 1; // create two buffers, one for the front, one for the back
			scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // use 32-bit color
			scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // tell how the chain is to be used
			scd.OutputWindow = hWnd; // set the window to be used by Direct3D
			scd.SampleDesc.Count = 1; // set the level of multi-sampling
			scd.SampleDesc.Quality = 0; // set the quality of multi-sampling
			scd.Windowed = TRUE; // set to windowed or full-screen mode

			// create a device class and swap chain class using the information in the scd struct
			D3D10CreateDeviceAndSwapChain(NULL, D3D10_DRIVER_TYPE_HARDWARE, NULL, 0, D3D10_SDK_VERSION, &scd, &swapchain, &device);

			// get the address of the back buffer and use it to create the render target
			ID3D10Texture2D* pBackBuffer;
			swapchain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*)&pBackBuffer);
			device->CreateRenderTargetView(pBackBuffer, NULL, &rtv);
			pBackBuffer->Release();

			// set the render target as the back buffer
			device->OMSetRenderTargets(1, &rtv, NULL);

			D3D10_VIEWPORT viewport; // create a struct to hold the viewport data

			ZeroMemory(&viewport, sizeof(D3D10_VIEWPORT)); // clear out the struct for use

			viewport.TopLeftX = 0; // set the left to 0
			viewport.TopLeftY = 0; // set the top to 0
			viewport.Width = 800; // set the width to the window's width
			viewport.Height = 600; // set the height to the window's height

			device->RSSetViewports(1, &viewport); // set the viewport
		}

		if (version == 11)
		{
			// create a struct to hold information about the swap chain
			DXGI_SWAP_CHAIN_DESC scd;

			// clear out the struct for use
			ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

			// fill the swap chain description struct
			scd.BufferCount = 1; // one back buffer
			scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // use 32-bit color
			scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // how swap chain is to be used
			scd.OutputWindow = hWnd; // the window to be used
			scd.SampleDesc.Count = 4; // how many multisamples
			scd.Windowed = TRUE; // windowed/full-screen mode

			// create a device, device context and swap chain using the information in the scd struct
			D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, NULL, NULL, NULL, D3D11_SDK_VERSION, &scd, &swapchain, &dev, NULL, &devcon);
		}

		hwnd_pane::installing();
	}

	virtual void pre_draw()
	{
		if (version == 9)
		{
			// clear the window to a deep blue
			d3ddev->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 40, 100), 1.0f, 0);
			d3ddev->BeginScene(); // begins the 3D scene
		}

		if (version == 9)
		{
			vice->ClearRenderTargetView(rtv, D3DXCOLOR(0.0f, 0.2f, 0.4f, 1.0f));
		}
	}

	virtual void post_draw()
	{
		if (version == 9)
		{
			d3ddev->EndScene(); // ends the 3D scene
			d3ddev->Present(NULL, NULL, NULL, NULL); // displays the created frame
		}

		if (version == 10)
		{
			// display the rendered frame
			swapchain->Present(0, 0);
		}
	}

	virtual void invalidate(const canvas::bounds& b
	{
		// Add bounds to stencil
	}


	virtual void reshape(const bounds& b)
	{
		hwnd_pane::reshape(b);
	}

	// 2D
	virtual void fill(const bounds& b, const color& c = color::constant::black, bool blendAlpha = true)
	{
		point topRight = b.calc_top_right();
		point bottomLeft = b.calc_bottom_left();

	}

	virtual void invert(const bounds& b)
	{
	}

	virtual void draw_line(const point& startPt, const point& endPt, double width = 1, const color& c = color::constant::black, bool blendAlpha = true)
	{
	}

	virtual void draw_text(const composite_string& s, const bounds& b, const color& c = color::constant::black)
	{
	}

	virtual void draw_bitmap(const gfx::bitmap& src, const bounds& srcBounds, const bounds& dstBounds, bool blendAlpha = true )
	{
	}

	virtual void draw_bitmap(const gfx::bitmask& msk, const bounds& mskBounds, const bounds& dstBounds, const color& fore = color::constant::black, const color& back = color::constant::white, bool blendForeAlpha = true, bool blendBackAlpha = true )
	{
	}

	virtual rcptr<gfx::bitmap> create_bitmap(const size& sz, std::optional<color> fillColor = std::nullopt)
	{
		return rcptr<gfx::bitmap>();
	}

	virtual rcptr<gfx::bitmap> load_bitmap(const composite_string& location)
	{
		return rcptr<gfx::bitmap>();
	}

	virtual rcptr<gfx::bitmask> load_bitmask(const composite_string& location)
	{
		return rcptr<gfx::bitmask>();
	}

	virtual rcptr<volatile gui::subsystem> get_subsystem()
	{
		return this_rcref;
	}

	virtual rcptr<console> get_default_console() volatile
	{
		return rcptr<console>(); // TBD
	}

	virtual rcptr<console> create_console() volatile
	{
		return rcptr<console>(); // TBD
	}

	virtual rcref<task<void> > message(const composite_string& msg) volatile
	{
	}

	virtual rcref<task<void> > open(const composite_string& title, const rcref<frame>& f, const rcref<frame::reshaper>& f = rcnew(frame::default_reshaper)) volatile
	{
	}

	// planar 
	virtual rcref<view> create_button(const rcref<gui::button>& btn) volatile
	{
		rcptr<view> a;
		return a.get_ref();
	}

	virtual rcref<view> create_check_box(const rcref<gui::check_box>& cb) volatile
	{
		rcptr<view> a;
		return a.get_ref();
	}

	virtual rcref<view> create_text_editor(const rcref<gui::text_editor>& te) volatile
	{
		rcptr<view> a;
		return a.get_ref();
	}

	virtual rcref<view> create_scroll_bar(const rcref<gui::scroll_bar>& sb) volatile
	{
		rcptr<view> a;
		return a.get_ref();
	}

	// TBD - Create frame buffer objects for nested canvas's
//	virtual rcptr<view> create_canvas(const rcref<canvas_panel>& cf) { return rcptr<view>(); }
//	virtual rcptr<view> create_canvas(const rcref<canvas_frame>& cf) { return rcptr<view>(); }

//	virtual rcptr<view> create_canvas3D(const rcref<canvas3D_frame>& cf) { return rcptr<view>(); }
};
*/
}
}
}

#endif
