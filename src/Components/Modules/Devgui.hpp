#pragma once

#define _QWORD uint64_t
#define __PAIR64__(high, low)   (((uint64_t) (high) << 32) | (uint32_t)(low))
#define CLAMP(x, low, high) ((x) < (low) ? (low) : ((x) > (high) ? (high) : (x)))

#define _DWORD uint32_t
#define DWORDn(x, n)  (*((_DWORD*)&(x)+n))

#define LAST_IND(x,part_type)    (sizeof(x)/sizeof(part_type) - 1)
#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN
#  define LOW_IND(x,part_type)   LAST_IND(x,part_type)
#  define HIGH_IND(x,part_type)  0
#else
#  define HIGH_IND(x,part_type)  LAST_IND(x,part_type)
#  define LOW_IND(x,part_type)   0
#endif

#define LODWORD(x) DWORDn(x,LOW_IND(x,_DWORD))

namespace Game
{
    enum DevGuiInputAxis
    {
        SCROLL_XAXIS = 0x0,
        SCROLL_YAXIS = 0x1,
        SCROLL_AXIS_COUNT = 0x2,
    };

    enum DevGuiInputButton
    {
        INPUT_UP = 0x0,
        INPUT_DOWN = 0x1,
        INPUT_LEFT = 0x2,
        INPUT_RIGHT = 0x3,
        INPUT_ACCEPT = 0x4,
        INPUT_REJECT = 0x5,
        INPUT_BIND = 0x6,
        INPUT_GRAPH_EDIT = 0x7,
        INPUT_GRAPH_ADD = 0x8,
        INPUT_GRAPH_REMOVE = 0x9,
        INPUT_GRAPH_SAVE = 0xA,
        INPUT_COUNT = 0xB,
    };

    struct ClGuiGlob 
    {
        bool inited;
        int32_t mapDirCount;
        char mapDirs[8][64];
        const char** mapNames[8];
        dvar_s* mapEnumDvar[8];
    };

    enum DevGuiInputState
    {
        SCROLL_NONE = 0x0,
        SCROLL_PRESSED = 0x1,
        SCROLL_STALLED = 0x2,
        SCROLL_HELD = 0x3,
    };

    enum DevGuiTokenResult 
    {
        DEVGUI_TOKEN_ERROR = 0x0,
        DEVGUI_TOKEN_MORE = 0x1,
        DEVGUI_TOKEN_LAST = 0x2,
    };

    struct DevGuiInput 
    {
        float mousePos[2];
        bool buttonDown[11];
        bool prevButtonDown[11];
        float scrollScale;
        DevGuiInputState digitalStates[2];
        float digitalAxis[2];
        float digitalTimes[2];
        DevGuiInputState analogStates[2];
        float analogAxis[2];
        float analogTimes[2];
        int16_t menuScroll[2];
        float menuScrollTime[2];
        float digitalSliderTime;
        float analogSliderTime;
        float sliderScrollTime;
        float sliderScrollMaxTimeStep;
    };

    union DevMenuChild 
    {
        const char* command;
        const Game::dvar_s* dvar;
        Game::DevGraph* graph;
        uint16_t menu;
    };

    struct DevMenuItem 
    {
        char label[26];
        uint8_t childType;
        uint8_t childMenuMemory;
        int16_t sortKey;
        uint16_t nextSibling;
        uint16_t prevSibling;
        uint16_t parent;
        DevMenuChild child;
    };

    struct devguiGlob_t
    {
        DevMenuItem menus[600];
        DevMenuItem* nextFreeMenu;
        DevMenuItem topmostMenu;
        bool bindNextKey;
        bool isActive;
        bool editingMenuItem;
        uint16_t selectedMenu;
        int32_t selRow;
        bool editingKnot;
        int32_t top;
        int32_t bottom;
        int32_t left;
        int32_t right;
        int32_t sliderWidth;
    };
}

