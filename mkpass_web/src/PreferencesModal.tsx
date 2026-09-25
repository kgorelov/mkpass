import React, { useState, useEffect } from 'react';
import { MkPassModule } from './wasm';

export const BUILTIN_DEFAULTS: Record<string, string> = {
  algorithm: 'password/argon2',
  length: '16',
  enable_old_algorithm: 'false',
  char_classes: 'lowercase,uppercase,digits,symbols',
  custom_chars: '',
  separator: '',
  passphrase_pattern: '',
  digits: 'false',
  symbols: 'false',
  substitutions: 'false',
  capitalize: 'true',
};

export interface SettingDef {
  key: string;
  name: string;
  section: 'General' | 'Password Options' | 'Passphrase Options';
}

export const SETTINGS_DEFS: SettingDef[] = [
  // General
  { key: 'algorithm', name: 'algorithm', section: 'General' },
  { key: 'length', name: 'length', section: 'General' },
  { key: 'enable_old_algorithm', name: 'enable_old_algorithm', section: 'General' },

  // Password Options
  { key: 'char_classes', name: 'char_classes', section: 'Password Options' },
  { key: 'custom_chars', name: 'custom_chars', section: 'Password Options' },

  // Passphrase Options
  { key: 'separator', name: 'separator', section: 'Passphrase Options' },
  { key: 'passphrase_pattern', name: 'passphrase_pattern', section: 'Passphrase Options' },
  { key: 'digits', name: 'digits', section: 'Passphrase Options' },
  { key: 'symbols', name: 'symbols', section: 'Passphrase Options' },
  { key: 'substitutions', name: 'substitutions', section: 'Passphrase Options' },
  { key: 'capitalize', name: 'capitalize', section: 'Passphrase Options' },
];

export interface PatternOption {
  value: string;
  label: string;
}

export function getPatternDescription(pattern: string): string {
  const map: Record<string, string> = {
    n: 'Noun',
    v: 'Verb',
    a: 'Adj',
    r: 'Adv',
  };
  return pattern
    .split('')
    .map((c) => map[c.toLowerCase()] || c)
    .join(', ');
}

export function parseCharClassesTokens(str: string): {
  lowercase: boolean;
  uppercase: boolean;
  digits: boolean;
  symbols: boolean;
  custom: boolean;
} {
  const tokens = str.split(',').map((s) => s.trim().toLowerCase());
  return {
    lowercase: tokens.includes('lowercase') || tokens.includes('lower') || tokens.includes('1'),
    uppercase: tokens.includes('uppercase') || tokens.includes('upper') || tokens.includes('2'),
    digits: tokens.includes('digits') || tokens.includes('digit') || tokens.includes('3'),
    symbols: tokens.includes('symbols') || tokens.includes('symbol') || tokens.includes('4'),
    custom: tokens.includes('custom') || tokens.includes('5'),
  };
}

export function formatCharClassesTokens(classes: {
  lowercase: boolean;
  uppercase: boolean;
  digits: boolean;
  symbols: boolean;
  custom: boolean;
}): string {
  const res: string[] = [];
  if (classes.lowercase) res.push('lowercase');
  if (classes.uppercase) res.push('uppercase');
  if (classes.digits) res.push('digits');
  if (classes.symbols) res.push('symbols');
  if (classes.custom) res.push('custom');
  return res.join(',');
}

interface PreferencesModalProps {
  isOpen: boolean;
  onClose: () => void;
  onSave: (newConfig: Record<string, string>) => void;
  wasmModule: MkPassModule | null;
  savedConfig: Record<string, string>;
}

