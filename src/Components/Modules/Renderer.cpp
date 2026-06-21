#include "STDInc.hpp"

namespace Components
{
	Utils::Signal<Renderer::BackendCallback> Renderer::BackendFrameSignal;
	Utils::Signal<Renderer::BackendCallback> Renderer::SingleBackendFrameSignal;

	Utils::Signal<Renderer::Callback> Renderer::EndRecoverDeviceSignal;
	Utils::Signal<Renderer::Callback> Renderer::BeginRecoverDeviceSignal;

	constexpr std::uint32_t dynamic_vb_size = 2 * 1048576;
	constexpr std::uint32_t skinned_cache_size = 8 * 1048576;
	constexpr std::uint32_t temp_skin_size = 12 * 1048576;
	constexpr std::uint32_t dynamic_ib_size = 4 * 1048576;
	constexpr std::uint32_t pretess_ib_size = 4 * 1048576;

	void Renderer::OnDeviceRecoveryEnd(Utils::Slot<Renderer::Callback> callback)
	{
		Renderer::EndRecoverDeviceSignal.connect(callback);
	}

	void Renderer::OnDeviceRecoveryBegin(Utils::Slot<Renderer::Callback> callback)
	{
		Renderer::BeginRecoverDeviceSignal.connect(callback);
	}

	void Renderer::PreVidRestart()
	{
		Renderer::BeginRecoverDeviceSignal();
	}

	void Renderer::PostVidRestart()
	{
		Renderer::EndRecoverDeviceSignal();
	}

	int Renderer::Width()
	{
		return reinterpret_cast<LPPOINT>(0x1621DB4)->x;
	}

	int Renderer::Height()
	{
		return reinterpret_cast<LPPOINT>(0x1621DB4)->y;
	}

	__declspec(naked) void Renderer::PostPreVidRestart()
	{
		__asm
		{
			pushad
			call Renderer::PreVidRestart
			popad

			push 535E80h
			retn
		}
	}

	__declspec(naked) void Renderer::PostVidRestartStub()
	{
		__asm
		{
			pushad
			call Renderer::PostVidRestart
			popad

			push 445390h
			retn
		}
	}

	float Renderer::CG_GetViewFov(int localClientNum)
	{
		float calc_fov = Dvars::cg_fov->current.value;
		unsigned int offhand_index = Game::ps->offHandIndex;
		if ((Game::ps->weapFlags & 2) == 0)
		{
			offhand_index = Game::ps->weapon;
		}

		Game::WeaponDef_s* weapon = Game::bg_weaponDefs[offhand_index];

		auto check_flags_and_fovmin = [&]() -> float
		{
			if ((Game::ps->eFlags & 0x300) != 0)
			{
				if (Game::ps->viewlocked)
				{
					auto func = Utils::Hook::Call<int()>(0x4356B0)();
					if (Game::bg_weaponDefs[func]->overlayInterface == Game::WEAPOVERLAYINTERFACE_TURRETSCOPE)
						return calc_fov = Dvars::Functions::Dvar_FindVar("turretScopeZoom")->current.value;
				}
				return calc_fov = 55.0f;
			}

			calc_fov = Dvars::cg_fovScale->current.value * calc_fov;
			float calc_fov_min = Dvars::cg_fovMin->current.value - calc_fov;

			if (calc_fov_min >= 0.0)
				return Dvars::cg_fovMin->current.value;

			//	From MW2
			if (calc_fov > 170.0f)
				return calc_fov = 170.0f;

			return calc_fov;
		};

		if (Game::ps->pm_type == 5)
		{
			calc_fov = 90.0f;
			return check_flags_and_fovmin();
		}

		if (weapon->aimDownSight)
		{
			if (Game::ps->fWeaponPosFrac == 1.0f)
			{
				calc_fov = weapon->fAdsZoomFov;
				return check_flags_and_fovmin();
			}

			if (0.0f != Game::ps->fWeaponPosFrac)
			{
				float ads_factor;

				if (Game::cgs->playerEntity.bPositionToADS)
				{
					const float w_pos_frac = Game::ps->fWeaponPosFrac - (1.0f - weapon->fAdsZoomInFrac);
					if (w_pos_frac <= 0.0f)
					{
						return check_flags_and_fovmin();
					}
					ads_factor = w_pos_frac / weapon->fAdsZoomInFrac;
				}
				else
				{
					const float w_pos_frac = Game::ps->fWeaponPosFrac - (1.0f - weapon->fAdsZoomOutFrac);
					if (w_pos_frac <= 0.0f)
					{
						return check_flags_and_fovmin();
					}
					ads_factor = w_pos_frac / weapon->fAdsZoomOutFrac;
				}

				if (ads_factor > 0.0f)
				{
					calc_fov = calc_fov - ads_factor * (calc_fov - weapon->fAdsZoomFov);
				}
			}
		}

		return check_flags_and_fovmin();
	}

