#pragma once
#include <windows.h>

#define WM_REPLAY_GAIN_CALC_PROGRESS        WM_USER + 0x1000
#define WM_REPLAY_GAIN_CALC_FINISH          WM_USER + 0x1001
#define WM_REPLAY_GAIN_CALC_ERROR           WM_USER + 0x1002

#define WM_KGPLAYER_PLAYER_STATE            WM_USER + 0x2010
#define WM_KGPLAYER_BUFFERING_DATA          WM_USER + 0x2012
#define WM_KGPLAYER_PLAYING_PROGRESS        WM_USER + 0x2013
#define WM_KGPLAYER_FFT_DATA                WM_USER + 0x2016
#define WM_KGPLAYER_WAVE_DATA               WM_USER + 0x2017
#define WM_KGPLAYER_GET_MV_WINDOW			WM_USER + 0x2018

#define PLAYER_STATE_READY                  0
#define PLAYER_STATE_PLAYING                1
#define PLAYER_STATE_STOPPED                2
#define PLAYER_STATE_PAUSED                 3
#define PLAYER_STATE_OPEN                   4
#define PLAYER_STATE_PLAYEND                5
#define PLAYER_STATE_PLAYERROR              6
#define PLAYER_STATE_BUFFERING              7
#define PLAYER_STATE_BUFFERING_TIMEOUT      8

enum OutputType
{
    outputTypeNone    = -1,
    outputTypeDSound  = 0,
    outputTypeWaveout = 1,
    outputTypeKStream = 2
    //outputTypeASIO      = 3
};

enum ProxyType
{
    TYPE_SOCK4    = 0,
    TYPE_SOCK5    = 1,
    TYPE_HTTP    = 2
};

enum KAudioTrackType
{
    SINGING,
    BACKGROUND_ONLY,
    MIXED
};

typedef enum _LineTpye {  MIX = 0, MICPHONE , WAVEIN } LineType;
typedef enum _TPlayerState  
{
    psReady = 0,
    psPlaying = 1, 
    psStopped = 2, 
    psPaused = 3,
    psOpen = 4, 
    psPlayEnd = 5, 
    psPlayError = 6,
    psBuffering = 7, 
    psBufferingTimeout = 8,
    psDeviceError = 9,
    psStarted = 10
}TPlayerState;

struct TPluginInfo
{
    wchar_t Name[_MAX_PATH];
    wchar_t DllPath[_MAX_PATH];
    bool IsActive;
};

struct TAudioInfo
{
    int BitPerSample;
    int Channels; 
    int SamplesPerSec; 
    int BitRate;
    bool IsFloat;
    __int64 Duration;
};

interface IDecoderEx : public IUnknown
{
    virtual bool __stdcall Open(const wchar_t* fileName) = 0;
    virtual void __stdcall Seek(__int64 seconds) = 0;
    virtual __int64 __stdcall GetPosition() = 0;
    virtual int __stdcall GetPCMData(char* data, int len) = 0;
    virtual void __stdcall GetAudioInfo(TAudioInfo* audioInfo) = 0;
};

interface IExtraMediaInfo : public IUnknown
{
    virtual __int64 __stdcall GetStartTime() = 0; 
    virtual __int64 __stdcall GetStopTime() = 0; 
    virtual __int32 __stdcall GetAudioTrackMappingCode() = 0;
};

class ILegacyP2PStream;
struct TDeviceInfo;
interface IAsyncEvent : public IUnknown
{
    virtual HRESULT __stdcall GetAssociatedObject(IUnknown** obj) = 0;
    virtual HRESULT __stdcall GetProgress(__int64* time) = 0;
    virtual HRESULT __stdcall GetStatus() = 0;  
};

interface IAsyncCallback : public IUnknown
{
    virtual HRESULT __stdcall Invoke(IAsyncEvent* asyncEvent) = 0;
};

