export default `const Cookbook = () => {
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
      </section>

      <section>
        <h3 className="text-sm font-bold text-gray-400 uppercase tracking-widest mb-4">Интерактивные компоненты</h3>
        <Counter />
      </section>
    </div>
  );
};`;
