import React from 'react';
import { Rocket, ShieldCheck, Zap, Globe, Github, Terminal } from 'lucide-react';

const Step = ({ number, title, children }) => (
  <div className="flex gap-4 mb-8">
    <div className="flex-shrink-0 w-10 h-10 bg-blue-600 text-white rounded-full flex items-center justify-center font-bold">
      {number}
    </div>
    <div>
      <h3 className="text-xl font-bold mb-2 text-gray-800">{title}</h3>
      <div className="text-gray-600">{children}</div>
    </div>
  </div>
);

const DeploymentGuide = () => {
  return (
    <div className="p-8 bg-white border rounded-xl shadow-sm">
      <div className="flex items-center gap-4 mb-8">
        <div className="p-3 bg-orange-100 text-orange-600 rounded-xl">
          <Rocket size={32} />
        </div>
        <div>
          <h2 className="text-3xl font-bold">Развертывание (Deployment)</h2>
          <p className="text-gray-500 text-lg">Финальный этап создания вашего приложения</p>
        </div>
      </div>

      <p className="text-gray-600 mb-12 text-lg leading-relaxed">
        Поздравляем! Вы прошли весь путь от первого компонента до сложного приложения с управлением состоянием.
        Последний шаг — сделать ваше приложение доступным для всего мира.
      </p>

      <div className="space-y-4">
        <Step number="1" title="Оптимизация сборки">
          <p className="mb-4">Перед деплоем необходимо создать оптимизированную версию приложения для продакшена.</p>
          <div className="bg-gray-900 rounded-lg p-4 font-mono text-sm text-blue-400">
            <div className="flex items-center gap-2 mb-1">
              <Terminal size={14} /> <span>npm run build</span>
            </div>
            <div className="text-gray-500 italic"># Эта команда создаст папку dist/ с минифицированными файлами</div>
          </div>
        </Step>

        <Step number="2" title="Переменные окружения">
          <p className="mb-2">Убедитесь, что все секретные ключи и URL API вынесены в <code>.env</code> файлы.</p>
          <div className="bg-blue-50 p-4 rounded-lg border border-blue-100 flex items-center gap-3 text-blue-800 text-sm">
            <ShieldCheck className="text-blue-500 shrink-0" />
            <span>Никогда не храните API ключи напрямую в коде или в Git-репозитории!</span>
          </div>
        </Step>

        <Step number="3" title="Выбор платформы">
          <p className="mb-4">Для React-приложений лучше всего подходят платформы с поддержкой Static Site Hosting:</p>
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            <div className="p-4 border rounded-lg hover:border-blue-500 transition-colors cursor-pointer group">
              <div className="flex items-center gap-2 font-bold mb-1">
                <Globe size={16} className="text-blue-500" /> Vercel / Netlify
              </div>
              <p className="text-xs text-gray-500">Автоматический деплой при пуше в GitHub. Идеально для большинства проектов.</p>
            </div>
            <div className="p-4 border rounded-lg hover:border-blue-500 transition-colors cursor-pointer group">
              <div className="flex items-center gap-2 font-bold mb-1">
                <Github size={16} className="text-gray-800" /> GitHub Pages
              </div>
              <p className="text-xs text-gray-500">Бесплатный хостинг для статических сайтов напрямую из вашего репозитория.</p>
            </div>
          </div>
        </Step>
      </div>

      <div className="mt-12 p-8 bg-gradient-to-r from-blue-600 to-indigo-700 rounded-2xl text-white">
        <h3 className="text-2xl font-bold mb-4 flex items-center gap-2">
          <Zap /> Что дальше?
        </h3>
        <p className="mb-6 opacity-90">
          Изучение React — это непрерывный процесс. Технологии меняются, но фундаментальные принципы,
          которые вы изучили в этой книге, останутся с вами навсегда.
        </p>
        <div className="flex gap-4">
          <button className="px-6 py-2 bg-white text-blue-600 rounded-lg font-bold hover:bg-blue-50 transition-colors">
            Вернуться к началу
          </button>
          <button className="px-6 py-2 bg-blue-500 text-white border border-blue-400 rounded-lg font-bold hover:bg-blue-400 transition-colors">
            Больше ресурсов
          </button>
        </div>
      </div>
    </div>
  );
};

export default DeploymentGuide;
