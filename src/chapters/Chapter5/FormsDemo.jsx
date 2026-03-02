import React, { useState } from 'react';

const BasicButton = () => {
  const onButtonClick = (evt) => {
    const button = evt.target;
    console.log(`The user clicked ${button.name || button.innerText}`);
    alert(`Вы нажали: ${button.innerText}`);
  };

  return (
    <div className="p-4 border rounded bg-gray-50 mb-6">
      <h3 className="font-bold mb-4 uppercase text-xs text-gray-500">Базовые кнопки</h3>
      <div className="flex gap-4">
        <button
          name='button-1'
          onClick={onButtonClick}
          className="px-4 py-2 bg-blue-500 text-white rounded hover:bg-blue-600 transition-colors"
        >
          Отлично
        </button>
        <button
          name='button-2'
          onClick={onButtonClick}
          className="px-4 py-2 bg-green-500 text-white rounded hover:bg-green-600 transition-colors"
        >
          Потрясающе
        </button>
      </div>
    </div>
  );
};

const ValidationForm = () => {
  const [fields, setFields] = useState({ name: '', email: '' });
  const [fieldErrors, setFieldErrors] = useState({});
  const [people, setPeople] = useState([]);

  const validate = (person) => {
    const errors = {};
    if (!person.name) errors.name = 'Имя обязательно';
    if (!person.email) errors.email = 'Email обязателен';
    if (person.email && !person.email.includes('@')) errors.email = 'Некорректный Email';
    return errors;
  };

  const onInputChange = (evt) => {
    const newFields = { ...fields, [evt.target.name]: evt.target.value };
    setFields(newFields);
  };

  const onFormSubmit = (evt) => {
    evt.preventDefault();
    const person = fields;
    const errors = validate(person);
    setFieldErrors(errors);

    if (Object.keys(errors).length) return;

    setPeople(people.concat(person));
    setFields({ name: '', email: '' });
  };

  return (
    <div className="p-4 border rounded bg-gray-50">
      <h3 className="font-bold mb-4 uppercase text-xs text-gray-500">Форма с валидацией</h3>
      <form onSubmit={onFormSubmit} className="mb-6">
        <div className="mb-4">
          <input
            placeholder='Имя'
            name='name'
            value={fields.name}
            onChange={onInputChange}
            className={`w-full p-2 border rounded ${fieldErrors.name ? 'border-red-500' : ''}`}
          />
          {fieldErrors.name && <span className="text-red-500 text-xs">{fieldErrors.name}</span>}
        </div>
        <div className="mb-4">
          <input
            placeholder='Email'
            name='email'
            value={fields.email}
            onChange={onInputChange}
            className={`w-full p-2 border rounded ${fieldErrors.email ? 'border-red-500' : ''}`}
          />
          {fieldErrors.email && <span className="text-red-500 text-xs">{fieldErrors.email}</span>}
        </div>
        <button type="submit" className="w-full py-2 bg-blue-600 text-white rounded font-bold hover:bg-blue-700 transition-colors">
          Добавить
        </button>
      </form>

      <div>
        <h4 className="font-bold mb-2">Список людей:</h4>
        <ul className="list-disc pl-5">
          {people.map((person, i) => <li key={i}>{person.name} ({person.email})</li>)}
        </ul>
      </div>
    </div>
  );
};

const FormsDemo = () => {
  return (
    <div className="p-8 bg-white border rounded-xl shadow-sm">
      <h2 className="text-2xl font-bold mb-6">Работа с формами</h2>
      <BasicButton />
      <ValidationForm />
    </div>
  );
};

export default FormsDemo;
