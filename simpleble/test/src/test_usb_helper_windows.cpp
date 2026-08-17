#include <gtest/gtest.h>

#include "backends/dongl/usb/UsbHelperWindowsUtils.h"

using namespace SimpleBLE::Dongl::USB;

TEST(UsbHelperWindowsUtilsTest, MatchesDonglHardwareIdTokensCaseInsensitively) {
    EXPECT_TRUE(Detail::hardware_id_matches_usb_device(L"USB\\VID_3918&PID_0001&MI_00", 0x3918, 0x0001));
    EXPECT_TRUE(Detail::hardware_id_matches_usb_device(L"usb\\vid_3918&pid_0001", 0x3918, 0x0001));
    EXPECT_FALSE(Detail::hardware_id_matches_usb_device(L"USB\\VID_3918&PID_0002", 0x3918, 0x0001));
    EXPECT_FALSE(Detail::hardware_id_matches_usb_device(L"USB\\VID_3918&PID_00010", 0x3918, 0x0001));
    EXPECT_FALSE(Detail::hardware_id_matches_usb_device(L"USB\\VID_1366&PID_1015", 0x3918, 0x0001));
}

TEST(UsbHelperWindowsUtilsTest, FormatsAllValidComPortNumbers) {
    EXPECT_EQ(Detail::windows_serial_path(L"COM4"), "\\\\.\\COM4");
    EXPECT_EQ(Detail::windows_serial_path(L"COM10"), "\\\\.\\COM10");
    EXPECT_EQ(Detail::windows_serial_path(L"com123"), "\\\\.\\COM123");
}

TEST(UsbHelperWindowsUtilsTest, RejectsInvalidComPortNames) {
    EXPECT_EQ(Detail::windows_serial_path(L"COM"), std::nullopt);
    EXPECT_EQ(Detail::windows_serial_path(L"COMA"), std::nullopt);
    EXPECT_EQ(Detail::windows_serial_path(L"ttyUSB0"), std::nullopt);
}
