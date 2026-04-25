export default `import React, { useState } from 'react';
import EditableTimerList from './EditableTimerList';
import ToggleableTimerForm from './ToggleableTimerForm';
import { newTimer } from './helpers';

const TimersDashboard = () => {
  const [timers, setTimers] = useState([
    {
      title: 'Изучить React',
      project: 'Обучение',
      id: '1',
      elapsed: 5456099,
      runningSince: Date.now(),
    },
    {
      title: 'Сделать зарядку',
      project: 'Здоровье',
      id: '2',
      elapsed: 1273998,
      runningSince: null,
    },
  ]);

  const handleCreateFormSubmit = (timer) => {
    const t = newTimer(timer);
    setTimers(timers.concat(t));
  };

  const handleEditFormSubmit = (attrs) => {
    setTimers(timers.map((timer) => {
      if (timer.id === attrs.id) {
        return { ...timer, title: attrs.title, project: attrs.project };
      } else {
        return timer;
      }
    }));
  };

  const handleTrashClick = (timerId) => {
    setTimers(timers.filter(t => t.id !== timerId));
  };

  const handleStartClick = (timerId) => {
    const now = Date.now();
    setTimers(timers.map((timer) => {
      if (timer.id === timerId) {
        return { ...timer, runningSince: now };
      } else {
        return timer;
      }
    }));
  };

  const handleStopClick = (timerId) => {
    const now = Date.now();
    setTimers(timers.map((timer) => {
      if (timer.id === timerId) {
        const lastElapsed = now - timer.runningSince;
        return { ...timer, elapsed: timer.elapsed + lastElapsed, runningSince: null };
      } else {
        return timer;
      }
    }));
  };

  return (
    <div className="max-w-md mx-auto p-4">
      <h1 className="text-3xl font-bold mb-8 text-center border-b pb-4">Учет времени</h1>
      <EditableTimerList
        timers={timers}
        onFormSubmit={handleEditFormSubmit}
        onTrashClick={handleTrashClick}
        onStartClick={handleStartClick}
        onStopClick={handleStopClick}
      />
      <ToggleableTimerForm onFormSubmit={handleCreateFormSubmit} />
    </div>
  );
};

export default TimersDashboard;`;
