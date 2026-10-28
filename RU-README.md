# 🇷🇺 Русская версия The Powder Toy

Этот репозиторий содержит **русифицированную сборку** The Powder Toy.
Оригинальный проект: https://github.com/ThePowderToy/The-Powder-Toy

<sub>Сборка на базе официальных исходников **v100.1 (build 400)**. Переведены интерфейс и все тексты игры на русский; физика, логика и геймплей не менялись.</sub>

![Platform](https://img.shields.io/badge/platform-macOS%20%7C%20Windows-lightgrey)

---

## ⬇️ Скачать

Готовые сборки — на странице **Releases**: https://github.com/vnsky03/The-Powder-Toy-Ru/releases

| Файл | Для кого |
|---|---|
| `The-Powder-Toy-Ru-v100.1-macOS.zip` | macOS |
| `ThePowderToy-RU-windows.zip` | Windows (64-бит) |

> Свежие Windows-сборки: вкладка **Actions** → workflow `build-windows` → последний прогон → **Artifacts**.

---

## 🍏 Установка на macOS

1. Скачайте `The-Powder-Toy-Ru-v100.1-macOS.zip` и распакуйте.
2. Перетащите **The Powder Toy RU.app** в папку **Программы**.
3. Запустите. Если macOS попросит подтверждение — кликните **правой кнопкой** по приложению → **Открыть** → **Открыть**.
4. Игра использует тот же профиль, что и оригинал (`~/Library/Application Support/The Powder Toy`) — сохранения и настройки не пропадут. Оригинальная версия не удаляется.

**Вернуть английскую версию:**

```sh
rm -rf "/Applications/The Powder Toy RU.app" "/Applications/The Powder Toy RU.bak"
```

---

## 🪟 Установка на Windows

Версия: v100.1 (build 400), русифицированная сборка.

```
УСТАНОВКА
---------
1. Скачайте архив ThePowderToy-RU-windows.zip.
2. Распакуйте его в любую папку (например, C:\Games\PowderToy-RU).
   ВАЖНО: не удаляйте файлы .dll рядом с ThePowderToy-RU.exe —
   игра без них не запустится.
3. Запустите ThePowderToy-RU.exe двойным щелчком.

Если Windows покажет «SmartScreen» / «Windows защитил ваш компьютер»:
   - нажмите «Подробнее»,
   - затем «Выполнить в любом случае».
   (Программа не подписана цифровой подписью, поэтому Windows
   перестраховывается; антивирус можно не отключать.)

Чтобы было удобнее:
   - кликните по ThePowderToy-RU.exe правой кнопкой →
     «Отправить» → «Ярлык на рабочий стол».

СОХРАНЕНИЯ И НАСТРОЙКИ
----------------------
Игра использует общий профиль The Powder Toy (как и оригинальная версия).
Ваши локальные сохранения останутся на месте и будут доступны
в русской версии. Оригинальный The Powder Toy устанавливать не нужно.

УДАЛЕНИЕ (ОТКАТ)
----------------
Просто удалите папку, в которую распаковали архив.
Никаких следов в системе не остаётся, сохранения не затрагиваются.
```

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

> Lua-консоль в сборках отключена (`-Dlua=none`). Кто хочет Lua — поставьте LuaJIT и пересоберите без этой опции.

---

## 📄 Лицензия

GPLv2 (как оригинальный проект). Права на The Powder Toy принадлежат его авторам; этот репозиторий — локализация.
