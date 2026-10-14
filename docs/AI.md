# AI Assistant (NovaPad 1.1.0)

Open **View → AI Assistant** (`Ctrl+Shift+A`). The dock can be moved to either
side or floated. No network request is made by opening the dock, changing a
setting, selecting code, or adding a selection.

1. Select **OpenAI-compatible** or **Anthropic**.
2. Enter a trusted **Base URL**, including the API version path if required.
   Defaults are `https://api.openai.com/v1` and `https://api.anthropic.com/v1`.
3. Select a suggested model or type the exact model ID available to your account.
   Suggestions are examples, not an availability guarantee.
4. Enter your API key in the masked field. The key is **never written to
   QSettings or a NovaPad file**. **Clear key** removes it from the field.
5. Write a prompt. Optionally select text in the editor and click **Add selected
   code**. Only that selection is copied into the prompt; the whole document,
   file path and other tabs are not included automatically.
6. Inspect the prompt, then click **Send**. **Cancel** aborts the request.
   The response is displayed as plain text and never executes code or fetches
   HTML/images. Copy and review any output before applying it to your document.

Provider, base URL and model preferences are remembered separately for each
provider. Keys are not restored after closing the window. Changing provider or
editing the base URL clears the key to avoid sending it to a different endpoint
accidentally. Prompts, responses and conversation history are not persisted.
The dock is initially hidden. **Clear text** clears its prompt and response;
it does not clear the key.

## Protocol and security

- OpenAI-compatible requests use `POST <base>/chat/completions`, a Bearer token,
  and a Chat Completions `messages` body. Anthropic uses `POST <base>/messages`,
  `x-api-key`, `anthropic-version: 2023-06-01`, and `max_tokens: 4096`.
  A base URL already ending in the appropriate endpoint is also accepted.
- HTTPS is mandatory except for **localhost** and literal loopback addresses,
  such as `127.0.0.1` or `[::1]`. Private LAN addresses are not exempt. A local
  server may omit the API key. Example: `http://127.0.0.1:8080/v1`.
- URLs containing embedded credentials, queries or fragments are rejected.
- TLS requires certificate/hostname verification and TLS 1.2 or newer. There is
  no insecure mode and TLS errors are never ignored. The operating system's CA
  trust store is used; custom CAs must be configured there, not bypassed here.
- Redirects are **not followed**, including same-origin redirects. Configure
  the final, trusted endpoint explicitly. Cookies, automatic credential reuse
  and disk caching are disabled.
- Requests are non-streaming, with a 60-second total deadline, a 1 MiB UTF-8
  prompt limit and a 4 MiB response limit. Server error bodies are not shown or
  logged because they can contain credentials or prompt text.
- Keys and request data necessarily exist in process/network-library memory.
  This is not a guarantee against OS swap, crash dumps, a compromised machine,
  or a malicious endpoint. Do not send secrets in prompts. The chosen provider
  receives the prompt and has its own retention/billing policy.

## Limitations

This release supports text-only, single-turn responses. It does not provide
streaming, tools, attachments, automatic model discovery, the OpenAI Responses
API, authentication refresh, or automatic editing/execution of model output.
Custom services must implement the selected provider's request/response format.
No live, paid provider call is part of CI; offline mock servers test both
protocols, headers, cancellation, redirect refusal, error handling and limits.

## فارسی

از **View → AI Assistant** با میانبر `Ctrl+Shift+A` پنل را باز کنید. Provider،
نشانی پایه و شناسهٔ مدل را انتخاب کنید و کلید را در فیلد مخفی وارد کنید.
کلید، پرسش و پاسخ فقط در حافظه می‌مانند و در QSettings ذخیره نمی‌شوند؛
فقط انتخاب Provider/Base URL/Model ذخیره می‌شود. با تغییر Provider یا نشانی،
کلید پاک می‌شود. در هر اجرای تازه باید کلید را دوباره وارد کنید.

با **Add selected code** فقط متن انتخاب‌شده به پرسش اضافه می‌شود؛ تا وقتی
**Send** را نزنید چیزی ارسال نمی‌شود. پاسخ متن ساده است و خودکار در فایل
اعمال یا اجرا نمی‌شود. HTTPS اجباری است، مگر برای localhost و IPهای loopback.
گواهی TLS بررسی می‌شود و redirect دنبال نمی‌شود. از سرویس مورداعتماد استفاده
کنید و اطلاعات محرمانه را در پرسش نگذارید. آزمون‌ها آفلاین‌اند و API پولی
واقعی بدون کلید کاربر آزمایش نشده است.
