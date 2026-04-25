export default `function reducer(state, action) {
  switch (action.type) {
    case 'ADD_MESSAGE':
      return {
        ...state,
        messages: [...state.messages, {
          text: action.text,
          user: 'You',
          id: Date.now()
        }]
      };
    case 'DELETE_MESSAGE':
      return {
        ...state,
        messages: state.messages.filter(m => m.id !== action.id)
      };
    default:
      return state;
  }
}

const ReduxCounter = () => {
  const [state, dispatch] = useReducer(reducer, initialState);
  // ... render logic
};`;
