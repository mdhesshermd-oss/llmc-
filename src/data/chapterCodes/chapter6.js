export default `const RefsDemo = () => {
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
    <form onSubmit={onFormSubmit}>
      <input type="text" ref={inputRef} />
      <button type="submit">Добавить</button>
      <button onClick={focusInput}>Установить фокус</button>
    </form>
  );
};`;
