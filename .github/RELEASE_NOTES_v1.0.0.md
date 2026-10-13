# NovaPad v1.0.0 — Release Notes

## English

NovaPad 1.0.0 is the first stable release of the C++17 / Qt 6 tabbed text editor.

### Highlights
- Added automatic bracket and quote completion, overtyping existing closers, deleting empty pairs together, and wrapping selected text.
- Added smart Home / Shift+Home, Ctrl+Backspace / Ctrl+Delete word deletion, and brace-aware Enter indentation.
- Added matching-bracket highlighting and line selection by clicking a line number.
- Added View → Full Screen (`F11`) and a persistent **Auto-close Brackets & Quotes** preference.
- Added `--version` / `-v` command-line output and expanded the self-test suite to 55 checks.

### Fixes and verification
- Fixed a use-after-free during window teardown with multiple tabs by disconnecting editor/document handlers and clearing tab records before destroying highlighters and editors.
- Added `--stress` regression coverage for repeated tab creation, editing, closing, and multi-tab teardown.
- CI builds and tests the release on Linux, Windows, and macOS.

## فارسی

نسخهٔ پایدار **NovaPad 1.0.0**؛ ویرایشگر متن تب‌دار با ++C17 و Qt 6.

### قابلیت‌های تازه
- تکمیل خودکار پرانتز، براکت، آکولاد و نقل‌قول؛ عبور از براکت پایانی موجود، حذف یک‌جای جفت خالی و قراردادن متن انتخاب‌شده داخل جفت.
- Home و Shift+Home هوشمند، حذف واژه با Ctrl+Backspace و Ctrl+Delete و تورفتگی مناسب هنگام رفتن به خط بعد پس از آکولاد.
- هایلایت براکت‌های متناظر و انتخاب خط با کلیک روی شمارهٔ آن.
- حالت تمام‌صفحه از مسیر View با میانبر F11 و تنظیم ماندگار «Auto-close Brackets & Quotes».
- نمایش نسخه با `--version` یا `-v` و افزایش مجموعهٔ خودآزمایی به ۵۵ بررسی.

### رفع باگ و آزمون
- رفع دسترسی پس از آزادسازی حافظه هنگام بستن پنجره‌ای با چند تب؛ اتصال‌های سند و ویرایشگر پیش از آزادسازی قطع و فهرست تب‌ها خالی می‌شود.
- افزودن آزمون تنش `--stress` برای ساخت، ویرایش و بستن مکرر تب‌ها و بررسی پاک‌سازی چندتب.
- ساخت و آزمون CI برای لینوکس، ویندوز و macOS.
