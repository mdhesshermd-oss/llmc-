import React, { useState, useEffect } from 'react';
import { renderElapsedString } from './helpers';
import { Trash, Edit3, Play, Square } from 'lucide-react';

const Timer = ({ id, title, project, elapsed, runningSince, onEditClick, onTrashClick, onStartClick, onStopClick }) => {
  const [elapsedString, setElapsedString] = useState(renderElapsedString(elapsed, runningSince));

  useEffect(() => {
    let intervalId;
    if (runningSince) {
      intervalId = setInterval(() => {
        setElapsedString(renderElapsedString(elapsed, runningSince));
      }, 1000);
    } else {
      setElapsedString(renderElapsedString(elapsed, runningSince));
    }
    return () => clearInterval(intervalId);
  }, [elapsed, runningSince]);

  return (
    <div className="bg-white border rounded-lg shadow-sm p-6 mb-4">
      <div className="mb-4">
        <h3 className="text-xl font-bold">{title}</h3>
        <p className="text-gray-500">{project}</p>
      </div>
      <div className="text-center py-6">
        <h2 className="text-4xl font-mono font-bold text-gray-800">{elapsedString}</h2>
      </div>
      <div className="flex justify-end gap-2 mb-4">
        <button onClick={() => onTrashClick(id)} className="p-2 text-gray-400 hover:text-red-500 transition-colors">
          <Trash size={18} />
        </button>
        <button onClick={onEditClick} className="p-2 text-gray-400 hover:text-blue-500 transition-colors">
          <Edit3 size={18} />
        </button>
      </div>
      {runningSince ? (
        <button
          onClick={() => onStopClick(id)}
          className="w-full py-2 bg-red-50 border border-red-500 text-red-500 rounded font-bold hover:bg-red-500 hover:text-white transition-all flex items-center justify-center gap-2"
        >
          <Square size={16} fill="currentColor" /> Стоп
        </button>
      ) : (
        <button
          onClick={() => onStartClick(id)}
          className="w-full py-2 bg-blue-50 border border-blue-500 text-blue-500 rounded font-bold hover:bg-blue-500 hover:text-white transition-all flex items-center justify-center gap-2"
        >
          <Play size={16} fill="currentColor" /> Старт
        </button>
      )}
    </div>
  );
};

export default Timer;