//------------------------------------------------------------------------------
interface ICorePlayer : public IUnknown
{
    virtual void __stdcall OpenStream(ILegacyP2PStream* stream, 
                                      IExtraMediaInfo* extraInfo) = 0;
    virtual void __stdcall OpenFile(const wchar_t* fileName,
                                    IExtraMediaInfo* extraInfo) = 0;
    virtual void __stdcall Play() = 0;
    virtual void __stdcall Pause() = 0;
    virtual void __stdcall Stop() = 0;
    virtual void __stdcall SetPosition(INT64 time) = 0;
    virtual INT64 __stdcall GetPosition() = 0;
    virtual void __stdcall SetVolume(int volume) = 0;
    virtual int __stdcall GetVolume() = 0;
    virtual void __stdcall SetMute(bool mute) = 0;
    virtual bool __stdcall GetMute() = 0;
    virtual void __stdcall SetBalance(int balance) = 0;
    virtual int __stdcall GetBalance() = 0;
    virtual void __stdcall GetAudioInfo(TAudioInfo* audioInfo) = 0;
    virtual HRESULT __stdcall StartRecording(const wchar_t* fileName,
                                             INT64 duration,
                                             IAsyncCallback* callback,
                                             int type, IUnknown* inst) = 0;
    virtual HRESULT __stdcall StopRecording(IUnknown* inst) = 0;
    virtual void __stdcall SelectAudioTrack(KAudioTrackType trackType) = 0;
};

//------------------------------------------------------------------------------
interface IDSPConfig : public IUnknown
{
    virtual void __stdcall SetPreamp(float preamp) = 0;
    virtual float __stdcall GetPreamp() = 0;
    virtual void __stdcall SetSound3D(int sound3d) = 0;
    virtual int __stdcall GetSound3D() = 0;
    virtual void __stdcall SetReplayGain(float replayGain) = 0;
    virtual void __stdcall SetWinampDSPPluginsInfo(const wchar_t* path,
                                                   HWND mainWindow) = 0;
    virtual void __stdcall GetPluginInfos(TPluginInfo* pluginInfos, 
                                          int* pluginCount) = 0;
    virtual int __stdcall GetWinampPluginPath(wchar_t* path, int buffLen) = 0;
    virtual void __stdcall EnableWinampDSPPlugin(int index) = 0;
    virtual void __stdcall ShowWinampDSPPluginConfigPanel(int index) = 0;
    virtual void __stdcall DisableWinampDSPPlugin(int index) = 0;
    virtual void __stdcall SetEqEnabled(bool enabled) = 0;
    virtual void __stdcall SetEqGain(int bandIndex, float bandGain) = 0;
    virtual void __stdcall SetSpectrumEnabled(bool enable) = 0;
    virtual void __stdcall SetWaveFormEnabled(bool enable) = 0;
};

//------------------------------------------------------------------------------
interface IPlayerConfig : public IUnknown
{
    // 当IDs为NULL时返回OutputType的总数目
    virtual int __stdcall GetOutputTypeIDList(int* IDs, int count) = 0;
    // name最大长度为_MAX_PATH(260)
    virtual int __stdcall GetOutputTypeNameList(int ID, wchar_t* name,
        int count) = 0;
    // 当info为NULL时返回Devices的总数目,如果outputTypeID有误则返回-1
    virtual int __stdcall GetOutputDevices(int outputTypeID, TDeviceInfo* info,
        int count) = 0;
    virtual void __stdcall GetCurrentOutput(int* typeID, int* deviceIndex) = 0;
    virtual bool __stdcall SetOutputDevice(int outputTypeID, 
        int deviceIndex) = 0;
    virtual void __stdcall SetOutputBits(int bits) = 0;
    virtual void __stdcall SetOutputFloat(bool isFloat) = 0;
    virtual void __stdcall SetOutputFadeEnabled(bool fadeEnabled) = 0;
    virtual void __stdcall SetFadingDuration(int fadeIn, int fadeOut) = 0;
    virtual void __stdcall SetProxy(const wchar_t* url, 
        int type,
        bool isuser) = 0;
    virtual void __stdcall SetDefaultVedioRenderer(CLSID& rendererId,
        int mode) = 0;
};