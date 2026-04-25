import React, { useState } from 'react';

const TimerForm = ({ id, title: initialTitle, project: initialProject, onFormSubmit, onFormClose }) => {
  const [title, setTitle] = useState(initialTitle || '');
  const [project, setProject] = useState(initialProject || '');

  const submitText = id ? 'Обновить' : 'Создать';

  return (
    <div className="bg-white border rounded-lg shadow-sm p-6 mb-4">
      <div className="mb-4">
        <label className="block text-sm font-bold text-gray-700 mb-1">Название</label>
        <input
          type="text"
          value={title}
          onChange={(e) => setTitle(e.target.value)}
          className="w-full border rounded p-2 focus:outline-none focus:ring-2 focus:ring-blue-500"
        />
      </div>
      <div className="mb-6">
        <label className="block text-sm font-bold text-gray-700 mb-1">Проект</label>
        <input
          type="text"
          value={project}
          onChange={(e) => setProject(e.target.value)}
          className="w-full border rounded p-2 focus:outline-none focus:ring-2 focus:ring-blue-500"
        />
      </div>
      <div className="flex gap-2">
        <button
          onClick={() => onFormSubmit({ id, title, project })}
          className="flex-1 py-2 bg-blue-600 text-white rounded font-bold hover:bg-blue-700 transition-colors"
        >
          {submitText}
        </button>
        <button
          onClick={onFormClose}
          className="flex-1 py-2 bg-white border border-red-500 text-red-500 rounded font-bold hover:bg-red-50 transition-colors"
        >
          Отмена
        </button>
      </div>
    </div>
  );
};

export default TimerForm;
