import React, { useState, useEffect } from 'react';
import './App.css';
import { MkPassModule } from './wasm';

function App() {
  const [service, setService] = useState('');
  const [masterPassword, setMasterPassword] = useState('');
  const [password, setPassword] = useState('');
  const [passwordLength, setPasswordLength] = useState(16);
  const [wasmModule, setWasmModule] = useState<MkPassModule | null>(null);

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
          <input
            type="text"
            placeholder="Service"
            value={service}
            onChange={(e) => setService(e.target.value)}
          />
          <input
            type="password"
            placeholder="Master Password"
            value={masterPassword}
            onChange={(e) => setMasterPassword(e.target.value)}
          />
          <div className="password-length">
            <label htmlFor="password-length">Password Length: {passwordLength}</label>
            <input
              id="password-length"
              type="range"
              min="8"
              max="64"
              value={passwordLength}
              onChange={(e) => setPasswordLength(parseInt(e.target.value, 10))}
            />
          </div>
          <button onClick={handleGenerate} disabled={!wasmModule}>
            {wasmModule ? "Generate" : "Loading..."}
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
