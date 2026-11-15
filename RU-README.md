# 🇷🇺 Русская версия The Powder Toy

Этот репозиторий содержит **русифицированную сборку** The Powder Toy.
Оригинальный проект: https://github.com/ThePowderToy/The-Powder-Toy

<sub>Сборка на базе официальных исходников **v100.1 (build 400)**. Переведены интерфейс и все тексты игры на русский; физика, логика и геймплей не менялись.</sub>

![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Windows-lightgrey)

---

## ⬇️ Скачать

Готовые сборки — на странице **Releases**: https://github.com/vnsky03/The-Powder-Toy-Ru/releases/latest

| Файл | Для кого |
|---|---|
| `The-Powder-Toy-RU-windows-x64.zip` | Windows 10 и 11, 64-бит |
| `The-Powder-Toy-RU-macOS-universal.zip` | macOS: Apple Silicon (M1 и новее) и Intel |

Архивы автономные: все библиотеки уже внутри, ничего дополнительно устанавливать не нужно.

> Релизы собирает GitHub Actions (workflow `release`). Сборки отдельных коммитов — вкладка **Actions** → последний прогон → **Artifacts**.

---

## 🍏 Установка на macOS

1. Скачайте `The-Powder-Toy-RU-macOS-universal.zip` и распакуйте: получится папка с приложением **The Powder Toy RU**.
2. Перетащите **The Powder Toy RU** в папку **Программы** (необязательно).
3. Первый запуск. Приложение не подписано Apple, поэтому macOS не откроет его сразу. Это делается один раз:
   - macOS 15 и новее: откройте приложение, в окне с предупреждением нажмите «Готово», затем **Системные настройки → Конфиденциальность и безопасность → Безопасность → Всё равно открыть** и введите пароль (кнопка появляется примерно на час после попытки открыть приложение);
   - macOS 14 и старше: правый щелчок по приложению → **Открыть** → **Открыть**;
   - если не получилось: в Терминале выполните `xattr -dr com.apple.quarantine "/Applications/The Powder Toy RU.app"`.
4. Игра использует тот же профиль, что и оригинал (`~/Library/Application Support/The Powder Toy`): сохранения и настройки не пропадут. Оригинальная версия не удаляется.

**Вернуть английскую версию:** удалите **The Powder Toy RU** из папки «Программы», оригинал и ваши сохранения останутся на месте.

---

## 🪟 Установка на Windows

1. Скачайте `The-Powder-Toy-RU-windows-x64.zip` и распакуйте в любую папку (например, `C:\Games\PowderToy-RU`).
2. Запустите `ThePowderToy-RU.exe`. Это один файл: DLL и дополнительные программы не нужны.
3. Если Windows покажет «Windows защитила ваш компьютер» (SmartScreen): **Подробнее → Выполнить в любом случае**. Программа не подписана цифровой подписью, антивирус отключать не нужно.
4. При первом запуске игра предлагает «Установить The Powder Toy» (кнопка Confirm). Это необязательно: можно нажать «Отмена». Если нажать Confirm, игра добавит ярлык в меню «Пуск» и свяжет файлы `.cps`, `.stm` и ссылки `ptsave:` с этой копией игры.

Игра использует общий профиль The Powder Toy (как и оригинальная версия): ваши сохранения остаются на месте. Оригинальный The Powder Toy устанавливать не нужно.

**Удаление:** удалите папку с игрой (и ярлык «The Powder Toy» из меню «Пуск», если нажимали Confirm). Сохранения не затрагиваются.

---

## 🛠 Что переведено

- HUD и главное меню;
- все окна: настройки, вход в аккаунт, браузеры сохранений, профиль, поиск, рендер, публикация, просмотр;
- все диалоги, информационные сообщения и тексты ошибок;
- 19 стен, категории меню, 11 служебных инструментов;
- **195 элементов** (имена и описания);
- шрифт расширен типографикой: `– — … • №`.

---

## 🧑‍💻 Сборка из исходников (для разработчиков)

**Windows (MSYS2 / UCRT64):**

```sh
pacman -S --noconfirm --needed mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-meson mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-bzip2 \
  mingw-w64-ucrt-x86_64-jsoncpp mingw-w64-ucrt-x86_64-curl \
  mingw-w64-ucrt-x86_64-SDL2 mingw-w64-ucrt-x86_64-libpng \
  mingw-w64-ucrt-x86_64-fftw
git clone --depth 1 https://github.com/vnsky03/The-Powder-Toy-Ru
cd The-Powder-Toy-Ru
meson setup build -Dlua=none
ninja -C build
```

**macOS:** нужны SDL2, libcurl, libpng, bzip2, FFTW3f, jsoncpp и компилятор с C++20 (например, clang ≥ 16):

```sh
CC=/opt/local/bin/clang-mp-17 CXX=/opt/local/bin/clang++-mp-17 \
OBJCXX=/opt/local/bin/clang++-mp-17 \
meson setup build -Dlua=none
ninja -C build
```

> Lua-консоль в релизных сборках включена, как в оригинале. В сборках по инструкциям выше (`-Dlua=none`) она отключена; чтобы получить её, соберите проект так же, как релиз: см. `.github/build.sh` и `.github/workflows/release.yaml`.

---

## 📄 Лицензия

GPLv2 (как оригинальный проект). Права на The Powder Toy принадлежат его авторам; этот репозиторий — локализация.
