#ifndef RTC8563_H
  #define RTC8563_H

#define RTC8563       0xA2

#define RTC_CTRL1     0x00
  #define rtcSTART    0x00
  #define rtcSTOP     0x20

#define RTC_CTRL2     0x01
  #define rtcTIE      0x01
  #define rtcAIE      0x02
  #define rtcTF       0x04
  #define rtcAF       0x08
  #define rtcTITP     0x01

#define RTC_SEC       0x02
#define RTC_MIN       0x03
#define RTC_HOUR      0x04
#define RTC_DAY       0x05
#define RTC_WDAY      0x06
#define RTC_MONTH_C   0x07
#define RTC_YEAR      0x08
#define RTC_ALMIN     0x09
#define RTC_ALHOUR    0x0a
#define RTC_ALDAY     0x0b
#define RTC_ALWDAY    0x0c
#define RTC_CLKOUT    0x0d
#define RTC_TIMCTRL   0x0e
#define RTC_TIMER     0x0f

	typedef struct {
  	int year,month,day,weekday;              // date
	  int hours,minutes,seconds,hundredths;    // time
	} RTC;

	RTC rtc;
  RTC rtc_set;

	void ShowDateTime(void);
	void GetRTC(void);
  void PutRTC(void);
  void rtc_init(void);

#endif


