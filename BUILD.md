# Инструкция по сборке в Visual Studio (C++)

Этот проект оптимизирован для компиляции в среде Microsoft Visual Studio 2019/2022.

## Настройка проекта

### 1. Поддержка Ассемблера (ВАЖНО)
Для корректной работы системных вызовов необходимо настроить файл `syscalls.asm`.
**Обязательно следуйте шагам в файле [ASM_GUIDE.md](ASM_GUIDE.md)** перед началом сборки.

### 2. Свойства проекта (Properties)
- **Configuration:** Release
- **Platform:** x64
- **C++ Standard:** C++17 or C++20
- **Optimizations:** /O2 (Maximize Speed)
- **Instruction Set:** /arch:AVX2

### 3. Компиляция ресурсов
Убедитесь, что `resources.rc` включен в проект. Он автоматически вшьет зашифрованную DLL в ваш EXE.

## Процесс сборки
1. Скомпилируйте `refactored_logic.cpp` как DLL.
2. Запакуйте: `python encrypt_and_pack.py core.dll packed_payload.bin`.
3. Соберите `refactored_loader.cpp` + `syscalls.asm` + `resources.rc` в итоговый `loader.exe`.