namespace Components
{
	class Devgui : public Component
	{
	public:
        static void DevGui_KeyEvent(const int localClientNum, const int key, const int down, const unsigned time);
        static void DevGui_MouseEvent(int32_t dx, int32_t dy);
        static bool DevGui_IsActive();
        static void DevGui_Toggle();
        static void DevGui_Reset();

        static void Dvar_SetVec3FromSource(Game::dvar_s* dvar, float x, float y, float z, Game::DvarSetSource source);
        static void Dvar_SetModified(Game::dvar_s* dvar);

		Devgui();
		~Devgui();
	private:
        static int DevGui_GetScreenWidth();
        static int DevGui_GetScreenHeight();

        //void functions
        static void DevGui_Init();
        static void DevGui_RegisterDvars();
        static void DevGui_AdvanceChildNum(int32_t numberToAdvance);
        static void DevGui_SelectTopLevelChild();
        static void DevGui_ChooseOrigin(int32_t* origin);
        static void DevGui_DrawFont(int32_t x, int32_t y, const uint8_t* color, const char* text);
        static void DevGui_DrawBox(int32_t x, int32_t y, int32_t w, int32_t h, const uint8_t* color);
        static void DevGui_DrawQuad(int32_t(*vtxs)[2], float* color);
        static void DevGui_DrawBevelBox(int32_t x, int32_t y, int32_t w, int32_t h, float shade, const uint8_t* color);
        static void DevGui_DrawMenuHorizontally(const Game::DevMenuItem* menu, uint16_t activeChild, int32_t* origin);
        static void DevGui_DrawMenuVertically(const Game::DevMenuItem* menu, uint16_t activeChild, int32_t* origin);
        static void DevGui_DrawMenu(uint16_t menuHandle, uint16_t activeChild, int32_t* origin);
        static void DevGui_DrawSliderPath(int32_t x, int32_t y);
        static void DevGui_DrawSingleSlider(int32_t x, int32_t y, int32_t rowWidth, int32_t rowHeight, float fraction, const uint8_t* knobColor);
        static void DevGui_DrawDvarValue(int32_t x, int32_t y, const Game::dvar_s* dvar);
        static void DevGui_DrawSliders(const Game::DevMenuItem* menu);
        static void DevGui_DrawBoxCentered(int32_t centerX, int32_t centerY, int32_t w, int32_t h, const uint8_t* color);
        static void DevGui_DrawLine(float* start, float* end, int32_t width, const uint8_t* color);
        static void DevGui_DrawGraph(const Game::DevMenuItem* menu, int32_t localClientNum);
        static void DevGui_DrawBindNextKey();
        static void DevGui_Draw(int32_t localClientNum);
        static void DevGui_InputInit(); // <-- need remove this :/
        static void R_CreateDevGui();
        static void CL_CreateDevGui();
        static void DevGui_OpenMenu(const char* path);
        static void DevGui_AddDvar(const char* path, const Game::dvar_s* dvar);
        static void DevGui_AddCommand(const char* path, const char* command);
        static void CL_InitDevGui();
        static void DevGui_InputUpdateMouse();
        static void DevGui_UpdateScrollInputs(int32_t localClientNum);
        static void DevGui_UpdateScrollStates(float deltaTime, Game::DevGuiInputState* states, float* axis, float* times);
        static void DevGui_UpdateMenuScroll(float deltaTime);
        static void DevGui_SelectNextMenuItem();
        static void DevGui_MoveDown();
        static void DevGui_SelectPrevMenuItem();
        static void DevGui_MoveUp();
        static void DevGui_MoveLeft();
        static void DevGui_MoveRight();
        static void DevGui_MoveSelectionHorizontally();
        static void DevGui_MoveSelectionVertically();
        static void DevGui_Accept(int32_t localClientNum);
        static void DevGui_Reject();
        static void DevGui_AddGraphKnot(Game::DevGraph* graph, int32_t localClientNum);
        static void DevGui_RemoveGraphKnot(Game::DevGraph* graph, int32_t localClientNum);
        static void DevGui_UpdateGraph(int32_t localClientNum, float deltaTime);
        static void DevGui_UpdateDvar(float deltaTime);
        static void DevGui_Update(int32_t localClientNum, float deltaTime);
        static void GraphFloat_DevGuiCB_Text(const Game::DevGraph* devGuiGraph, float inputX, float inputY, char* text);
        static void DevGui_AddGraph(const char* path, Game::DevGraph* graph);
        static void GraphFloat_DevGuiCB_Event(const Game::DevGraph* graph, Game::DevEventType event);
        static void GraphFloat_CreateDevGui(Game::GraphFloat* graph, const char* devguiPath);
        static void DevGui_KeyPressed(int32_t key);
        static void CG_InitVisionSetsMenu();
        static void CG_AddVisionSetMenuItem(Game::XAssetHeader header);

