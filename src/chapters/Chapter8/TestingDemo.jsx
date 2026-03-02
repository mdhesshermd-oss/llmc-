import React, { useState } from 'react';
import { Beaker } from 'lucide-react';

const Counter = () => {
  const [count, setCount] = useState(0);
  return (
    <div className="text-center p-4 border rounded bg-white shadow-sm">
      <div className="text-4xl font-mono mb-4" data-testid="count">{count}</div>
      <button
        onClick={() => setCount(count + 1)}
        className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 transition-colors"
        data-testid="increment-button"
      >
        Увеличить
      </button>
    </div>
  );
};

const TestingDemo = () => {
  const testCode = `import { render, screen, fireEvent } from '@testing-library/react';
import { describe, it, expect } from 'vitest';
import Counter from './Counter';

describe('Counter', () => {
  it('должен отображать начальное значение 0', () => {
    render(<Counter />);
    const countElement = screen.getByTestId('count');
    expect(countElement.textContent).toBe('0');
  });

  it('должен увеличивать значение при нажатии на кнопку', () => {
    render(<Counter />);
    const button = screen.getByTestId('increment-button');
    const countElement = screen.getByTestId('count');

    fireEvent.click(button);

    expect(countElement.textContent).toBe('1');
  });
});`;

  return (
    <div className="p-8 bg-white border rounded-xl shadow-sm">
      <h2 className="text-2xl font-bold mb-6">Тестирование</h2>

      <p className="text-gray-600 mb-8">
        В этой главе мы изучаем, как писать модульные тесты для React-компонентов.
        Мы используем <strong>Vitest</strong> как среду выполнения тестов и <strong>React Testing Library</strong> для взаимодействия с DOM.
      </p>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-8">
        <div>
          <h3 className="text-lg font-bold mb-4 flex items-center gap-2">
            <Beaker className="text-blue-500" size={20} />
            Компонент для теста
          </h3>
          <div className="bg-gray-50 p-8 rounded-xl border border-dashed border-gray-300 flex items-center justify-center min-h-[250px]">
            <Counter />
          </div>
          <p className="mt-4 text-sm text-gray-500 italic">
            Это интерактивный компонент, который мы будем тестировать.
          </p>
        </div>

        <div>
          <h3 className="text-lg font-bold mb-4">Код теста (Vitest + RTL)</h3>
          <pre className="bg-gray-900 text-gray-100 p-6 rounded-lg overflow-x-auto text-sm font-mono leading-relaxed">
            {testCode}
          </pre>
        </div>
      </div>

      <div className="mt-12 p-6 bg-blue-50 rounded-xl border border-blue-100">
        <h3 className="text-blue-800 font-bold mb-2 uppercase text-xs tracking-wider">Основные принципы:</h3>
        <ul className="list-disc pl-5 text-blue-700 space-y-2">
          <li>Тестируйте поведение, а не детали реализации.</li>
          <li>Используйте <code>data-testid</code> для надежного поиска элементов.</li>
          <li>Имитируйте действия пользователя (клики, ввод текста) с помощью <code>fireEvent</code> или <code>userEvent</code>.</li>
        </ul>
      </div>
    </div>
  );
};

export default TestingDemo;
