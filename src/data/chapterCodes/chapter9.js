export default `const RoutingBasics = () => {
  return (
    <MemoryRouter>
      <nav>
        <Link to="/atlantic">Атлантический</Link>
        <Link to="/pacific">Тихий</Link>
      </nav>
      <Routes>
        <Route path="/atlantic" element={<Atlantic />} />
        <Route path="/pacific" element={<Pacific />} />
      </Routes>
    </MemoryRouter>
  );
};

const Atlantic = () => (
  <div>
    <h3>Атлантический океан</h3>
    <p>Атлантический океан занимает примерно 1/5 поверхности Земли.</p>
  </div>
);`;
