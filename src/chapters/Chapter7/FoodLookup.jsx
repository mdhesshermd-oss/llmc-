import React, { useState } from 'react';
import { Search, Loader2 } from 'lucide-react';

// Mock Client
const client = {
  search: (query, cb) => {
    // Simulate server delay
    setTimeout(() => {
      const foods = [
        { description: 'Apple', kcal: 52, protein: 0.3, fat: 0.2, carbs: 14 },
        { description: 'Banana', kcal: 89, protein: 1.1, fat: 0.3, carbs: 23 },
        { description: 'Chicken Breast', kcal: 165, protein: 31, fat: 3.6, carbs: 0 },
        { description: 'Salmon', kcal: 208, protein: 20, fat: 13, carbs: 0 },
        { description: 'Broccoli', kcal: 34, protein: 2.8, fat: 0.4, carbs: 7 },
      ];
      const filtered = foods.filter(f =>
        f.description.toLowerCase().includes(query.toLowerCase())
      );
      cb(filtered);
    }, 500);
  }
};

const FoodLookup = () => {
  const [foods, setFoods] = useState([]);
  const [showRemoveIcon, setShowRemoveIcon] = useState(false);
  const [searchValue, setSearchValue] = useState('');
  const [loading, setLoading] = useState(false);

  const handleSearchChange = (e) => {
    const value = e.target.value;
    setSearchValue(value);

    if (value === '') {
      setFoods([]);
      setShowRemoveIcon(false);
    } else {
      setShowRemoveIcon(true);
      setLoading(true);
      client.search(value, (foods) => {
        setFoods(foods);
        setLoading(false);
      });
    }
  };

  const handleSearchCancel = () => {
    setFoods([]);
    setSearchValue('');
    setShowRemoveIcon(false);
  };

  return (
    <div className="p-8 bg-white border rounded-xl shadow-sm">
      <h2 className="text-2xl font-bold mb-6">Взаимодействие с сервером</h2>

      <p className="text-gray-600 mb-8">
        В этой главе мы реализуем поиск продуктов с имитацией запроса к API.
        Мы учимся обрабатывать состояния загрузки и обновлять интерфейс на основе полученных данных.
      </p>

      <div className="max-w-xl mx-auto">
        <div className="relative mb-8">
          <div className="absolute inset-y-0 left-0 pl-3 flex items-center pointer-events-none">
            <Search className="h-5 w-5 text-gray-400" />
          </div>
          <input
            type="text"
            className="block w-full pl-10 pr-10 py-3 border border-gray-300 rounded-lg focus:ring-blue-500 focus:border-blue-500 outline-none transition-all shadow-sm"
            placeholder="Поиск продуктов..."
            value={searchValue}
            onChange={handleSearchChange}
          />
          {showRemoveIcon && !loading && (
            <button
              onClick={handleSearchCancel}
              className="absolute inset-y-0 right-0 pr-3 flex items-center"
            >
              <span className="text-gray-400 hover:text-gray-600 text-xl">&times;</span>
            </button>
          )}
          {loading && (
            <div className="absolute inset-y-0 right-0 pr-3 flex items-center">
              <Loader2 className="h-5 w-5 text-blue-500 animate-spin" />
            </div>
          )}
        </div>

        <div className="overflow-hidden border rounded-lg">
          <table className="min-w-full divide-y divide-gray-200">
            <thead className="bg-gray-50">
              <tr>
                <th className="px-6 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider">Продукт</th>
                <th className="px-6 py-3 text-right text-xs font-medium text-gray-500 uppercase tracking-wider">Ккал</th>
                <th className="px-6 py-3 text-right text-xs font-medium text-gray-500 uppercase tracking-wider">Белки</th>
                <th className="px-6 py-3 text-right text-xs font-medium text-gray-500 uppercase tracking-wider">Жиры</th>
                <th className="px-6 py-3 text-right text-xs font-medium text-gray-500 uppercase tracking-wider">Углев.</th>
              </tr>
            </thead>
            <tbody className="bg-white divide-y divide-gray-200">
              {foods.map((food, i) => (
                <tr key={i} className="hover:bg-blue-50 transition-colors cursor-pointer">
                  <td className="px-6 py-4 whitespace-nowrap text-sm font-medium text-gray-900">{food.description}</td>
                  <td className="px-6 py-4 whitespace-nowrap text-sm text-right text-gray-500">{food.kcal}</td>
                  <td className="px-6 py-4 whitespace-nowrap text-sm text-right text-gray-500">{food.protein}г</td>
                  <td className="px-6 py-4 whitespace-nowrap text-sm text-right text-gray-500">{food.fat}г</td>
                  <td className="px-6 py-4 whitespace-nowrap text-sm text-right text-gray-500">{food.carbs}г</td>
                </tr>
              ))}
              {foods.length === 0 && !loading && (
                <tr>
                  <td colSpan="5" className="px-6 py-8 text-center text-gray-400 italic">
                    {searchValue ? 'Ничего не найдено' : 'Начните вводить название для поиска'}
                  </td>
                </tr>
              )}
            </tbody>
          </table>
        </div>
      </div>
    </div>
  );
};

export default FoodLookup;