        static uint16_t DevGui_GetMenuParent(uint16_t handle);
        static int32_t DevGui_SubMenuTextWidth();
        static int32_t DevGui_MenuItemWidth(const Game::DevMenuItem* menu);
        static int32_t DevGui_MaxChildMenuWidth(const Game::DevMenuItem* menu);
        static Game::devguiGlob_t* DevGui_GetMenu(uint16_t handle);
        static Game::DevGuiTokenResult DevGui_PathToken(const char** pathInOut, char* label, __int16* sortKeyOut);
        static int32_t DevGui_GetFontHeight();
        static int32_t DevGui_GetFontWidth(const char* text);
        static bool DevGui_EditableMenuItem(const Game::DevMenuItem* menu);
        static bool DevGui_MenuItemDisabled(const Game::DevMenuItem* menu);
        static bool DevGui_IsButtonDown(Game::DevGuiInputButton button);
        static int32_t DevGui_DvarRowCount(const Game::dvar_s* dvar);
        static int32_t DevGui_GetSliderPath(uint16_t menuHandle, char* path, int32_t pathLen);
        static bool DevGui_IsValidPath(const char* path);
        static uint16_t DevGui_FindMenu(uint16_t parentHandle, const char* label);
        static uint16_t DevGui_GetMenuHandle(Game::DevMenuItem* menu);
        static int32_t DevGui_CompareMenus(const Game::DevMenuItem* menu0, const Game::DevMenuItem* menu1);
        static uint16_t DevGui_CreateMenu(uint16_t parentHandle, const char* label, __int16 sortKey);
        static uint16_t DevGui_RegisterMenu(uint16_t parentHandle, const char* label, __int16 sortKey);
        static uint16_t DevGui_ConstructPath_r(uint16_t parent, const char* path);
        static bool DevGui_InputUpdate(int32_t localClientNum, float deltaTime);
        static __int16 DevGui_GetMenuScroll(Game::DevGuiInputAxis axis);
        static bool DevGui_IsButtonReleased(Game::DevGuiInputButton button);
        static const Game::dvar_s* DevGui_SelectedDvar();
        static int32_t DevGui_ScrollDownInternal();
        static int32_t DevGui_ScrollDown();
        static int32_t DevGui_ScrollUpInternal();
        static int32_t DevGui_ScrollUp();
        static int32_t DevGui_UpdateSelection();
        static bool DevGui_IsButtonPressed(Game::DevGuiInputButton button);
        static double DevGui_UpdateFloatScroll(float deltaTime, float value, float min, float max, float step, Game::DevGuiInputAxis axis);
        static int32_t DevGui_UpdateIntScroll(float deltaTime, int32_t value, int32_t min, int32_t max, Game::DevGuiInputAxis axis);
        static float DevGui_PickFloatScrollStep(float min, float max);


        static void AimAssist_InitStub();
        static void CL_InitUI_stub01();
        static void CL_StartHunkUsersStub();
        static bool CL_IsConsoleKey(int32_t key);
	};
}