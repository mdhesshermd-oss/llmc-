import React, { Suspense, lazy } from 'react';
import { BrowserRouter as Router, Routes, Route, Link, useParams } from 'react-router-dom';
import { chapters } from './data/chapters';
import { BookOpen, ChevronRight } from 'lucide-react';
import ChapterLayout from './components/ChapterLayout';

// Lazy load chapter components
const ProductList = lazy(() => import('./chapters/Chapter1/ProductHunt'));
const TimersDashboard = lazy(() => import('./chapters/Chapter2/TimeTracker/TimersDashboard'));
const JSXDemo = lazy(() => import('./chapters/Chapter3/JSXDemo'));
const Cookbook = lazy(() => import('./chapters/Chapter4/Cookbook'));
const FormsDemo = lazy(() => import('./chapters/Chapter5/FormsDemo'));
const RefsDemo = lazy(() => import('./chapters/Chapter6/RefsDemo'));
const FoodLookup = lazy(() => import('./chapters/Chapter7/FoodLookup'));
const TestingDemo = lazy(() => import('./chapters/Chapter8/TestingDemo'));
const RoutingBasics = lazy(() => import('./chapters/Chapter9/RoutingBasics'));
const ReduxCounter = lazy(() => import('./chapters/Chapter10_11/ReduxCounter'));
const GraphQLDemo = lazy(() => import('./chapters/Chapter12/GraphQLDemo'));
const DeploymentGuide = lazy(() => import('./chapters/Chapter13/DeploymentGuide'));

// Placeholder components until they are implemented
const Placeholder = ({ id }) => <div className="p-8 text-center text-gray-500">Глава {id} находится в разработке.</div>;

const chapterComponents = {
  1: ProductList,
  2: TimersDashboard,
  3: JSXDemo,
  4: Cookbook,
  5: FormsDemo,
  6: RefsDemo,
  7: FoodLookup,
  8: TestingDemo,
  9: RoutingBasics,
  10: ReduxCounter,
  11: ReduxCounter,
  12: GraphQLDemo,
  13: DeploymentGuide,
};

const Layout = ({ children }) => {
  return (
    <div className="flex h-screen bg-gray-100">
      <aside className="w-64 bg-white border-r overflow-y-auto">
        <div className="p-6 border-b">
          <Link to="/" className="text-xl font-bold text-gray-800">React Fullstack Курс</Link>
        </div>
        <nav className="p-4">
          <ul className="space-y-2">
            <li>
              <Link to="/" className="flex items-center p-2 text-gray-700 hover:bg-gray-100 rounded">
                <BookOpen size={20} className="mr-3" />
                Введение
              </Link>
            </li>
            {chapters.map(chapter => (
              <li key={chapter.id}>
                <Link to={chapter.path} className="flex items-center p-2 text-gray-700 hover:bg-gray-100 rounded">
                  <span className="w-6 h-6 flex items-center justify-center bg-blue-100 text-blue-600 rounded-full text-xs mr-3 font-bold">
                    {chapter.id}
                  </span>
                  <span className="text-sm truncate">{chapter.title}</span>
                </Link>
              </li>
            ))}
          </ul>
        </nav>
      </aside>
      <main className="flex-1 overflow-y-auto p-8">
        <Suspense fallback={<div className="flex items-center justify-center h-full">Загрузка...</div>}>
          {children}
        </Suspense>
      </main>
    </div>
  );
};

const ChapterPage = () => {
  const { id } = useParams();
  const chapterId = parseInt(id);
  const chapter = chapters.find(c => c.id === chapterId);

  if (!chapter) return <div>Глава не найдена</div>;

  const Component = chapterComponents[chapterId] || (() => <Placeholder id={chapterId} />);

  // Logic to load code string from chapterCodes
  const [code, setCode] = React.useState("// Загрузка кода...");

  React.useEffect(() => {
    import(`./data/chapterCodes/chapter${chapterId}.js`)
      .then(module => setCode(module.default))
      .catch(() => setCode("// Исходный код для этой главы еще не добавлен."));
  }, [chapterId]);

  return (
    <ChapterLayout
      title={`Глава ${chapter.id}: ${chapter.title}`}
      description={chapter.description}
      code={code}
    >
      <Component />
    </ChapterLayout>
  );
};

const Home = () => {
  return (
    <div className="max-w-4xl mx-auto">
      <div className="text-center mb-12">
        <h1 className="text-5xl font-extrabold text-gray-900 mb-4">
          Полноценный курс React
        </h1>
        <p className="text-xl text-gray-600">
          На основе книги "Fullstack React: The Complete Guide to ReactJS and Friends"
        </p>
      </div>

      <div className="grid gap-6">
        {chapters.map(chapter => (
          <Link key={chapter.id} to={chapter.path} className="group block bg-white p-6 rounded-xl border shadow-sm hover:shadow-md transition-all">
            <div className="flex items-center justify-between">
              <div className="flex items-center">
                <div className="w-12 h-12 bg-blue-600 text-white rounded-lg flex items-center justify-center text-xl font-bold mr-4">
                  {chapter.id}
                </div>
                <div>
                  <h3 className="text-xl font-bold text-gray-900 group-hover:text-blue-600 transition-colors">
                    {chapter.title}
                  </h3>
                  <p className="text-gray-600">{chapter.description}</p>
                </div>
              </div>
              <ChevronRight className="text-gray-400 group-hover:text-blue-600" />
            </div>
          </Link>
        ))}
      </div>
    </div>
  );
};

function App() {
  return (
    <Router>
      <Layout>
        <Routes>
          <Route path="/" element={<Home />} />
          <Route path="/chapter/:id" element={<ChapterPage />} />
        </Routes>
      </Layout>
    </Router>
  );
}

export default App;
