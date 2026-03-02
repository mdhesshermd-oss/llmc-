import React, { useState } from 'react';
import Timer from './Timer';
import TimerForm from './TimerForm';

const EditableTimer = ({ id, title, project, elapsed, runningSince, onFormSubmit, onTrashClick, onStartClick, onStopClick }) => {
  const [editFormOpen, setEditFormOpen] = useState(false);

  const handleEditClick = () => setEditFormOpen(true);
  const handleFormClose = () => setEditFormOpen(false);
  const handleSubmit = (timer) => {
    onFormSubmit(timer);
    setEditFormOpen(false);
  };

  if (editFormOpen) {
    return (
      <TimerForm
        id={id}
        title={title}
        project={project}
        onFormSubmit={handleSubmit}
        onFormClose={handleFormClose}
      />
    );
  } else {
    return (
      <Timer
        id={id}
        title={title}
        project={project}
        elapsed={elapsed}
        runningSince={runningSince}
        onEditClick={handleEditClick}
        onTrashClick={onTrashClick}
        onStartClick={onStartClick}
        onStopClick={onStopClick}
      />
    );
  }
};

export default EditableTimer;
