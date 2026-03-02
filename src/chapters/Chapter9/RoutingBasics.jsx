import React from 'react';
import { MemoryRouter, Routes, Route, Link, useParams, useLocation } from 'react-router-dom';
import { Map, Info, Waves } from 'lucide-react';

const Atlantic = () => (
  <div className="animate-in fade-in slide-in-from-bottom-4 duration-500">
    <h3 className="text-xl font-bold mb-2 flex items-center gap-2">
      <Waves className="text-blue-500" /> Атлантический океан
    </h3>
    <p className="text-gray-600">
      Атлантический океан занимает примерно 1/5 поверхности Земли. Это второй по величине океан в мире.
    </p>
  </div>
);

const Pacific = () => (
  <div className="animate-in fade-in slide-in-from-bottom-4 duration-500">
    <h3 className="text-xl font-bold mb-2 flex items-center gap-2">
      <Waves className="text-blue-600" /> Тихий океан
    </h3>
    <p className="text-gray-600">
      Фернан Магеллан, португальский исследователь, назвал этот океан "mar pacifico" в 1521 году, что означает "мирное море".
    </p>
  </div>
);

const BlackSea = () => (
  <div className="animate-in fade-in slide-in-from-bottom-4 duration-500">
    <h3 className="text-xl font-bold mb-2 flex items-center gap-2">
      <Waves className="text-gray-700" /> Черное море
    </h3>
    <p className="text-gray-600">
      Внутреннее море бассейна Атлантического океана. Омывает берега России, Украины, Румынии, Болгарии, Турции и Грузии.
    </p>
  </div>
);

const NotFound = () => (
  <div className="text-center py-8 text-red-500">
    <h3 className="text-xl font-bold">404: Страница не найдена</h3>
    <p>Выберите один из водоемов в списке выше.</p>
  </div>
);

const RoutingInner = () => {
  const location = useLocation();

  return (
    <div className="p-8 bg-white border rounded-xl shadow-sm">
      <h2 className="text-2xl font-bold mb-6">Маршрутизация (React Router)</h2>

      <p className="text-gray-600 mb-8">
        В этой главе мы изучаем, как создавать многостраничные приложения.
        Пример ниже использует <code>MemoryRouter</code> для демонстрации навигации без изменения URL в адресной строке браузера.
      </p>

      <div className="bg-gray-50 rounded-xl border p-6">
        <div className="mb-6">
          <h4 className="text-sm font-bold text-gray-400 uppercase tracking-widest mb-4">Выберите водоем:</h4>
          <nav className="flex gap-4">
            <Link
              to="/atlantic"
              className={`px-4 py-2 rounded-lg font-medium transition-all ${location.pathname === '/atlantic' ? 'bg-blue-600 text-white shadow-md' : 'bg-white border hover:bg-gray-100'}`}
            >
              Атлантический
            </Link>
            <Link
              to="/pacific"
              className={`px-4 py-2 rounded-lg font-medium transition-all ${location.pathname === '/pacific' ? 'bg-blue-600 text-white shadow-md' : 'bg-white border hover:bg-gray-100'}`}
            >
              Тихий
            </Link>
            <Link
              to="/black-sea"
              className={`px-4 py-2 rounded-lg font-medium transition-all ${location.pathname === '/black-sea' ? 'bg-blue-600 text-white shadow-md' : 'bg-white border hover:bg-gray-100'}`}
            >
              Черное море
            </Link>
          </nav>
        </div>

        <div className="bg-white p-8 rounded-xl border min-h-[200px] flex flex-col justify-center">
          <Routes>
            <Route path="/" element={<div className="text-center text-gray-400 italic">Нажмите на одну из кнопок выше для навигации</div>} />
            <Route path="/atlantic" element={<Atlantic />} />
            <Route path="/pacific" element={<Pacific />} />
            <Route path="/black-sea" element={<BlackSea />} />
            <Route path="*" element={<NotFound />} />
          </Routes>
        </div>
      </div>

      <div className="mt-8 flex items-start gap-3 p-4 bg-yellow-50 rounded-lg border border-yellow-200 text-yellow-800 text-sm">
        <Info size={20} className="shrink-0" />
        <p>
          В реальном приложении вы будете использовать <code>BrowserRouter</code>, чтобы URL в адресной строке синхронизировался с состоянием вашего приложения.
        </p>
      </div>
    </div>
  );
};

const RoutingBasics = () => {
  return (
    <MemoryRouter>
      <RoutingInner />
    </MemoryRouter>
  );
};

export default RoutingBasics;
