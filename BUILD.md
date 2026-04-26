# Инструкция по сборке (Visual Studio C++)

Этот проект оптимизирован для компиляции в среде Microsoft Visual Studio 2022.

## Настройка проекта

### 1. Поддержка Ассемблера (КРИТИЧНО)
Проект содержит файлы `.asm` (`syscalls.asm` и `svm_launch.asm`), которые реализуют системные вызовы и вход в режим гипервизора.
**Обязательно следуйте шагам в [ASM_GUIDE.md](ASM_GUIDE.md)**, чтобы включить поддержку MASM в Visual Studio.

### 2. Свойства проекта (x64 Release)
- **C++ Standard**: ISO C++17 или C++20.
- **Optimization**: /O2 (Maximize Speed).
- **Runtime Library**: Multi-threaded (/MT) — для работы без зависимостей от DLL студии.
- **Security Check**: Disable Security Check (/GS-) — важно для шеллкодов и драйверного кода.
- **Linker Input**: Добавьте `ntoskrnl.lib` для сборки драйвера и `Crypt32.lib` для лоадера.

### 3. Сборка Драйвера
Драйвер `driver.c` должен собираться с использованием **Windows Driver Kit (WDK)**. Он служит "мостиком" для запуска гипервизора на всех ядрах.

## Процесс сборки
1. Скомпилируйте `refactored_logic.cpp` как DLL (это ядро чита).
2. Зашифруйте DLL: `python encrypt_and_pack.py core.dll packed_payload.bin`.
3. Убедитесь, что `packed_payload.bin` прописан в `resources.rc`.
4. Соберите итоговый `refactored_loader.exe` (включая все .asm и .cpp файлы).
5. Соберите `driver.sys`.

## Использование
1. Загрузите `driver.sys` через KDMapper или аналогичный маппер.
2. Запустите `refactored_loader.exe`.
3. Дождитесь запуска DayZ.
