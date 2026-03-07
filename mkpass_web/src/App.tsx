import React, { useState, useEffect, useRef } from 'react';
import './App.css';
import { MkPassModule, QrCodeData } from './wasm';

const EyeIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor">
    <path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/>
  </svg>
);

const EyeOffIcon = () => (
  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
    <path d="M9.88 9.88a3 3 0 1 0 4.24 4.24"></path>
    <path d="M10.73 5.08A10.43 10.43 0 0 1 12 5c7 0 10 7 10 7a13.16 13.16 0 0 1-1.67 2.68"></path>
    <path d="M6.61 6.61A13.526 13.526 0 0 0 2 12s3 7 10 7a9.76 9.76 0 0 0 5.36-1.65"></path>
    <line x1="2" x2="22" y1="2" y2="22"></line>
  </svg>
);

const CopyIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor">
    <path d="M16 1H4c-1.1 0-2 .9-2 2v14h2V3h12V1zm3 4H8c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h11c1.1 0 2-.9 2-2V7c0-1.1-.9-2-2-2zm0 16H8V7h11v14z"/>
  </svg>
);

const QrIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor">
    <path d="M3 3h8v8H3zm0 10h8v8H3zm10-10h8v8h-8zm0 10h8v8h-8zM5 5v4h4V5zm0 10v4h4v-4zm10-10v4h4V5zm4 14h-2v-2h-2v2h-2v-2h-2v2h2v2h-2v2h2v-2h2v2h2v-2h2z"/>
  </svg>
);

interface QrCodeProps {
  data: QrCodeData;
}

const QrCodeComponent: React.FC<QrCodeProps> = ({ data }) => {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    if (canvasRef.current) {
      const canvas = canvasRef.current;
      const ctx = canvas.getContext('2d');
      if (ctx) {
        const size = data.size;
        const scale = 5;
        canvas.width = size * scale;
        canvas.height = size * scale;
        ctx.fillStyle = 'white';
        ctx.fillRect(0, 0, canvas.width, canvas.height);
        ctx.fillStyle = 'black';
        for (let y = 0; y < size; y++) {
          for (let x = 0; x < size; x++) {
            if (data.data.get(y * size + x)) {
              ctx.fillRect(x * scale, y * scale, scale, scale);
            }
          }
        }
      }
    }
  }, [data]);

  return (
    <div className="qr-container">
      <canvas ref={canvasRef} className="qr-canvas" />
    </div>
  );
};

function App() {
  const [service, setService] = useState('');
  const [masterPassword, setMasterPassword] = useState('');
  const [repeatPassword, setRepeatPassword] = useState('');
  const [password, setPassword] = useState('');
  const [passwordLength, setPasswordLength] = useState(16);
  const [algorithm, setAlgorithm] = useState<number>(1);
  const [charClassesState, setCharClassesState] = useState({
    lowercase: true,
    uppercase: true,
    digits: true,
    symbols: true,
    custom: false,
  });
  const [customChars, setCustomChars] = useState('');

  const [wasmModule, setWasmModule] = useState<MkPassModule | null>(null);
  const [isModalOpen, setIsModalOpen] = useState(false);
  const [isPasswordVisible, setIsPasswordVisible] = useState(false);
  const [isQrCodeVisible, setIsQrCodeVisible] = useState(false);
  const [qrCodeData, setQrCodeData] = useState<QrCodeData | null>(null);

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
      if ((window as any).mkpass_wasm) {
        (window as any).mkpass_wasm().then((module: MkPassModule) => {
          console.log("WASM Module Loaded.");
          setWasmModule(module);
          setAlgorithm(module.Algorithm.Argon2.value);
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

    try {
      const result = wasmModule.MkPass(masterPassword, service, charClasses, algorithm, passwordLength, customChars);
      setPassword(result);
      setIsModalOpen(true);
      setIsPasswordVisible(false);
      setIsQrCodeVisible(false);
      setQrCodeData(null);
    } catch (e: any) {
      console.error("WASM MkPass error:", e);
      let errorMsg = e.message || e.toString();
      if (typeof e === 'number') {
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

  const handleToggleQr = () => {
    if (!isQrCodeVisible && !qrCodeData && wasmModule) {
      const data = wasmModule.GenerateQrCode(password);
      setQrCodeData(data);
    }
    setIsQrCodeVisible(!isQrCodeVisible);
  };

  return (
    <div className="App">
      <header className="App-header">
        <div className="title-container">
          <img src="/logo_mkpass.png" alt="mkpass logo" className="logo" />
          <h1>mkpass</h1>
        </div>
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
                  <option value={wasmModule.Algorithm.Argon2.value}>Argon2</option>
                  <option value={wasmModule.Algorithm.SlowSha512.value}>SlowSha512</option>
                  <option value={wasmModule.Algorithm.Old.value}>Old</option>
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
                  disabled={wasmModule ? algorithm === wasmModule.Algorithm.Old.value : false}
                  onChange={(e) => setCharClassesState({ ...charClassesState, lowercase: e.target.checked })}
                />
                Lower-case
              </label>
              <label>
                <input
                  type="checkbox"
                  checked={charClassesState.uppercase}
                  disabled={wasmModule ? algorithm === wasmModule.Algorithm.Old.value : false}
                  onChange={(e) => setCharClassesState({ ...charClassesState, uppercase: e.target.checked })}
                />
                Upper-case
              </label>
              <label>
                <input
                  type="checkbox"
                  checked={charClassesState.digits}
                  disabled={wasmModule ? algorithm === wasmModule.Algorithm.Old.value : false}
                  onChange={(e) => setCharClassesState({ ...charClassesState, digits: e.target.checked })}
                />
                Digits
              </label>
              <label>
                <input
                  type="checkbox"
                  checked={charClassesState.symbols}
                  disabled={wasmModule ? algorithm === wasmModule.Algorithm.Old.value : false}
                  onChange={(e) => setCharClassesState({ ...charClassesState, symbols: e.target.checked })}
                />
                Symbols
              </label>
              <label>
                <input
                  type="checkbox"
                  checked={charClassesState.custom}
                  disabled={wasmModule ? algorithm === wasmModule.Algorithm.Old.value : false}
                  onChange={(e) => setCharClassesState({ ...charClassesState, custom: e.target.checked })}
                />
                Custom
              </label>
            </div>
            {charClassesState.custom && (wasmModule ? algorithm !== wasmModule.Algorithm.Old.value : true) && (
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
      </header>

      {isModalOpen && (
        <div className="modal-overlay">
          <div className="modal-content">
            <h2>Generated Password</h2>
            <div className="password-container">
              <div className="password-text">
                {isPasswordVisible ? password : '•'.repeat(password.length)}
              </div>
              <button className="icon-button" onClick={() => setIsPasswordVisible(!isPasswordVisible)} title={isPasswordVisible ? "Hide" : "Show"}>
                {isPasswordVisible ? <EyeOffIcon /> : <EyeIcon />}
              </button>
              <button className="icon-button" onClick={handleCopy} title="Copy to clipboard">
                <CopyIcon />
              </button>
            </div>

            {isQrCodeVisible && qrCodeData && (
              <QrCodeComponent data={qrCodeData} />
            )}

            <div className="modal-actions">
              <button onClick={handleToggleQr}>
                <QrIcon /> {isQrCodeVisible ? "Hide QR Code" : "Show QR Code"}
              </button>
              <button className="close-button" onClick={() => setIsModalOpen(false)}>Close</button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

export default App;
