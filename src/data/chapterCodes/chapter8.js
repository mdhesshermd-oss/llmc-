export default `import { render, screen, fireEvent } from '@testing-library/react';
import { describe, it, expect } from 'vitest';
import Counter from './Counter';

describe('Counter', () => {
  it('должен отображать начальное значение 0', () => {
    render(<Counter />);
    const countElement = screen.getByTestId('count');
    expect(countElement.textContent).toBe('0');
  });

  it('должен увеличивать значение при нажатии на кнопку', () => {
    render(<Counter />);
    const button = screen.getByTestId('increment-button');
    const countElement = screen.getByTestId('count');

    fireEvent.click(button);

    expect(countElement.textContent).toBe('1');
  });
});`;
