import React, { useState } from 'react';
import { Plus } from 'lucide-react';
import TimerForm from './TimerForm';

const ToggleableTimerForm = ({ onFormSubmit }) => {
  const [isOpen, setIsOpen] = useState(false);

  const handleFormOpen = () => setIsOpen(true);
  const handleFormClose = () => setIsOpen(false);
  const handleFormSubmit = (timer) => {
    onFormSubmit(timer);
    setIsOpen(false);
  };

  if (isOpen) {
    return (
      <TimerForm
        onFormSubmit={handleFormSubmit}
        onFormClose={handleFormClose}
      />
    );
  } else {
    return (
      <div className="flex justify-center py-4">
        <button
          onClick={handleFormOpen}
          className="p-3 bg-white border border-gray-300 rounded-full shadow-sm text-gray-600 hover:text-blue-600 hover:border-blue-600 transition-all"
        >
          <Plus size={32} />
        </button>
      </div>
    );
  }
};

export default ToggleableTimerForm;
