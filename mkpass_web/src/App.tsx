import React, { useState, useEffect } from 'react';
import './App.css';
import { MkPassModule } from './wasm';

function App() {
  const [service, setService] = useState('');
  const [masterPassword, setMasterPassword] = useState('');
  const [repeatPassword, setRepeatPassword] = useState('');
  const [password, setPassword] = useState('');
  const [passwordLength, setPasswordLength] = useState(16);
  const [algorithm, setAlgorithm] = useState<number>(1); // Default to Argon2 (1)
  const [charClassesState, setCharClassesState] = useState({
    lowercase: true,
    uppercase: true,
    digits: true,
    symbols: true,
    custom: false,
  });
  const [customChars, setCustomChars] = useState('');

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
          console.log("WASM Module Loaded. Algorithm values:", module.Algorithm);
          setWasmModule(module);
          setAlgorithm(module.Algorithm.Argon2);
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
    if (charClassesState.lowercase) charClasses.push_back(wasmModule.CharacterClass.LOWERCASE);
    if (charClassesState.uppercase) charClasses.push_back(wasmModule.CharacterClass.UPPERCASE);
    if (charClassesState.digits) charClasses.push_back(wasmModule.CharacterClass.DIGITS);
    if (charClassesState.symbols) charClasses.push_back(wasmModule.CharacterClass.SYMBOLS);
    if (charClassesState.custom) charClasses.push_back(wasmModule.CharacterClass.CUSTOM);

    console.log("Generating with params:", {
      masterPassword: masterPassword.length > 0 ? "[REDACTED]" : "EMPTY",
      service,
      charClasses: charClassesState,
      algorithm,
      passwordLength,
      customChars
    });

    try {
      console.log("Calling wasmModule.MkPass...");
      const result = wasmModule.MkPass(masterPassword, service, charClasses, algorithm, passwordLength, customChars);
      console.log("MkPass result obtained");
      setPassword(result);
    } catch (e: any) {
      console.error("WASM MkPass error:", e);
      let errorMsg = e.message || e.toString();
      if (typeof e === 'number') {
          // Emscripten often throws numbers for pointers to error messages
          try {
              errorMsg = wasmModule.getExceptionMessage(e);
          } catch(ex) {
              errorMsg = `WASM error code: ${e}`;
          }
      }
      alert(`Error generating password: ${errorMsg}`);
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
            <label>Algorithm:</label>
            <select
              value={algorithm}
              onChange={(e) => setAlgorithm(Number(e.target.value))}
              className="select-algorithm"
              disabled={!wasmModule}
            >
              {wasmModule ? (
                <>
                  <option value={wasmModule.Algorithm.Argon2}>Argon2</option>
                  <option value={wasmModule.Algorithm.SlowSha512}>SlowSha512</option>
                  <option value={wasmModule.Algorithm.Old}>Old</option>
                </>
              ) : (
                <option value={1}>Loading algorithms...</option>
              )}
            </select>
          </div>

          <div className="character-classes">
            <label>Character Classes:</label>
            <div className="checkbox-grid">
              <label>
                <input
                  type="checkbox"
                  checked={charClassesState.lowercase}
                  disabled={wasmModule ? algorithm === wasmModule.Algorithm.Old : false}
                  onChange={(e) => setCharClassesState({ ...charClassesState, lowercase: e.target.checked })}
                />
                Lower-case
              </label>
              <label>
                <input
                  type="checkbox"
                  checked={charClassesState.uppercase}
                  disabled={wasmModule ? algorithm === wasmModule.Algorithm.Old : false}
                  onChange={(e) => setCharClassesState({ ...charClassesState, uppercase: e.target.checked })}
                />
                Upper-case
              </label>
              <label>
                <input
                  type="checkbox"
                  checked={charClassesState.digits}
                  disabled={wasmModule ? algorithm === wasmModule.Algorithm.Old : false}
                  onChange={(e) => setCharClassesState({ ...charClassesState, digits: e.target.checked })}
                />
                Digits
              </label>
              <label>
                <input
                  type="checkbox"
                  checked={charClassesState.symbols}
                  disabled={wasmModule ? algorithm === wasmModule.Algorithm.Old : false}
                  onChange={(e) => setCharClassesState({ ...charClassesState, symbols: e.target.checked })}
                />
                Symbols
              </label>
              <label>
                <input
                  type="checkbox"
                  checked={charClassesState.custom}
                  disabled={wasmModule ? algorithm === wasmModule.Algorithm.Old : false}
                  onChange={(e) => setCharClassesState({ ...charClassesState, custom: e.target.checked })}
                />
                Custom
              </label>
            </div>
            {charClassesState.custom && (wasmModule ? algorithm !== wasmModule.Algorithm.Old : true) && (
              <input
                type="text"
                placeholder="Custom characters"
                value={customChars}
                onChange={(e) => setCustomChars(e.target.value)}
                className="custom-chars-input"
              />
            )}
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
