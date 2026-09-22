#ifndef davecc_Availability_h
#define davecc_Availability_h

#define __MAC_10_0 1000
#define __MAC_10_1 1010
#define __MAC_10_2 1020
#define __MAC_10_3 1030
#define __MAC_10_4 1040
#define __MAC_10_5 1050
#define __MAC_10_6 1060
#define __MAC_10_7 1070
#define __MAC_10_8 1080
#define __MAC_10_9 1090
#define __MAC_10_10 101000
#define __MAC_10_11 101100
#define __MAC_10_12 101200
#define __MAC_10_13 101300
#define __MAC_10_14 101400
#define __MAC_10_15 101500
#define __MAC_11_0 110000
#define __MAC_12_0 120000
#define __MAC_13_0 130000
#define __MAC_14_0 140000
#define __MAC_15_0 150000

#define __IPHONE_2_0 20000
#define __IPHONE_3_0 30000
#define __IPHONE_4_0 40000
#define __IPHONE_5_0 50000
#define __IPHONE_6_0 60000
#define __IPHONE_7_0 70000
#define __IPHONE_8_0 80000
#define __IPHONE_9_0 90000
#define __IPHONE_10_0 100000
#define __IPHONE_11_0 110000
#define __IPHONE_12_0 120000
#define __IPHONE_13_0 130000
#define __IPHONE_14_0 140000
#define __IPHONE_15_0 150000
#define __IPHONE_16_0 160000
#define __IPHONE_17_0 170000
#define __IPHONE_18_0 180000

#define __MAC_OS_X_VERSION_MIN_REQUIRED 140000
#define __MAC_OS_X_VERSION_MAX_ALLOWED 150000
#define __IPHONE_OS_VERSION_MIN_REQUIRED 0
#define __IPHONE_OS_VERSION_MAX_ALLOWED 0

#define API_AVAILABLE(...)
#define API_DEPRECATED(...)
#define API_DEPRECATED_WITH_REPLACEMENT(...)
#define API_UNAVAILABLE(...)
#define API_UNAVAILABLE_BEGIN
#define API_UNAVAILABLE_END
#define API_AVAILABLE_BEGIN(...)
#define API_AVAILABLE_END

#define AVAILABLE_MAC_OS_X_VERSION_10_0_AND_LATER
#define DEPRECATED_IN_MAC_OS_X_VERSION_10_0_AND_LATER

#ifndef __OSX_AVAILABLE_STARTING
#define __OSX_AVAILABLE_STARTING(_osx, _ios)
#endif
#ifndef __OSX_AVAILABLE_BUT_DEPRECATED
#define __OSX_AVAILABLE_BUT_DEPRECATED(_osxIntro, _osxDep, _iosIntro, _iosDep)
#endif

#endif
