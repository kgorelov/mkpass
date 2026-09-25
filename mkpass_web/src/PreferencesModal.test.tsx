import React from 'react';
import { render, screen, fireEvent } from '@testing-library/react';
import { PreferencesModal, BUILTIN_DEFAULTS, parseCharClassesTokens, formatCharClassesTokens } from './PreferencesModal';

describe('PreferencesModal', () => {
  const defaultProps = {
    isOpen: true,
    onClose: jest.fn(),
    onSave: jest.fn(),
    wasmModule: null,
    savedConfig: {},
  };

  beforeEach(() => {
    jest.clearAllMocks();
  });

  test('does not render when isOpen is false', () => {
    render(<PreferencesModal {...defaultProps} isOpen={false} />);
    expect(screen.queryByText('Preferences')).not.toBeInTheDocument();
  });

  test('renders Preferences dialog title, sections, and 11 variable names', () => {
    render(<PreferencesModal {...defaultProps} />);
    expect(screen.getByRole('heading', { name: 'Preferences' })).toBeInTheDocument();

    expect(screen.getByText('General')).toBeInTheDocument();
    expect(screen.getByText('Password Options')).toBeInTheDocument();
    expect(screen.getByText('Passphrase Options')).toBeInTheDocument();

    const expectedKeys = [
      'algorithm',
      'length',
      'enable_old_algorithm',
      'char_classes',
      'custom_chars',
      'separator',
      'passphrase_pattern',
      'digits',
      'symbols',
      'substitutions',
      'capitalize',
    ];

    for (const key of expectedKeys) {
      expect(screen.getByText(key)).toBeInTheDocument();
    }
  });

  test('character classes has 5 checkboxes in one column', () => {
    render(<PreferencesModal {...defaultProps} />);
    expect(screen.getByLabelText('Lower-case')).toBeInTheDocument();
    expect(screen.getByLabelText('Upper-case')).toBeInTheDocument();
    expect(screen.getByLabelText('Digits')).toBeInTheDocument();
    expect(screen.getByLabelText('Symbols')).toBeInTheDocument();
    expect(screen.getByLabelText('Custom')).toBeInTheDocument();

    // Verify container class
    const container = screen.getByLabelText('Lower-case').closest('.pref-char-classes-column');
    expect(container).toBeInTheDocument();
  });

  test('enabling a row enables its editor widget', () => {
    render(<PreferencesModal {...defaultProps} />);

    const lengthCheckbox = screen.getByLabelText('Enable length');
    expect(lengthCheckbox).not.toBeChecked();

    const lengthInput = screen.getByDisplayValue('16');
    expect(lengthInput).toBeDisabled();

    fireEvent.click(lengthCheckbox);
    expect(lengthCheckbox).toBeChecked();
    expect(lengthInput).not.toBeDisabled();
  });

  test('OldPassword appears when enable_old_algorithm is set to true', () => {
    render(
      <PreferencesModal
        {...defaultProps}
        savedConfig={{ enable_old_algorithm: 'false' }}
      />
    );

    // OldPassword should not be in the algorithm select
    expect(screen.queryByRole('option', { name: 'OldPassword' })).not.toBeInTheDocument();

    // Change enable_old_algorithm to true
    const enableOldCheckbox = screen.getByLabelText('Enable enable_old_algorithm');
    fireEvent.click(enableOldCheckbox);

    const oldAlgoSelects = screen.getAllByRole('combobox');
    // find enable_old_algorithm select
    const enableOldSelect = oldAlgoSelects.find(
      (sel) => sel.closest('tr')?.querySelector('.col-name')?.textContent === 'enable_old_algorithm'
    );
    expect(enableOldSelect).toBeDefined();

    fireEvent.change(enableOldSelect!, { target: { value: 'true' } });

    // Now OldPassword should appear in algorithm options
    expect(screen.getByRole('option', { name: 'OldPassword' })).toBeInTheDocument();
  });

  test('Restore Defaults resets all enabled checkboxes and values', () => {
    render(
      <PreferencesModal
        {...defaultProps}
        savedConfig={{
          algorithm: 'password/sha512',
          length: '24',
          separator: '-',
        }}
      />
    );

    expect(screen.getByLabelText('Enable algorithm')).toBeChecked();
    expect(screen.getByLabelText('Enable length')).toBeChecked();

    const restoreBtn = screen.getByRole('button', { name: 'Restore Defaults' });
    fireEvent.click(restoreBtn);

    expect(screen.getByLabelText('Enable algorithm')).not.toBeChecked();
    expect(screen.getByLabelText('Enable length')).not.toBeChecked();
    expect(screen.getByDisplayValue('16')).toBeInTheDocument();
  });

  test('Save calls onSave with only enabled keys', () => {
    const onSave = jest.fn();
    const onClose = jest.fn();

    render(
      <PreferencesModal
        {...defaultProps}
        onSave={onSave}
        onClose={onClose}
        savedConfig={{
          length: '20',
        }}
      />
    );

    // Enable separator
    fireEvent.click(screen.getByLabelText('Enable separator'));
    const separatorSelect = screen.getAllByRole('combobox').find(
      (sel) => sel.closest('tr')?.querySelector('.col-name')?.textContent === 'separator'
    );
    fireEvent.change(separatorSelect!, { target: { value: '-' } });

    // Save
    fireEvent.click(screen.getByRole('button', { name: 'Save' }));

    expect(onSave).toHaveBeenCalledWith({
      length: '20',
      separator: '-',
    });
    expect(onClose).toHaveBeenCalled();
  });

  test('Cancel button calls onClose without onSave', () => {
    const onSave = jest.fn();
    const onClose = jest.fn();

    render(
      <PreferencesModal
        {...defaultProps}
        onSave={onSave}
        onClose={onClose}
      />
    );

    fireEvent.click(screen.getByRole('button', { name: 'Cancel' }));
    expect(onClose).toHaveBeenCalled();
    expect(onSave).not.toHaveBeenCalled();
  });

  test('parseCharClassesTokens and formatCharClassesTokens work properly', () => {
    const parsed = parseCharClassesTokens('lowercase,digits');
    expect(parsed.lowercase).toBe(true);
    expect(parsed.uppercase).toBe(false);
    expect(parsed.digits).toBe(true);
    expect(parsed.symbols).toBe(false);
    expect(parsed.custom).toBe(false);

    const formatted = formatCharClassesTokens(parsed);
    expect(formatted).toBe('lowercase,digits');
  });
});
