export default `// Продвинутые паттерны Redux
// Использование Redux Thunk для асинхронных действий

const fetchMessages = () => {
  return (dispatch) => {
    dispatch({ type: 'FETCH_MESSAGES_REQUEST' });
    client.getMessages((messages) => {
      dispatch({
        type: 'FETCH_MESSAGES_SUCCESS',
        messages: messages
      });
    });
  };
};`;
