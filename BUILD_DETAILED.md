# Полная инструкция по сборке проекта DayZ Stealth

Этот гид поможет вам собрать проект в один готовый исполняемый файл (`loader.exe`), содержащий внутри зашифрованный чит.

## 🛠 Требования
1. **Visual Studio 2022** (с установленным компонентом "Desktop development with C++").
2. **Windows Driver Kit (WDK)** — необходим для компиляции `driver.cpp`.
3. **Python 3.10+** — для работы скрипта шифрования.
4. **MASM (Microsoft Macro Assembler)** — встроен в Visual Studio, но требует активации в проекте.

---

## Шаг 1: Подготовка полезной нагрузки (DLL)
Ваш чит должен быть скомпилирован как динамическая библиотека.

1. Создайте проект **C++ DLL** в Visual Studio.
2. Добавьте в него файл `refactored_logic.cpp` и необходимые заголовочные файлы (`hypervisor_io.h`, `overlay_hijack.h` и т.д.).
3. В настройках проекта (Properties):
   - **Configuration:** Release | **Platform:** x64
   - **C++ Standard:** C++17
   - **Runtime Library:** Multi-threaded (/MT)
4. Скомпилируйте проект. Вы получите файл `core.dll`.

---

## Шаг 2: Шифрование полезной нагрузки
Чтобы античит не нашел DLL внутри вашего EXE, её нужно зашифровать.

1. Положите скомпилированный `core.dll` в папку со скриптом `encrypt_and_pack.py`.
2. Откройте терминал и выполните команду:
   ```bash
   python encrypt_and_pack.py core.dll packed_payload.bin
   ```
   *Скрипт создаст файл `packed_payload.bin` (зашифрованный AES-128 CBC).*

---

## Шаг 3: Настройка финального проекта (EXE)
Теперь соберем лоадер, который запустит гипервизор и внедрит чит.

1. Создайте проект **C++ Console Application** в Visual Studio.
2. Добавьте файлы:
   - `refactored_loader.cpp`, `hv_vmm.cpp`, `driver.cpp`, `manual_map.h` и др.
   - **Ассемблер:** `syscalls.asm`, `svm_bridge.asm`.
   - **Ресурсы:** `resources.rc`.
3. **Активация Ассемблера (КРИТИЧНО):**
   - Нажмите правой кнопкой на проект -> **Build Dependencies** -> **Build Customizations**.
   - Поставьте галочку на **masm**.
   - Нажмите правой кнопкой на файлы `.asm` -> **Properties** -> **Item Type** -> **Microsoft Macro Assembler**.
4. **Настройка библиотек:**
   - В свойствах проекта -> **Linker** -> **Input** -> **Additional Dependencies** добавьте:
     `Crypt32.lib`
     `ntoskrnl.lib` (для драйверной части)

---

## Шаг 4: Сборка и Итог
1. Убедитесь, что файл `packed_payload.bin` находится в той же папке, где и `resources.rc`.
2. Установите режим **Release | x64**.
3. Нажмите **Build Solution**.

**Результат:**
Вы получите один файл `refactored_loader.exe`. При запуске он:
1. Свяжется с драйвером для запуска гипервизора на всех ядрах.
2. Расшифрует встроенную DLL в памяти.
3. Найдет DayZ и внедрит в него чит через гипервизор и Thread Hijacking.
4. Скроет свое присутствие и удалит следы из системы.

## ⚠️ Важное замечание
Для корректной работы драйвера (`driver.cpp`) он должен иметь цифровую подпись или быть загружен через **KDMapper**.
