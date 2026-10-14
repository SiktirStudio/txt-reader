# NovaPad v1.1.0 — Release Notes

## English

### New
- Added a dockable **AI Assistant** (View → AI Assistant, `Ctrl+Shift+A`) for
  OpenAI-compatible Chat Completions and Anthropic Messages APIs.
- Select provider, trusted base URL and an editable model ID. Explicitly add
  selected editor code to the prompt; no document content is sent automatically.
- API keys are masked and kept only in memory. Only provider/base URL/model
  preferences are saved. Changing provider or base URL clears the key.
- Added four-column, tab-aware **Indentation Guides**, with a persistent View
  preference applied to current and new tabs and colors for both themes.
- The application now starts maximized and restores its previous normal or
  maximized mode after leaving full screen, including menu/state synchronization.
- Replaced the application icon with a crisp white-background design in PNG,
  multi-resolution ICO and macOS ICNS formats.

### Security, packaging and verification
- Added the Qt Network dependency. AI requests require HTTPS except for literal
  loopback/localhost; URL credentials/queries/fragments and header injection are
  rejected. TLS certificate verification is mandatory and redirects are refused.
- Added cancellation, a 60-second deadline, prompt/response size bounds, plain-text
  responses and redacted server errors. No automatic code execution or editing.
- Linux bundles now include TLS plugins, OpenSSL 3, required Qt libraries, a
  launcher and Qt plugin configuration. Windows/macOS deployment checks require
  a working TLS backend too. Third-party license notices accompany the bundles.
- CI runs self-test, stress, TLS and version checks on **all three platforms**
  and repeats smoke tests on deployed packages. Tests use temporary settings and
  offline mock API servers, never real credentials or paid API calls.
- Release publication waits for the full platform matrix and requires all three
  packages. Added AI/security and verification documentation.

### Known limitations
- AI is text-only, single-turn and non-streaming; suggested model IDs must be
  available to your account. Live provider calls are not part of automated tests.
- Offscreen checks do not replace real desktop or clean-machine GUI verification.
- Windows/macOS builds are unsigned/unnotarized. Linux targets Ubuntu 24.04 or
  compatible newer systems and still needs system desktop libraries and CA roots.

## فارسی

### قابلیت‌های تازه
- افزودن پنل **AI Assistant** از مسیر View و میانبر `Ctrl+Shift+A` برای APIهای
  OpenAI-compatible و Anthropic، با انتخاب Provider، نشانی پایه و مدل دلخواه.
- افزودن صریح کد انتخاب‌شده به پرسش؛ هیچ فایل یا متنی خودکار ارسال نمی‌شود.
- کلید API مخفی و فقط در حافظه نگه‌داری می‌شود؛ تنها انتخاب Provider/Base URL/Model
  ذخیره می‌شود. با تغییر Provider یا نشانی، کلید پاک می‌شود.
- افزودن **Indentation Guides** چهارستونه و آگاه از Tab، با تنظیم ماندگار View
  برای تب‌های موجود و تازه و پشتیبانی از هر دو تم.
- شروع برنامه به‌شکل maximized و بازگشت به حالت عادی یا maximized قبلی پس از
  خروج از Full Screen؛ هماهنگی گزینهٔ منو با وضعیت واقعی پنجره.
- آیکون تازه با پس‌زمینهٔ سفید در قالب PNG، ICO چنداندازه‌ای و ICNS مک.

### امنیت، بسته‌بندی و آزمون
- افزودن Qt Network؛ اجبار HTTPS به‌جز localhost و IPهای loopback، بررسی گواهی
  TLS، ردکردن redirect و نشانی دارای اطلاعات ورود، query یا fragment.
- لغو درخواست، مهلت ۶۰ثانیه‌ای، محدودیت حجم پرسش/پاسخ و پاسخ متن ساده؛ بدون
  اجرای کد یا ویرایش خودکار. متن خطای سرور برای جلوگیری از افشای کلید نمایش داده نمی‌شود.
- بستهٔ لینوکس شامل پلاگین‌های TLS، OpenSSL 3، کتابخانه‌های لازم Qt و launcher
  است. وجود TLS در بسته‌های ویندوز و مک نیز آزموده می‌شود.
- اجرای selftest، stress، بررسی TLS و نسخه در CI هر سه سیستم‌عامل و تکرار
  آزمون‌ها روی برنامهٔ بسته‌بندی‌شده؛ آزمون‌های AI کاملاً آفلاین با کلید ساختگی.
- انتشار پس از موفقیت کل ماتریس و با هر سه بسته انجام می‌شود؛ مستندات امنیت،
  استفاده از AI و چک‌لیست آزمون به‌روز شده‌اند.

### محدودیت‌ها
- AI فعلاً متنی، تک‌مرحله‌ای و بدون streaming است؛ اعتبار مدل به حساب کاربر
  بستگی دارد. API پولی واقعی در آزمون خودکار فراخوانی نمی‌شود.
- آزمون offscreen جای بررسی دسکتاپ واقعی یا سیستم پاک را نمی‌گیرد.
- بسته‌های ویندوز و مک امضاشده/notarized نیستند. لینوکس برای Ubuntu 24.04 یا
  سیستم سازگار جدیدتر است و به کتابخانه‌های دسکتاپ و گواهی‌های ریشهٔ سیستم نیاز دارد.
