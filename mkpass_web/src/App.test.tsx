import React from 'react';
import { render, screen } from '@testing-library/react';
import App from './App';

test('renders mkpass application title and logo', () => {
  render(<App />);
  const titleElement = screen.getByRole('heading', { name: /mkpass/i });
  expect(titleElement).toBeInTheDocument();

  const logoElement = screen.getByAltText(/mkpass logo/i);
  expect(logoElement).toBeInTheDocument();
});