	int Renderer::R_AllocDynamicIndexBuffer(IDirect3DIndexBuffer9** ib, int size_in_bytes, const char* buffer_name)
	{
		if (HRESULT hr = Game::GetDevice()->CreateIndexBuffer(size_in_bytes, (D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY), D3DFMT_INDEX16, D3DPOOL_DEFAULT, ib, nullptr);
			hr < 0)
		{
			const char* msg = Utils::function<const char* __stdcall(HRESULT)>(0x65350E)(hr); // R_ErrorDescription
			msg = Utils::String::VA("DirectX didn't create a 0x%.8x dynamic index buffer: %s\n", size_in_bytes, msg);

			Utils::function<void(const char*)>(0x5951C0)(msg); // Sys_Error
		}

		return 0;
	}

	void Renderer::R_InitDynamicIndexBufferState(Game::GfxIndexBufferState* ib, std::uint32_t index_count, const char* buffer_name)
	{
		ib->used = 0;
		ib->total = index_count;

		R_AllocDynamicIndexBuffer(&ib->buffer, 2 * index_count, buffer_name);
	}

	char* Renderer::R_AllocDynamicVertexBuffer(IDirect3DVertexBuffer9** vb, std::uint32_t size_in_bytes, const char* buffer_name)
	{
		if (HRESULT hr = Game::GetDevice()->CreateVertexBuffer(size_in_bytes, (D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY), 0, D3DPOOL_DEFAULT, vb, nullptr);
			hr < 0)
		{
			const char* msg = Utils::function<const char* __stdcall(HRESULT)>(0x65350E)(hr); // R_ErrorDescription
			msg = Utils::String::VA("DirectX didn't create a 0x%.8x dynamic vertex buffer: %s\n", size_in_bytes, msg);

			Utils::function<void(const char*)>(0x5951C0)(msg); // Sys_Error
		}

		return nullptr;
	}

	void Renderer::R_InitDynamicVertexBufferState(Game::GfxVertexBufferState* vb, std::uint32_t bytes, const char* buffer_name)
	{
		vb->used = 0;
		vb->total = static_cast<int>(bytes);
		vb->verts = nullptr;

		R_AllocDynamicVertexBuffer(&vb->buffer, bytes, buffer_name);
	}

	void Renderer::R_InitTempSkinBuf(std::uint32_t bytes)
	{
		auto* back_end_data = reinterpret_cast<Game::GfxBackEndData*>(0x1629100);
		for (auto i = 0; i < 2; ++i)
		{
			back_end_data[i].tempSkinBuf = (char*)VirtualAlloc(nullptr, bytes, MEM_RESERVE, PAGE_READWRITE);
		}
	}

	void Renderer::R_CreateDynamicBuffers()
	{
		if (Dvars::r_increaseRenderBuffers->current.enabled)
		{
			R_InitDynamicVertexBufferState(&Game::gfx_buf->dynamicVertexBufferPool[0], dynamic_vb_size, "dynamicVertexBufferPool");
			Game::gfx_buf->dynamicVertexBuffer = Game::gfx_buf->dynamicVertexBufferPool;

			R_InitDynamicVertexBufferState(&Game::gfx_buf->skinnedCacheVbPool[0], skinned_cache_size, "skinnedCacheVbPool");
			R_InitDynamicVertexBufferState(&Game::gfx_buf->skinnedCacheVbPool[1], skinned_cache_size, "skinnedCacheVbPool");
			R_InitTempSkinBuf(temp_skin_size);

			R_InitDynamicIndexBufferState(&Game::gfx_buf->dynamicIndexBufferPool[0], dynamic_ib_size, "dynamicIndexBufferPool");
			Game::gfx_buf->dynamicIndexBuffer = Game::gfx_buf->dynamicIndexBufferPool;

			R_InitDynamicIndexBufferState(&Game::gfx_buf->preTessIndexBufferPool[0], pretess_ib_size, "preTessIndexBufferPool");
			R_InitDynamicIndexBufferState(&Game::gfx_buf->preTessIndexBufferPool[1], pretess_ib_size, "preTessIndexBufferPool");
			Game::gfx_buf->preTessBufferFrame = 0;
			Game::gfx_buf->preTessIndexBuffer = Game::gfx_buf->preTessIndexBufferPool;
		}
		else
		{
			Utils::Hook::Call<void()>(0x600520)();
		}
	}

