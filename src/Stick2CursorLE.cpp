#include "skse/PluginAPI.h"
#include "skse/skse_version.h"
#include "skse/GameInput.h"
#include "skse/PapyrusVM.h"
#include "skse/PapyrusNativeFunctions.h"
#include <windows.h>
#include <Xinput.h>
#include <cmath>
#include <cstdio>

// Stick2Cursor LE - experimental first implementation.
// Target runtime: Skyrim LE 1.9.32 / SKSE 1.7.3 (Win32).
namespace {
const char* kPlugin="Stick2CursorLE";
const char* kIni="Data\\SKSE\\Plugins\\Stick2CursorLE.ini";
PluginHandle g_handle=kPluginHandle_Invalid;
SKSEMessagingInterface* g_messaging=nullptr;
volatile LONG g_active=0,g_capture=0,g_captureFirst=0,g_captureSecond=0;
volatile LONG g_chordA=XINPUT_GAMEPAD_LEFT_SHOULDER,g_chordB=XINPUT_GAMEPAD_RIGHT_SHOULDER;
volatile LONG g_mode=0;       // 0 toggle, 1 hold
volatile LONG g_source=0;     // 0 right stick, 1 D-pad
volatile LONG g_speed=900;    // pixels per second at full deflection
volatile LONG g_cursorMode=0;
volatile LONG g_prevChord=0;
volatile LONG g_dpad=0;
volatile LONG g_lastX=0,g_lastY=0;
HANDLE g_thread=nullptr;
unsigned g_tick=0;

void ReadConfig(){
 g_speed=GetPrivateProfileIntA("Cursor","Speed",900,kIni);
 if(g_speed<100)g_speed=100;if(g_speed>3000)g_speed=3000;
 g_mode=GetPrivateProfileIntA("Cursor","ActivationMode",0,kIni)?1:0;
 g_source=GetPrivateProfileIntA("Cursor","InputSource",0,kIni)?1:0;
 g_chordA=GetPrivateProfileIntA("Cursor","ChordButtonA",XINPUT_GAMEPAD_LEFT_SHOULDER,kIni);
 g_chordB=GetPrivateProfileIntA("Cursor","ChordButtonB",XINPUT_GAMEPAD_RIGHT_SHOULDER,kIni);
}
void WriteInt(const char* key,LONG value){char b[24];_snprintf_s(b,sizeof(b),_TRUNCATE,"%ld",value);WritePrivateProfileStringA("Cursor",key,b,kIni);}
void MoveCursor(float dx,float dy){
 if(dx==0&&dy==0)return;
 // Inject relative mouse input so Skyrim's input/Scaleform path receives motion.
 // Do not move the desktop pointer with SetCursorPos: KHAOS can draw its own cursor.
 INPUT in{};in.type=INPUT_MOUSE;in.mi.dx=(LONG)std::lround(dx);in.mi.dy=(LONG)std::lround(dy);in.mi.dwFlags=MOUSEEVENTF_MOVE;
 SendInput(1,&in,sizeof(in));
}
DWORD WINAPI PollGamepad(void*){
 XINPUT_STATE state{};DWORD last=GetTickCount();
 while(InterlockedCompareExchange(&g_cursorMode,0,0)){
  const DWORD now=GetTickCount();float dt=(now-last)*0.001f;last=now;if(dt>0.05f)dt=0.05f;
  if(XInputGetState(0,&state)==ERROR_SUCCESS){const WORD b=state.Gamepad.wButtons;
   const WORD a=(WORD)InterlockedCompareExchange(&g_chordA,0,0),c=(WORD)InterlockedCompareExchange(&g_chordB,0,0);
   const bool chord=(b&a)!=0&&(b&c)!=0;const bool was=InterlockedExchange(&g_prevChord,chord?1:0)!=0;
   if(g_mode==0){if(chord&&!was)InterlockedExchange(&g_active,!InterlockedCompareExchange(&g_active,0,0));}
   else InterlockedExchange(&g_active,chord?1:0);
   if(InterlockedCompareExchange(&g_active,0,0)){
    const LONG speed=InterlockedCompareExchange(&g_speed,0,0);float dx=0,dy=0;
    if(InterlockedCompareExchange(&g_source,0,0)==0){const SHORT x=state.Gamepad.sThumbRX,y=state.Gamepad.sThumbRY;const float fx=(std::abs((int)x)<8000)?0.0f:(float)x/32767.0f;const float fy=(std::abs((int)y)<8000)?0.0f:(float)y/32767.0f;dx=fx*speed*dt;dy=-fy*speed*dt;}
    else{const WORD d=(WORD)(b&(XINPUT_GAMEPAD_DPAD_LEFT|XINPUT_GAMEPAD_DPAD_RIGHT|XINPUT_GAMEPAD_DPAD_UP|XINPUT_GAMEPAD_DPAD_DOWN));dx=((d&XINPUT_GAMEPAD_DPAD_RIGHT)?1.0f:0.0f)-((d&XINPUT_GAMEPAD_DPAD_LEFT)?1.0f:0.0f);dy=((d&XINPUT_GAMEPAD_DPAD_DOWN)?1.0f:0.0f)-((d&XINPUT_GAMEPAD_DPAD_UP)?1.0f:0.0f);dx*=speed*dt;dy*=speed*dt;}
    MoveCursor(dx,dy);
   }
  }else{InterlockedExchange(&g_prevChord,0);if(g_mode==1)InterlockedExchange(&g_active,0);}
  Sleep(8);
 }
 return 0;
}
void StartPolling(){if(!g_thread){InterlockedExchange(&g_cursorMode,1);g_thread=CreateThread(nullptr,0,PollGamepad,nullptr,0,nullptr);}}
void StopPolling(){InterlockedExchange(&g_cursorMode,0);if(g_thread){WaitForSingleObject(g_thread,250);CloseHandle(g_thread);g_thread=nullptr;}InterlockedExchange(&g_active,0);}

// Papyrus settings bridge: MCM owns presentation; native code owns persistence.
SInt32 GetInt(StaticFunctionTag*,BSFixedString key){const char* k=key.data?key.data:"";if(!_stricmp(k,"Mode"))return g_mode;if(!_stricmp(k,"Source"))return g_source;if(!_stricmp(k,"Speed"))return g_speed;if(!_stricmp(k,"ChordA"))return g_chordA;if(!_stricmp(k,"ChordB"))return g_chordB;return 0;}
void SetInt(StaticFunctionTag*,BSFixedString key,SInt32 v){const char* k=key.data?key.data:"";if(!_stricmp(k,"Mode")){g_mode=v?1:0;WriteInt("ActivationMode",g_mode);}else if(!_stricmp(k,"Source")){g_source=v?1:0;WriteInt("InputSource",g_source);}else if(!_stricmp(k,"Speed")){g_speed=v<100?100:v>3000?3000:v;WriteInt("Speed",g_speed);}else if(!_stricmp(k,"ChordA")){g_chordA=v;WriteInt("ChordButtonA",v);}else if(!_stricmp(k,"ChordB")){g_chordB=v;WriteInt("ChordButtonB",v);}}
void BeginCapture(StaticFunctionTag*,SInt32 slot){InterlockedExchange(&g_captureFirst,0);InterlockedExchange(&g_captureSecond,0);InterlockedExchange(&g_capture,slot==2?2:1);}
SInt32 Captured(StaticFunctionTag*,SInt32 slot){return InterlockedCompareExchange(slot==2?&g_captureSecond:&g_captureFirst,0,0);}
bool RegisterPapyrus(VMClassRegistry* vm){
 vm->RegisterFunction(new NativeFunction1<StaticFunctionTag,SInt32,BSFixedString>("GetInt","Stick2CursorNative",GetInt,vm));
 vm->RegisterFunction(new NativeFunction2<StaticFunctionTag,void,BSFixedString,SInt32>("SetInt","Stick2CursorNative",SetInt,vm));
 vm->RegisterFunction(new NativeFunction1<StaticFunctionTag,void,SInt32>("BeginCapture","Stick2CursorNative",BeginCapture,vm));
 vm->RegisterFunction(new NativeFunction1<StaticFunctionTag,SInt32,SInt32>("Captured","Stick2CursorNative",Captured,vm));return true;
}
class InputSink:public BSTEventSink<InputEvent>{public:EventResult ReceiveEvent(InputEvent** events,InputEventDispatcher*)override{
 if(!events||!*events)return kEvent_Continue;for(InputEvent* e=*events;e;e=e->next){
  if(e->deviceType==kDeviceType_Gamepad&&e->eventType==InputEvent::kEventType_Thumbstick&&g_active&&g_source==0){auto* stick=(ThumbstickEvent*)e;if(stick->keyMask==0xCu){stick->x=0;stick->y=0;}continue;}
  if(e->eventType!=InputEvent::kEventType_Button||e->deviceType!=kDeviceType_Gamepad)continue;auto* b=(ButtonEvent*)e;const float value=*(float*)&b->flags;const bool down=value!=0.0f;const WORD mask=(WORD)b->keyMask;
  if(g_capture){LONG slot=g_capture;if(down&&b->timer==0.0f){if(slot==1){InterlockedExchange(&g_captureFirst,mask);InterlockedExchange(&g_capture,2);}else{InterlockedExchange(&g_captureSecond,mask);InterlockedExchange(&g_capture,0);}}}
  if(InterlockedCompareExchange(&g_active,0,0)&&g_source==1&&(mask&(XINPUT_GAMEPAD_DPAD_LEFT|XINPUT_GAMEPAD_DPAD_RIGHT|XINPUT_GAMEPAD_DPAD_UP|XINPUT_GAMEPAD_DPAD_DOWN)))b->keyMask=0;
 }return kEvent_Continue;}};
InputSink g_sink;
void Message(SKSEMessagingInterface::Message* m){if(!m)return;if(m->type==SKSEMessagingInterface::kMessage_DataLoaded)ReadConfig();if(m->type==SKSEMessagingInterface::kMessage_InputLoaded){auto* d=*g_inputEventDispatcher;if(d)d->AddEventSink(&g_sink);StartPolling();}}
}
extern "C" {
bool SKSEPlugin_Query(const SKSEInterface* skse,PluginInfo* info){info->infoVersion=PluginInfo::kInfoVersion;info->name="Stick2CursorLE";info->version=1;return skse&&skse->isEditor==0&&skse->runtimeVersion==RUNTIME_VERSION_1_9_32_0&&skse->skseVersion>=PACKED_SKSE_VERSION;}
bool SKSEPlugin_Load(const SKSEInterface* skse){g_handle=skse->GetPluginHandle();g_messaging=(SKSEMessagingInterface*)skse->QueryInterface(kInterface_Messaging);auto* pap=(SKSEPapyrusInterface*)skse->QueryInterface(kInterface_Papyrus);if(!g_messaging||!pap)return false;pap->Register(RegisterPapyrus);g_messaging->RegisterListener(g_handle,"SKSE",Message);return true;}
}
