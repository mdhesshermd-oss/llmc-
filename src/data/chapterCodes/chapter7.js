export default `const FoodLookup = () => {
  const [foods, setFoods] = useState([]);
  const [searchValue, setSearchValue] = useState('');
  const [loading, setLoading] = useState(false);

  const handleSearchChange = (e) => {
    const value = e.target.value;
    setSearchValue(value);

    if (value === '') {
      setFoods([]);
    } else {
      setLoading(true);
      client.search(value, (foods) => {
        setFoods(foods);
        setLoading(false);
      });
    }
  };

  return (
    <div>
      <input
        type="text"
        placeholder="Поиск продуктов..."
        value={searchValue}
        onChange={handleSearchChange}
      />
      {loading ? <p>Загрузка...</p> : (
        <table>
          { /* Рендеринг списка продуктов */ }
        </table>
      )}
    </div>
  );
};`;
