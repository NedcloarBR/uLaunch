
#pragma once
#include <ul/ul_Result.hpp>

namespace ul::system {

    enum class AppletMessage : u32 {
        None = 0,
        ChangeIntoForeground = 1,
        ChangeIntoBackground = 2,
        Exit = 4,
        ApplicationExited = 6,
        FocusStateChanged = 15,
        Resume = 16,
        DetectShortPressingHomeButton = 20,
        DetectLongPressingHomeButton = 21,
        DetectShortPressingPowerButton = 22,
        DetectMiddlePressingPowerButton = 23,
        DetectLongPressingPowerButton = 24,
        RequestToPrepareSleep = 25,
        FinishedSleepSequence = 26,
        SleepRequiredByHighTemperature = 27,
        SleepRequiredByLowBattery = 28,
        AutoPowerDown = 29,
        OperationModeChanged = 30,
        PerformanceModeChanged = 31,
        DetectReceivingCecSystemStandby = 32,
        SdCardRemoved = 33,
        LaunchApplicationRequested = 34,
        RequestToDisplay = 35,
        ShowApplicationLogo = 55,
        HideApplicationLogo = 56,
        ForceHideApplicationLogo = 57,
        FloatingApplicationDetected = 60,
        DetectShortPressingCaptureButton = 90,
        AlbumScreenShotTaken = 92,
        AlbumRecordingSaved = 93
    };

    // TODO (low priority): official general channel message names? (unofficial names are preceded by Unk_*)
    enum class GeneralChannelMessage : u32 {
        Unk_Invalid,
        RequestHomeMenu = 2,
        Unk_Sleep = 3,
        Unk_Shutdown = 5,
        Unk_Reboot = 6,
        RequestJumpToSystemUpdate = 11,
        Unk_OverlayBrightValueChanged = 13,
        Unk_OverlayAutoBrightnessChanged = 14,
        Unk_OverlayAirplaneModeChanged = 15,
        Unk_OverlayShown = 16,
        Unk_OverlayHidden = 17,
        RequestToLaunchApplication = 32,
        RequestJumpToStory = 33
    };

    struct SystemAppletMessageHeader {
        static constexpr u32 Magic = 0x534D4153; // "SAMS" -> System applet message...?

        u32 magic;
        u32 unk;
        GeneralChannelMessage msg;
        u32 unk_2;

        inline constexpr bool IsValid() {
            return this->magic == Magic;
        }

        static inline constexpr SystemAppletMessageHeader Create(const GeneralChannelMessage msg) {
            return {
                .magic = Magic,
                .unk = 1,
                .msg = msg,
                .unk_2 = 1
            };
        }
    };
    static_assert(sizeof(SystemAppletMessageHeader) == 0x10);

    Result PushSimpleSystemAppletMessage(const GeneralChannelMessage msg);

    struct StorageReader {
        AppletStorage st;
        u64 offset;

        StorageReader(AppletStorage st) : st(st), offset(0) {}

        inline size_t GetSize() {
            s64 tmp_size;
            UL_RC_ASSERT(appletStorageGetSize(&this->st, &tmp_size));
            return static_cast<size_t>(tmp_size);
        }

        inline Result ReadBuffer(void *out_buf, const size_t buf_size) {
            UL_RC_TRY(appletStorageRead(&this->st, this->offset, out_buf, buf_size));
            this->offset += buf_size;
            return 0;
        }
        
        template<typename T>
        inline Result Read(T &out_t) {
            return this->ReadBuffer(&out_t, sizeof(out_t));
        }
    };

}
