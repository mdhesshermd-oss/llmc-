# Руководство по сборке в Visual Studio (C++) - Advanced Edition

## Подготовка
1. Установите **Windows SDK** и **Visual Studio 2022**.
2. Скачайте библиотеку **ImGui** (требуются файлы `imgui/`).
3. Добавьте в проект файлы `.cpp`, `.h` и `.asm`.

## Настройка MASM (Ассемблер)
Для корректной сборки системных вызовов:
1. Правый клик по проекту -> **Build Dependencies** -> **Build Customizations**.
2. Поставьте галочку на **masm**.
3. Правый клик на файл `syscalls.asm` -> **Properties**.
4. Убедитесь, что **Item Type** установлен в **Microsoft Macro Assembler**.

## Настройка компилятора
- **Configuration:** Release | x64
- **C++ Standard:** C++17 or C++20
- **Optimizations:** /O2
- **Instruction Set:** /arch:AVX2
- **Additional Include Directories:** Путь к папке с ImGui.

## Этапы сборки
1. Скомпилируйте `refactored_logic.cpp` как DLL.
2. Запакуйте её: `python encrypt_and_pack.py cheat.dll packed_payload.bin`.
3. Скомпилируйте `refactored_loader.cpp` + `syscalls.asm` + `resources.rc` в итоговый EXE.
