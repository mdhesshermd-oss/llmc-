export default `const JSXDemo = () => {
  const user = {
    firstName: 'Иван',
    lastName: 'Иванов',
    avatarUrl: 'https://i.pravatar.cc/150?u=ivan'
  };

  const formatName = (user) => {
    return user.firstName + ' ' + user.lastName;
  };

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
          JSX позволяет встраивать любые JavaScript выражения внутри фигурных скобок { '{ }' }.
        </p>
      </div>
      { /* ... остальной код ... */ }
    </div>
  );
};`;
