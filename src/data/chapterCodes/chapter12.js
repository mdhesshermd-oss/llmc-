export default `const queryCode = \`query AllThreads {
  threads {
    id
    title
    count
    author {
      name
      avatar
    }
  }
}\`;

const GraphQLDemo = () => {
  const [data, setData] = useState(null);

  useEffect(() => {
    // Имитация GraphQL запроса
    fetchThreads().then(res => setData(res.data));
  }, []);

  return (
    <div>
      {data?.threads.map(thread => (
        <div key={thread.id}>
          <h4>{thread.title}</h4>
          <span>Автор: {thread.author.name}</span>
        </div>
      ))}
    </div>
  );
};`;
