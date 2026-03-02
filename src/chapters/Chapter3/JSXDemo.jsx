import React from 'react';

const JSXDemo = () => {
  const user = {
    firstName: 'Иван',
    lastName: 'Иванов',
    avatarUrl: 'https://i.pravatar.cc/150?u=ivan'
  };

  const formatName = (user) => {
    return user.firstName + ' ' + user.lastName;
  };

  // Пример условного рендеринга
  const getGreeting = (user) => {
    if (user) {
      return <h1 className="text-2xl font-bold mb-4">Привет, {formatName(user)}!</h1>;
    }
    return <h1 className="text-2xl font-bold mb-4">Привет, незнакомец.</h1>;
  };

  return (
    <div className="p-8 bg-white border rounded-xl shadow-sm">
      <div className="mb-8">
        <h3 className="text-lg font-bold text-gray-500 uppercase tracking-wider mb-4">Встраивание выражений</h3>
        {getGreeting(user)}
        <p className="text-gray-600">
          JSX позволяет встраивать любые JavaScript выражения внутри фигурных скобок <code>{'{ }'}</code>.
        </p>
      </div>

      <div className="mb-8 border-t pt-8">
        <h3 className="text-lg font-bold text-gray-500 uppercase tracking-wider mb-4">Атрибуты и дети</h3>
        <div className="flex items-center gap-4 bg-gray-50 p-4 rounded-lg">
          <img
            src={user.avatarUrl}
            alt={formatName(user)}
            className="w-16 h-16 rounded-full border-2 border-blue-500"
          />
          <div>
            <p className="font-bold text-lg">{formatName(user)}</p>
            <p className="text-sm text-gray-500">Пользователь React</p>
          </div>
        </div>
      </div>

      <div className="border-t pt-8">
        <h3 className="text-lg font-bold text-gray-500 uppercase tracking-wider mb-4">JSX - это объект</h3>
        <p className="text-gray-600 mb-4">
          Babel компилирует JSX в вызовы <code>React.createElement()</code>.
          Эти два примера эквивалентны:
        </p>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          <div className="bg-gray-900 p-4 rounded text-blue-400 font-mono text-sm">
            {`// JSX\nconst element = (\n  <h1 className="greeting">\n    Привет, мир!\n  </h1>\n);`}
          </div>
          <div className="bg-gray-900 p-4 rounded text-green-400 font-mono text-sm">
            {`// JavaScript\nconst element = React.createElement(\n  'h1',\n  {className: 'greeting'},\n  'Привет, мир!'\n);`}
          </div>
        </div>
      </div>
    </div>
  );
};

export default JSXDemo;
