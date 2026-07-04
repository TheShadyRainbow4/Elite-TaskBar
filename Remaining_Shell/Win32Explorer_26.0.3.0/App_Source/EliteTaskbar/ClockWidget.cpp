#include "stdafx.h"
#pragma warning(disable: 4100 4244 4267 4189 4312)
#include "ClockWidget.h"
#include <gdiplus.h>
#include <string>

#pragma comment (lib,"Gdiplus.lib")
using namespace Gdiplus;

void ClockWidget::Draw(HDC hdc, RECT rect) {
    Graphics graphics(hdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetTextRenderingHint(TextRenderingHintAntiAlias); // ClearType destroys DWM Glass Alpha!

    SYSTEMTIME st;
    GetLocalTime(&st);

    // Format time: h:mm tt
    wchar_t timeBuf[32];
    GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, timeBuf, 32);

    // Format date: M/d/yyyy
    wchar_t dateBuf[32];
    GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, dateBuf, 32);

    FontFamily fontFamily(L"Segoe UI");
    Font fontTime(&fontFamily, 10, FontStyleRegular, UnitPoint);
    Font fontDate(&fontFamily, 9, FontStyleRegular, UnitPoint);
    SolidBrush shadowBrush(Color(200, 0, 0, 0)); // Dark shadow
    SolidBrush brush(Color(255, 255, 255, 255)); // White text

    StringFormat format;
    format.SetAlignment(StringAlignmentCenter);
    format.SetLineAlignment(StringAlignmentCenter);

    RectF rectTime(0, 0, (REAL)rect.right, (REAL)rect.bottom / 2.0f);
    RectF rectDate(0, (REAL)rect.bottom / 2.0f, (REAL)rect.right, (REAL)rect.bottom / 2.0f);

    RectF shadowRectTime(1, 1, (REAL)rect.right + 1, (REAL)rect.bottom / 2.0f + 1);
    RectF shadowRectDate(1, (REAL)rect.bottom / 2.0f + 1, (REAL)rect.right + 1, (REAL)rect.bottom / 2.0f + 1);

    // Draw Shadows
    graphics.DrawString(timeBuf, -1, &fontTime, shadowRectTime, &format, &shadowBrush);
    graphics.DrawString(dateBuf, -1, &fontDate, shadowRectDate, &format, &shadowBrush);

    // Draw Main Text
    graphics.DrawString(timeBuf, -1, &fontTime, rectTime, &format, &brush);
    graphics.DrawString(dateBuf, -1, &fontDate, rectDate, &format, &brush);
}



