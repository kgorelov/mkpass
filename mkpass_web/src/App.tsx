import React, { useState, useEffect } from 'react';
import './App.css';
import { MkPassModule } from './wasm';

function App() {
  const [service, setService] = useState('');
  const [masterPassword, setMasterPassword] = useState('');
  const [repeatPassword, setRepeatPassword] = useState('');
  const [password, setPassword] = useState('');
  const [passwordLength, setPasswordLength] = useState(16);
  const [wasmModule, setWasmModule] = useState<MkPassModule | null>(null);

  const [passwordMatchStatus, setPasswordMatchStatus] = useState({
    isValid: true,
    message: '',
    className: '',
    statusClassName: ''
  });

  useEffect(() => {
    if (repeatPassword === '') {
      setPasswordMatchStatus({ isValid: true, message: '', className: '', statusClassName: '' });
      return;
    }

    if (masterPassword === repeatPassword) {
      setPasswordMatchStatus({
        isValid: true,
        message: 'OK: Passwords match.',
        className: 'input-ok',
        statusClassName: 'status-ok'
      });
    } else if (masterPassword.startsWith(repeatPassword)) {
      setPasswordMatchStatus({
        isValid: false,
        message: 'Warning: passwords don\'t match',
        className: '',
        statusClassName: 'status-warning'
      });
    } else {
      setPasswordMatchStatus({
        isValid: false,
        message: 'Error: passwords don\'t match',
        className: 'input-error',
        statusClassName: 'status-error'
      });
    }
  }, [masterPassword, repeatPassword]);

  useEffect(() => {
    const script = document.createElement('script');
    script.src = '/mkpass_webasm.js';
    script.async = true;
    script.onload = () => {
      if (window.mkpass_wasm) {
        window.mkpass_wasm().then((module: MkPassModule) => {
          setWasmModule(module);
        });
      }
    };
    document.body.appendChild(script);
    return () => {
      document.body.removeChild(script);
    };
  }, []);

  const handleGenerate = () => {
    if (!wasmModule) {
      alert("WASM module not loaded yet.");
      return;
    }

    const charClasses = new wasmModule.VectorCharacterClass();
    charClasses.push_back(wasmModule.CharacterClass.LOWERCASE);
    charClasses.push_back(wasmModule.CharacterClass.UPPERCASE);
    charClasses.push_back(wasmModule.CharacterClass.DIGITS);
    charClasses.push_back(wasmModule.CharacterClass.SYMBOLS);

    const ctx = {
      password: masterPassword,
      service: service,
      char_classes: charClasses,
      algorithm: wasmModule.Algorithm.Argon2,
      length: passwordLength,
      custom_chars: "",
    };

    try {
      const result = wasmModule.MkPass(ctx);
      setPassword(result);
    } catch (e) {
      console.error(e);
      alert("Error generating password.");
    } finally {
      charClasses.delete();
    }
  };

  const handleCopy = () => {
    navigator.clipboard.writeText(password);
  };

  return (
    <div className="App">
      <header className="App-header">
        <h1>mkpass</h1>
        <div className="form">
          <div className="input-group">
            <label>Master Password:</label>
            <input
              type="password"
              placeholder="Master Password"
              className={passwordMatchStatus.className}
              value={masterPassword}
              onChange={(e) => setMasterPassword(e.target.value)}
            />
          </div>
          <div className="input-group">
            <label>Repeat Password:</label>
            <input
              type="password"
              placeholder="Repeat Password"
              className={passwordMatchStatus.className}
              value={repeatPassword}
              onChange={(e) => setRepeatPassword(e.target.value)}
            />
          </div>
          <div className={`status-message ${passwordMatchStatus.statusClassName}`}>
            {passwordMatchStatus.message}
          </div>
          <div className="input-group">
            <label>Service:</label>
            <input
              type="text"
              placeholder="Service"
              value={service}
              onChange={(e) => setService(e.target.value)}
            />
          </div>
          <div className="input-group">
            <label>Password Length: {passwordLength}</label>
            <input
              type="range"
              min="8"
              max="64"
              value={passwordLength}
              onChange={(e) => setPasswordLength(parseInt(e.target.value, 10))}
            />
          </div>
          <button
            onClick={handleGenerate}
            disabled={!wasmModule || !passwordMatchStatus.isValid || !masterPassword || !service}
          >
            {!wasmModule ? "Loading..." : "Generate"}
          </button>
        </div>
        {password && (
          <div className="result">
            <p>Generated Password:</p>
            <div className="password-display">
                <code>{password}</code>
            </div>
            <button onClick={handleCopy}>Copy</button>
          </div>
        )}
      </header>
    </div>
  );
}

export default App;
