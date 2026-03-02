import React, { useState } from 'react';
import { Info, AlertCircle, CheckCircle } from 'lucide-react';

const Header = ({ children }) => (
  <header className="mb-6 border-b pb-4">
    <h2 className="text-2xl font-bold text-gray-800">{children}</h2>
  </header>
);

const Button = ({ onClick, children, type = 'primary' }) => {
  const styles = {
    primary: 'bg-blue-600 text-white hover:bg-blue-700',
    secondary: 'bg-gray-200 text-gray-800 hover:bg-gray-300',
    danger: 'bg-red-600 text-white hover:bg-red-700',
  };

  return (
    <button
      onClick={onClick}
      className={`px-4 py-2 rounded font-medium transition-colors ${styles[type]}`}
    >
      {children}
    </button>
  );
};

const Message = ({ type = 'info', title, children }) => {
  const configs = {
    info: { color: 'blue', icon: Info },
    error: { color: 'red', icon: AlertCircle },
    success: { color: 'green', icon: CheckCircle },
  };

  const { color, icon: Icon } = configs[type];

  return (
    <div className={`p-4 border rounded-lg bg-${color}-50 border-${color}-200 mb-4 flex gap-3`}>
      <Icon className={`text-${color}-500`} size={20} />
      <div>
        {title && <h4 className={`font-bold text-${color}-800 mb-1`}>{title}</h4>}
        <div className={`text-${color}-700`}>{children}</div>
      </div>
    </div>
  );
};

const Counter = () => {
  const [count, setCount] = useState(0);

  return (
    <div className="p-6 bg-gray-50 rounded-xl border text-center">
      <h3 className="text-lg font-bold mb-4">Счетчик</h3>
      <div className="text-5xl font-mono mb-6">{count}</div>
      <div className="flex justify-center gap-2">
        <Button onClick={() => setCount(count - 1)} type="secondary">-1</Button>
        <Button onClick={() => setCount(0)} type="danger">Сброс</Button>
        <Button onClick={() => setCount(count + 1)}>+1</Button>
      </div>
    </div>
  );
};

const Cookbook = () => {
  return (
    <div className="p-8 bg-white border rounded-xl shadow-sm">
      <Header>Библиотека компонентов (Cookbook)</Header>

      <section className="mb-10">
        <h3 className="text-sm font-bold text-gray-400 uppercase tracking-widest mb-4">Кнопки</h3>
        <div className="flex gap-4">
          <Button onClick={() => alert('Нажата основная кнопка')}>Основная</Button>
          <Button onClick={() => alert('Нажата вторичная кнопка')} type="secondary">Вторичная</Button>
          <Button onClick={() => alert('Нажата опасная кнопка')} type="danger">Опасная</Button>
        </div>
      </section>

      <section className="mb-10">
        <h3 className="text-sm font-bold text-gray-400 uppercase tracking-widest mb-4">Сообщения</h3>
        <Message type="info" title="Для вашего сведения">
          Это информационное сообщение, построенное на переиспользуемом компоненте Message.
        </Message>
        <Message type="success" title="Успех!">
          Операция была успешно завершена.
        </Message>
        <Message type="error" title="Ошибка">
          Произошло что-то непредвиденное. Пожалуйста, попробуйте еще раз.
        </Message>
      </section>

      <section>
        <h3 className="text-sm font-bold text-gray-400 uppercase tracking-widest mb-4">Интерактивные компоненты</h3>
        <Counter />
      </section>
    </div>
  );
};

export default Cookbook;
