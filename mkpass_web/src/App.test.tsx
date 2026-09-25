import React from 'react';
import { render, screen, fireEvent } from '@testing-library/react';
import App from './App';

test('renders mkpass application title and logo', () => {
  render(<App />);
  const titleElement = screen.getByRole('heading', { name: /mkpass/i });
  expect(titleElement).toBeInTheDocument();

  const logoElement = screen.getByAltText(/mkpass logo/i);
  expect(logoElement).toBeInTheDocument();
});

test('opens Preferences modal when Preferences button is clicked', () => {
  render(<App />);
  const prefButton = screen.getByRole('button', { name: /preferences/i });
  expect(prefButton).toBeInTheDocument();

  expect(screen.queryByRole('heading', { name: 'Preferences' })).not.toBeInTheDocument();
  fireEvent.click(prefButton);
  expect(screen.getByRole('heading', { name: 'Preferences' })).toBeInTheDocument();
});