	__declspec(naked) void Renderer::r_warn_temp_skin_size_limit_stub()
	{
		const static uint32_t retn_addr = 0x61E388;
		__asm
		{
			push	eax;
			mov		eax, Dvars::r_increaseRenderBuffers;
			cmp		byte ptr[eax + 12], 1;
			pop		eax;

			jne		DISABLE;

			jmp		ENABLE;
		DISABLE:
			push	eax;
			mov		eax, 480000h;
			cmp		edx, eax;
			pop		eax;

			jmp		retn_addr;
		ENABLE:
			push	eax;
			mov		eax, temp_skin_size;
			cmp		edx, eax;
			pop		eax;

			jmp		retn_addr;
		}
	}

	__declspec(naked) void Renderer::r_warn_max_skinned_cache_vertices_limit_stub()
	{
		const static uint32_t retn_addr = 0x61E262;
		__asm
		{
			push	eax;
			mov		eax, Dvars::r_increaseRenderBuffers;
			cmp		byte ptr[eax + 12], 1;
			pop		eax;

			jne		DISABLE;

			jmp		ENABLE;
		DISABLE:
			push	eax;
			mov		eax, 480000h;
			cmp		edx, eax;
			pop		eax;

			jmp		retn_addr;
		ENABLE:
			push	eax;
			mov		eax, temp_skin_size;
			cmp		edx, eax;
			pop		eax;

			jmp		retn_addr;
		}
	}

	Renderer::Renderer()
	{
		Events::OnMapLoad([]
		{
			if(Dvars::r_increaseRenderBuffers->current.enabled)
			{
				Dvars::Override::DvarBoolOverride("r_fastSkin", false, Game::saved);
				Dvars::Override::DvarBoolOverride("r_skinCache", false, Game::saved);
			}
		});

		Events::OnDvarInit([]
		{
			Dvars::r_increaseRenderBuffers = Dvars::Register::Dvar_RegisterBool("r_increaseRenderBuffers", "Enables/Disables increasing rendering buffers\n^1Warning:^7 If you enable this dvar, next dvars 'r_fastSkin' and 'r_skinCache' will be turned off after start the local server/map\n^1NOT RECOMMEND CHANGE VALUE WHILE IN-GAME!", false, Game::saved);
		});

		// many thanks to xoxor4d for 'render buffer' limits increasing
		// https://github.com/xoxor4d/iw3xo-dev/blob/4eeba0bf63fbf991f44be6eae105ea9e60df0c3f/src/components/modules/_renderer.cpp#L1059
		/*
		* Increase the amount of skinned vertices (bone controlled meshes) per frame.
		*      (R_MAX_SKINNED_CACHE_VERTICES | TEMP_SKIN_BUF_SIZE) Warnings
		*           'r_fastSkin' or 'r_skinCache' needs to be disabled or
		*			  the client will crash if you hit an unkown limit
		*/

		Utils::Hook(0x5D8BD2, R_CreateDynamicBuffers, HOOK_CALL).install()->quick();

		// Change 'R_WARN_TEMP_SKIN_BUF_SIZE' warning limit to new buffer size
		Utils::Hook::Nop(0x61E382, 6); // clear
		Utils::Hook(0x61E382, r_warn_temp_skin_size_limit_stub, HOOK_JUMP).install()->quick();

		// Change 'R_WARN_MAX_SKINNED_CACHE_VERTICES' warning limit to new buffer size
		Utils::Hook::Nop(0x61E25C, 6); // clear
		Utils::Hook(0x61E25C, r_warn_max_skinned_cache_vertices_limit_stub, HOOK_JUMP).install()->quick();

		//	Begin device recovery (not D3D9Ex)
		Utils::Hook(0x5DA0A0, []
		{
			Game::DB_BeginRecoverLostDevice();
			BeginRecoverDeviceSignal();
		}, HOOK_CALL).install()->quick();

		//	End device recovery (not D3D9Ex)
		Utils::Hook(0x5DA1A3, []
		{
			EndRecoverDeviceSignal();
			Game::DB_EndRecoverLostDevice();
		}, HOOK_CALL).install()->quick();

		//	Begin vid_restart
		Utils::Hook(0x444FCF, PostPreVidRestart, HOOK_CALL).install()->quick();
		//	End vid_restart
		Utils::Hook(0x4450CD, PostVidRestartStub, HOOK_CALL).install()->quick();
		
		Utils::Hook(0x42E1D1, CG_GetViewFov, HOOK_CALL).install()->quick();
	}

	Renderer::~Renderer()
	{
		BackendFrameSignal.clear();
		SingleBackendFrameSignal.clear();

		EndRecoverDeviceSignal.clear();
		BeginRecoverDeviceSignal.clear();
	}
}