export const PreferencesModal: React.FC<PreferencesModalProps> = ({
  isOpen,
  onClose,
  onSave,
  wasmModule,
  savedConfig,
}) => {
  const [enabledMap, setEnabledMap] = useState<Record<string, boolean>>({});
  const [valuesMap, setValuesMap] = useState<Record<string, string>>({});
  const [availablePatterns, setAvailablePatterns] = useState<PatternOption[]>([
    { value: '', label: 'Random' },
  ]);
  const [customSeparators, setCustomSeparators] = useState<string[]>([]);
  const [customPatterns, setCustomPatterns] = useState<string[]>([]);

  // Initialize patterns from wasm
  useEffect(() => {
    const list: PatternOption[] = [{ value: '', label: 'Random' }];
    if (wasmModule) {
      try {
        const maxLen = wasmModule.GetMaxPassphrasePatternLength();
        for (let l = 1; l <= maxLen; l++) {
          const patterns = wasmModule.GetPassphrasePatterns(l);
          for (let i = 0; i < patterns.size(); i++) {
            const p = patterns.get(i);
            const desc = getPatternDescription(p);
            list.push({
              value: p,
              label: `${p} (${desc})`,
            });
          }
          patterns.delete();
        }
      } catch (e) {
        console.error('Failed to load patterns from wasm', e);
      }
    }
    setAvailablePatterns(list);
  }, [wasmModule]);

  // Sync state whenever modal opens or savedConfig changes
  useEffect(() => {
    if (isOpen) {
      const initEnabled: Record<string, boolean> = {};
      const initValues: Record<string, string> = {};
      const extraSeps: string[] = [];
      const extraPats: string[] = [];

      for (const def of SETTINGS_DEFS) {
        const isSet = savedConfig[def.key] !== undefined;
        initEnabled[def.key] = isSet;
        const val = isSet ? savedConfig[def.key] : BUILTIN_DEFAULTS[def.key];
        initValues[def.key] = val;

        if (def.key === 'separator' && val) {
          if (!['', '-', ' ', '/'].includes(val)) {
            extraSeps.push(val);
          }
        }
        if (def.key === 'passphrase_pattern' && val) {
          if (val !== '' && !availablePatterns.some((p) => p.value.toLowerCase() === val.toLowerCase())) {
            extraPats.push(val);
          }
        }
      }

      setEnabledMap(initEnabled);
      setValuesMap(initValues);
      setCustomSeparators(extraSeps);
      setCustomPatterns(extraPats);
    }
  }, [isOpen, savedConfig, availablePatterns]);

  if (!isOpen) return null;

  const handleToggleEnabled = (key: string) => {
    setEnabledMap((prev) => ({
      ...prev,
      [key]: !prev[key],
    }));
  };

  const handleValueChange = (key: string, val: string) => {
    setValuesMap((prev) => {
      const updated = { ...prev, [key]: val };

      // If disabling enable_old_algorithm and algorithm was password/old, revert to argon2
      if (key === 'enable_old_algorithm' && val === 'false' && updated.algorithm === 'password/old') {
        updated.algorithm = 'password/argon2';
      }

      return updated;
    });
  };

  const handleCharClassToggle = (clsKey: 'lowercase' | 'uppercase' | 'digits' | 'symbols' | 'custom') => {
    const current = parseCharClassesTokens(valuesMap['char_classes'] || BUILTIN_DEFAULTS['char_classes']);
    current[clsKey] = !current[clsKey];
    handleValueChange('char_classes', formatCharClassesTokens(current));
  };

  const handleSeparatorSelect = (val: string) => {
    if (val === '__CUSTOM__') {
      const customVal = window.prompt('Enter custom separator string:');
      if (customVal !== null) {
        if (!['', '-', ' ', '/'].includes(customVal) && !customSeparators.includes(customVal)) {
          setCustomSeparators((prev) => [...prev, customVal]);
        }
        handleValueChange('separator', customVal);
      }
    } else {
      handleValueChange('separator', val);
    }
  };

  const handlePatternSelect = (val: string) => {
    if (val === '__CUSTOM__') {
      const customVal = window.prompt('Enter custom passphrase pattern (e.g. navrn):');
      if (customVal !== null) {
        const clean = customVal.trim().toLowerCase();
        if (clean && !availablePatterns.some((p) => p.value === clean) && !customPatterns.includes(clean)) {
          setCustomPatterns((prev) => [...prev, clean]);
        }
        handleValueChange('passphrase_pattern', clean);
      }
    } else {
      handleValueChange('passphrase_pattern', val);
    }
  };

  const handleRestoreDefaults = () => {
    const newEnabled: Record<string, boolean> = {};
    const newValues: Record<string, string> = {};
    for (const def of SETTINGS_DEFS) {
      newEnabled[def.key] = false;
      newValues[def.key] = BUILTIN_DEFAULTS[def.key];
    }
    setEnabledMap(newEnabled);
    setValuesMap(newValues);
  };

  const handleSave = () => {
    if (enabledMap['length']) {
      const len = parseInt(valuesMap['length'], 10);
      if (isNaN(len) || len < 1 || len > 128) {
        alert('Invalid length: must be between 1 and 128');
        return;
      }
    }

    const newConfig: Record<string, string> = {};
    for (const def of SETTINGS_DEFS) {
      if (enabledMap[def.key]) {
        newConfig[def.key] = valuesMap[def.key];
      }
    }

    onSave(newConfig);
    onClose();
  };

  const isOldAlgoAllowed = valuesMap['enable_old_algorithm'] === 'true';
  const charClasses = parseCharClassesTokens(valuesMap['char_classes'] || BUILTIN_DEFAULTS['char_classes']);

  const renderEditor = (def: SettingDef) => {
    const isEnabled = enabledMap[def.key] || false;
    const value = valuesMap[def.key] !== undefined ? valuesMap[def.key] : BUILTIN_DEFAULTS[def.key];

    switch (def.key) {
      case 'algorithm':
        return (
          <select
            value={value}
            disabled={!isEnabled}
            onChange={(e) => handleValueChange(def.key, e.target.value)}
            className="pref-select"
          >
            <option value="password/argon2">Password (Argon2)</option>
            <option value="password/sha512">Password (SHA512 HMAC)</option>
            {isOldAlgoAllowed && <option value="password/old">OldPassword</option>}
            <option value="passphrase/diceware">Passphrase Diceware (Argon2)</option>
            <option value="passphrase/wordnet">Passphrase Wordnet Pattern (Argon2)</option>
          </select>
        );

      case 'length':
        return (
          <input
            type="number"
            min="1"
            max="128"
            value={value}
            disabled={!isEnabled}
            onChange={(e) => handleValueChange(def.key, e.target.value)}
            className="pref-input-number"
          />
        );

      case 'enable_old_algorithm':
      case 'digits':
      case 'symbols':
      case 'substitutions':
      case 'capitalize':
        return (
          <select
            value={value}
            disabled={!isEnabled}
            onChange={(e) => handleValueChange(def.key, e.target.value)}
            className="pref-select"
          >
            <option value="false">false</option>
            <option value="true">true</option>
          </select>
        );

      case 'char_classes':
        return (
          <div className="pref-char-classes-column">
            <label className={!isEnabled ? 'disabled' : ''}>
              <input
                type="checkbox"
                checked={charClasses.lowercase}
                disabled={!isEnabled}
                onChange={() => handleCharClassToggle('lowercase')}
              />
              Lower-case
            </label>
            <label className={!isEnabled ? 'disabled' : ''}>
              <input
                type="checkbox"
                checked={charClasses.uppercase}
                disabled={!isEnabled}
                onChange={() => handleCharClassToggle('uppercase')}
              />
              Upper-case
            </label>
            <label className={!isEnabled ? 'disabled' : ''}>
              <input
                type="checkbox"
                checked={charClasses.digits}
                disabled={!isEnabled}
                onChange={() => handleCharClassToggle('digits')}
              />
              Digits
            </label>
            <label className={!isEnabled ? 'disabled' : ''}>
              <input
                type="checkbox"
                checked={charClasses.symbols}
                disabled={!isEnabled}
                onChange={() => handleCharClassToggle('symbols')}
              />
              Symbols
            </label>
            <label className={!isEnabled ? 'disabled' : ''}>
              <input
                type="checkbox"
                checked={charClasses.custom}
                disabled={!isEnabled}
                onChange={() => handleCharClassToggle('custom')}
              />
              Custom
            </label>
          </div>
        );

      case 'custom_chars':
        return (
          <input
            type="text"
            placeholder="e.g. !@#$%"
            value={value}
            disabled={!isEnabled}
            onChange={(e) => handleValueChange(def.key, e.target.value)}
            className="pref-input-text"
          />
        );

      case 'separator':
        return (
          <select
            value={value}
            disabled={!isEnabled}
            onChange={(e) => handleSeparatorSelect(e.target.value)}
            className="pref-select"
          >
            <option value="">None</option>
            <option value="-">Hyphen (-)</option>
            <option value=" ">Space ( )</option>
            <option value="/">Slash (/)</option>
            {customSeparators.map((s) => (
              <option key={s} value={s}>
                Custom ('{s}')
              </option>
            ))}
            <option value="__CUSTOM__">Custom...</option>
          </select>
        );

      case 'passphrase_pattern':
        return (
          <select
            value={value}
            disabled={!isEnabled}
            onChange={(e) => handlePatternSelect(e.target.value)}
            className="pref-select"
          >
            {availablePatterns.map((p) => (
              <option key={p.value} value={p.value}>
                {p.label}
              </option>
            ))}
            {customPatterns.map((p) => (
              <option key={p} value={p}>
                Custom ({p})
              </option>
            ))}
            <option value="__CUSTOM__">Custom...</option>
          </select>
        );

      default:
        return null;
    }
  };

  const sections: ('General' | 'Password Options' | 'Passphrase Options')[] = [
    'General',
    'Password Options',
    'Passphrase Options',
  ];

  return (
    <div className="modal-overlay">
      <div className="modal-content preferences-modal-content">
        <h2>Preferences</h2>
        <div className="preferences-scroll-container">
          {sections.map((section) => {
            const sectionDefs = SETTINGS_DEFS.filter((d) => d.section === section);
            return (
              <div key={section} className="preferences-section">
                <div className="preferences-section-title">{section}</div>
                <table className="preferences-table">
                  <thead>
                    <tr>
                      <th className="col-enabled">Enabled</th>
                      <th className="col-name">Variable Name</th>
                      <th className="col-value">Default Value</th>
                    </tr>
                  </thead>
                  <tbody>
                    {sectionDefs.map((def) => {
                      const isEnabled = enabledMap[def.key] || false;
                      return (
                        <tr
                          key={def.key}
                          className={isEnabled ? 'row-enabled' : 'row-disabled'}
                        >
                          <td className="col-enabled">
                            <input
                              type="checkbox"
                              aria-label={`Enable ${def.name}`}
                              checked={isEnabled}
                              onChange={() => handleToggleEnabled(def.key)}
                            />
                          </td>
                          <td className="col-name">{def.name}</td>
                          <td className="col-value">{renderEditor(def)}</td>
                        </tr>
                      );
                    })}
                  </tbody>
                </table>
              </div>
            );
          })}
        </div>

        <div className="preferences-actions">
          <button
            type="button"
            className="btn-restore"
            onClick={handleRestoreDefaults}
          >
            Restore Defaults
          </button>
          <div className="preferences-actions-right">
            <button
              type="button"
              className="btn-cancel"
              onClick={onClose}
            >
              Cancel
            </button>
            <button
              type="button"
              className="btn-save"
              onClick={handleSave}
            >
              Save
            </button>
          </div>
        </div>
      </div>
    </div>
  );
};
