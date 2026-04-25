import React, { useState } from 'react';
import { Play, Code } from 'lucide-react';
import CodeBlock from './CodeBlock';

const ChapterLayout = ({ children, code, title, description }) => {
  const [activeTab, setActiveTab] = useState('preview');

  return (
    <div className="max-w-5xl mx-auto">
      <div className="mb-8">
        <h2 className="text-3xl font-bold text-gray-900 mb-2">{title}</h2>
        <p className="text-lg text-gray-600">{description}</p>
      </div>

      <div className="bg-white rounded-xl shadow-sm border overflow-hidden">
        <div className="flex border-b bg-gray-50">
          <button
            onClick={() => setActiveTab('preview')}
            className={`px-6 py-3 flex items-center text-sm font-medium transition-colors ${activeTab === 'preview' ? 'text-blue-600 border-b-2 border-blue-600 bg-white' : 'text-gray-500 hover:text-gray-700'}`}
          >
            <Play size={16} className="mr-2" />
            Превью
          </button>
          <button
            onClick={() => setActiveTab('code')}
            className={`px-6 py-3 flex items-center text-sm font-medium transition-colors ${activeTab === 'code' ? 'text-blue-600 border-b-2 border-blue-600 bg-white' : 'text-gray-500 hover:text-gray-700'}`}
          >
            <Code size={16} className="mr-2" />
            Код
          </button>
        </div>

        <div className="p-6">
          {activeTab === 'preview' ? (
            <div className="min-h-[400px]">
              {children}
            </div>
          ) : (
            <div className="overflow-auto max-h-[600px] rounded-lg">
              <CodeBlock code={code} />
            </div>
          )}
        </div>
      </div>
    </div>
  );
};

export default ChapterLayout;
