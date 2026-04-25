# Инструкция по сборке в Visual Studio (C++)

Этот проект оптимизирован для компиляции в среде Microsoft Visual Studio 2019/2022.

## Настройка проекта

### 1. Создание решения
1. Создайте новый проект **Empty Project (C++)**.
2. Добавьте все `.cpp` и `.h` файлы в проект через Solution Explorer.

### 2. Свойства проекта (Properties)
Для корректной работы и защиты установите следующие настройки:
- **Configuration:** Release
- **Platform:** x64
- **C++ Language Standard:** ISO C++17 Standard (`/std:c++17`)
- **Optimization:** Maximize Speed (`/O2`)
- **Instruction Set:** Advanced Vector Extensions 2 (`/arch:AVX2`) — критично для SIMD кода.
- **Character Set:** Use Multi-Byte Character Set (для совместимости с `FindWindowA`).

### 3. Компиляция ресурсов
Файл `resources.rc` должен быть включен в проект. Visual Studio автоматически вызовет `rc.exe` при сборке, если файл добавлен в раздел "Resource Files". Убедитесь, что `packed_payload.bin` находится в той же папке.

## Процесс сборки (3 этапа)

1. **Сборка DLL (Core):**
   - Сначала скомпилируйте `refactored_logic.cpp` как динамическую библиотеку (`.dll`).
2. **Упаковка:**
   - Выполните `python encrypt_and_pack.py core.dll packed_payload.bin`.
3. **Сборка EXE (Loader):**
   - Скомпилируйте `refactored_loader.cpp` вместе с ресурсами для получения финального `loader.exe`.

## Совет по разработке
Используйте **Pre-compiled Headers** или разделите проект на два разных проекта в одном Solution: один для DLL, другой для Loader, чтобы автоматизировать процесс упаковки через "Post-Build Events".
