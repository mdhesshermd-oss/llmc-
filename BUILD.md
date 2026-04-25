# Инструкция по сборке Single-File проекта

Этот процесс позволяет собрать лоадер и зашифрованную DLL в один исполняемый файл (`loader.exe`).

## Шаг 1: Сборка основной библиотеки
```bash
cl.exe /LD /O2 /arch:AVX2 refactored_logic.c /Fe:cheat_core.dll
```

## Шаг 2: Упаковка DLL в бинарный файл
Скрипт сожмет DLL и добавит заголовок с размером оригинала.
```bash
python encrypt_and_pack.py cheat_core.dll packed_payload.bin
```

## Шаг 3: Компиляция ресурсов
Скомпилируйте файл ресурсов, который указывает на `packed_payload.bin`.
```bash
rc.exe resources.rc
```
Это создаст файл `resources.res`.

## Шаг 4: Сборка финального лоадера
Соберите лоадер, прилинковав скомпилированные ресурсы.
```bash
cl.exe /O2 refactored_loader.c resources.res /link /OUT:loader.exe
```

---

## Как это работает
1.  **DLL** шифруется и сохраняется как **packed_payload.bin**.
2.  **rc.exe** внедряет этот бинарный файл в секцию ресурсов будущего EXE.
3.  При запуске **loader.exe** использует функции `FindResource` и `LoadResource` для чтения самого себя и извлечения зашифрованной DLL.
4.  DLL расшифровывается в памяти (без сохранения на диск) и запускается.
