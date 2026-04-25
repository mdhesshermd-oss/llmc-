import React, { useState, useEffect } from 'react';
import { Database, Zap, User, MessageSquare } from 'lucide-react';

const GraphQLDemo = () => {
  const [loading, setLoading] = useState(false);
  const [data, setData] = useState(null);

  const fetchThreads = () => {
    setLoading(true);
    // Simulate GraphQL Query
    setTimeout(() => {
      setData({
        threads: [
          {
            id: '1',
            title: 'Как выучить GraphQL за выходные?',
            count: 12,
            author: { name: 'ReactFan', avatar: 'https://i.pravatar.cc/150?u=1' }
          },
          {
            id: '2',
            title: 'Аполло против Реле: что выбрать?',
            count: 8,
            author: { name: 'FullstackDev', avatar: 'https://i.pravatar.cc/150?u=2' }
          }
        ]
      });
      setLoading(false);
    }, 800);
  };

  useEffect(() => {
    fetchThreads();
  }, []);

  const queryCode = `query AllThreads {
  threads {
    id
    title
    count
    author {
      name
      avatar
    }
  }
}`;

  return (
    <div className="p-8 bg-white border rounded-xl shadow-sm">
      <h2 className="text-2xl font-bold mb-6">Работа с GraphQL</h2>

      <p className="text-gray-600 mb-8">
        В этой главе мы изучаем, как интегрировать <strong>GraphQL</strong> в наше приложение.
        Вместо множества REST эндпоинтов, GraphQL использует один эндпоинт и позволяет запрашивать именно те данные, которые нам нужны.
      </p>

      <div className="grid grid-cols-1 lg:grid-cols-2 gap-8">
        <div className="space-y-6">
          <div className="bg-gray-900 rounded-xl p-6 text-gray-300 font-mono text-sm overflow-x-auto shadow-lg">
            <h4 className="text-pink-400 mb-4 font-bold flex items-center gap-2">
              <Zap size={16} /> GraphQL Query
            </h4>
            <pre className="leading-relaxed">
              {queryCode}
            </pre>
          </div>

          <div className="p-4 bg-green-50 rounded-lg border border-green-200 text-green-800 text-sm">
            <h4 className="font-bold mb-1 flex items-center gap-2">
              <Database size={16} /> Почему GraphQL?
            </h4>
            <ul className="list-disc pl-5 space-y-1">
              <li>Отсутствие избыточной выборки данных (Overfetching).</li>
              <li>Строгая типизация схемы данных.</li>
              <li>Возможность получить связанные данные за один запрос.</li>
            </ul>
          </div>
        </div>

        <div>
          <h3 className="text-lg font-bold mb-4 flex items-center gap-2">
            <MessageSquare className="text-pink-500" size={20} />
            Результат запроса (Threads)
          </h3>

          <div className="border rounded-xl overflow-hidden bg-gray-50">
            {loading ? (
              <div className="p-20 text-center text-gray-400">
                <div className="animate-pulse flex flex-col items-center">
                  <div className="w-12 h-12 bg-gray-200 rounded-full mb-4"></div>
                  <div className="h-4 bg-gray-200 rounded w-48 mb-2"></div>
                  <div className="h-4 bg-gray-200 rounded w-32"></div>
                </div>
                <p className="mt-4">Выполнение запроса...</p>
              </div>
            ) : (
              <div className="divide-y">
                {data?.threads.map(thread => (
                  <div key={thread.id} className="p-4 bg-white hover:bg-pink-50 transition-colors cursor-pointer group">
                    <div className="flex items-center gap-3 mb-2">
                      <img src={thread.author.avatar} alt={thread.author.name} className="w-8 h-8 rounded-full border border-pink-200" />
                      <span className="text-xs font-bold text-gray-500">{thread.author.name}</span>
                    </div>
                    <h4 className="font-bold text-gray-800 group-hover:text-pink-600 transition-colors">{thread.title}</h4>
                    <div className="mt-2 flex items-center gap-4 text-xs text-gray-400">
                      <span className="flex items-center gap-1">
                        <MessageSquare size={12} /> {thread.count} ответов
                      </span>
                      <span>ID: {thread.id}</span>
                    </div>
                  </div>
                ))}
              </div>
            )}
          </div>
          <button
            onClick={fetchThreads}
            className="w-full mt-4 py-2 border-2 border-pink-500 text-pink-500 rounded-lg font-bold hover:bg-pink-500 hover:text-white transition-all"
          >
            Выполнить запрос повторно
          </button>
        </div>
      </div>
    </div>
  );
};

export default GraphQLDemo;
