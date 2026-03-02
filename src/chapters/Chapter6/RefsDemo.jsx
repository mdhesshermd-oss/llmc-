import React, { useRef, useState } from 'react';

const RefsDemo = () => {
  const inputRef = useRef(null);
  const [names, setNames] = useState([]);

  const onFormSubmit = (evt) => {
    evt.preventDefault();
    const name = inputRef.current.value;
    if (!name) return;

    setNames([...names, name]);
    inputRef.current.value = '';
  };

  const focusInput = () => {
    inputRef.current.focus();
  };

  return (
    <div className="p-8 bg-white border rounded-xl shadow-sm">
      <h2 className="text-2xl font-bold mb-6">Использование Refs</h2>

      <p className="text-gray-600 mb-6">
        В этой главе мы используем <code>useRef</code> (или <code>ref</code> в классовых компонентах) для доступа к DOM-элементам напрямую.
        Это полезно для управления фокусом, выделения текста или интеграции со сторонними библиотеками.
      </p>

      <div className="p-6 bg-gray-50 rounded-xl border mb-8">
        <form onSubmit={onFormSubmit} className="flex gap-2 mb-4">
          <input
            type="text"
            ref={inputRef}
            placeholder="Введите имя..."
            className="flex-1 p-2 border rounded focus:ring-2 focus:ring-blue-500 outline-none"
          />
          <button
            type="submit"
            className="px-4 py-2 bg-blue-600 text-white rounded font-bold hover:bg-blue-700 transition-colors"
          >
            Добавить
          </button>
        </form>

        <button
          onClick={focusInput}
          className="text-sm text-blue-600 hover:underline"
        >
          Установить фокус на поле ввода
        </button>
      </div>

      <div>
        <h3 className="font-bold mb-4 uppercase text-xs text-gray-500">Список (из неуправляемого инпута):</h3>
        <ul className="space-y-2">
          {names.map((name, i) => (
            <li key={i} className="p-2 bg-blue-50 rounded border border-blue-100 text-blue-800">
              {name}
            </li>
          ))}
          {names.length === 0 && <li className="text-gray-400 italic text-sm">Список пуст</li>}
        </ul>
      </div>
    </div>
  );
};

export default RefsDemo;
