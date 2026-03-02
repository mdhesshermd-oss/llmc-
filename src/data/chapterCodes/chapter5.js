export default `const ValidationForm = () => {
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
    <form onSubmit={onFormSubmit}>
      <input name='name' value={fields.name} onChange={onInputChange} />
      <input name='email' value={fields.email} onChange={onInputChange} />
      <button type="submit">Добавить</button>
    </form>
  );
};`;
