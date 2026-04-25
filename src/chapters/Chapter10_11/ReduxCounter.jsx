import React, { useReducer } from 'react';
import { Layers, Send, User } from 'lucide-react';

// Simplified Redux-like logic using useReducer for demonstration
const initialState = {
  messages: [
    { text: 'Привет! Как продвигается изучение Redux?', user: 'Mentor', id: 1 },
    { text: 'Всё понятно, архитектура очень строгая!', user: 'You', id: 2 },
  ],
};

function reducer(state, action) {
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
  const [inputValue, setInputValue] = React.useState('');

  const handleSubmit = (e) => {
    e.preventDefault();
    if (!inputValue.trim()) return;
    dispatch({ type: 'ADD_MESSAGE', text: inputValue });
    setInputValue('');
  };

  return (
    <div className="p-8 bg-white border rounded-xl shadow-sm">
      <h2 className="text-2xl font-bold mb-6">Управление состоянием (Redux)</h2>

      <p className="text-gray-600 mb-8">
        В главах 10 и 11 мы изучаем <strong>Redux</strong>. Архитектура Redux строится на трех принципах:
        единый источник истины, состояние только для чтения и изменения с помощью чистых функций (редьюсеров).
      </p>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-8">
        <div className="flex flex-col h-[500px] border rounded-xl overflow-hidden bg-gray-50 shadow-inner">
          <div className="p-4 bg-white border-b flex items-center justify-between">
            <h3 className="font-bold flex items-center gap-2">
              <Layers className="text-blue-500" size={18} /> Чат (Redux Store)
            </h3>
            <span className="text-xs bg-blue-100 text-blue-600 px-2 py-1 rounded-full font-bold">
              {state.messages.length} сообщений
            </span>
          </div>

          <div className="flex-1 overflow-y-auto p-4 space-y-4">
            {state.messages.map((m) => (
              <div key={m.id} className={`flex ${m.user === 'You' ? 'justify-end' : 'justify-start'}`}>
                <div className={`max-w-[80%] p-3 rounded-lg shadow-sm ${m.user === 'You' ? 'bg-blue-600 text-white rounded-br-none' : 'bg-white text-gray-800 rounded-bl-none border'}`}>
                  <div className="flex items-center gap-1 mb-1 opacity-70 text-[10px] font-bold uppercase tracking-wider">
                    <User size={10} /> {m.user}
                  </div>
                  <p className="text-sm">{m.text}</p>
                  {m.user === 'You' && (
                    <button
                      onClick={() => dispatch({ type: 'DELETE_MESSAGE', id: m.id })}
                      className="mt-2 text-[10px] underline opacity-50 hover:opacity-100"
                    >
                      Удалить
                    </button>
                  )}
                </div>
              </div>
            ))}
          </div>

          <form onSubmit={handleSubmit} className="p-4 bg-white border-t flex gap-2">
            <input
              type="text"
              value={inputValue}
              onChange={(e) => setInputValue(e.target.value)}
              placeholder="Напишите сообщение..."
              className="flex-1 p-2 border rounded-lg focus:ring-2 focus:ring-blue-500 outline-none text-sm"
            />
            <button
              type="submit"
              className="p-2 bg-blue-600 text-white rounded-lg hover:bg-blue-700 transition-colors"
            >
              <Send size={18} />
            </button>
          </form>
        </div>

        <div className="space-y-6">
          <div className="p-6 bg-gray-900 rounded-xl text-gray-300 font-mono text-xs overflow-x-auto">
            <h4 className="text-blue-400 mb-4 font-bold border-b border-gray-700 pb-2">// Reducer Logic</h4>
            <pre>
{`function reducer(state, action) {
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
        messages: state.messages.filter(
          m => m.id !== action.id
        )
      };
    default:
      return state;
  }
}`}
            </pre>
          </div>

          <div className="p-4 bg-orange-50 rounded-lg border border-orange-200 text-orange-800 text-sm">
            <h4 className="font-bold mb-1">Почему это важно?</h4>
            <p>
              Redux позволяет сделать состояние приложения предсказуемым. Каждое изменение — это явное действие (Action), которое обрабатывается чистой функцией. Это упрощает отладку и тестирование.
            </p>
          </div>
        </div>
      </div>
    </div>
  );
};

export default ReduxCounter;
