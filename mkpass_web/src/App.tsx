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

const DeleteIcon = () => (
  <svg viewBox="0 0 24 24" fill="currentColor">
    <path d="M6 19c0 1.1.9 2 2 2h8c1.1 0 2-.9 2-2V7H6v12zM19 4h-3.5l-1-1h-5l-1 1H5v2h14V4z"/>
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

interface SavedService {
  service: string;
  algorithm: number;
  length: number;
  charClasses: {
    lowercase: boolean;
    uppercase: boolean;
    digits: boolean;
    symbols: boolean;
    custom: boolean;
  };
  customChars: string;
  separator: string;
  capitalizeWords: boolean;
  pattern: string;
  allowSubstitutions: boolean;
}

const ALGORITHM_NAMES: Record<number, string> = {
  1: "Argon2",
  2: "SHA512 HMAC",
  3: "OldPassword",
  4: "Diceware (Argon2)",
  5: "Wordnet Pattern (Argon2)",
};

function getAlgorithmName(algo: number): string {
  return ALGORITHM_NAMES[algo] || "Unknown";
}

function highlightServiceName(name: string): string {
  let trailingStart = name.length;
  while (trailingStart > 0 && /\s/.test(name[trailingStart - 1])) {
    trailingStart--;
  }

  let html = "";
  for (let i = 0; i < name.length; i++) {
    const c = name[i];
    const code = name.charCodeAt(i);

    if (i >= trailingStart) {
      if (c === ' ') {
        html += "<span class='highlight-trailing-space'>&nbsp;</span>";
      } else if (c === '\t') {
        html += "<span class='highlight-trailing-space'>[TAB]</span>";
      } else if (c === '\r') {
        html += "<span class='highlight-trailing-space'>[CR]</span>";
      } else if (c === '\n') {
        html += "<span class='highlight-trailing-space'>[LF]</span>";
      } else {
        html += `<span class='highlight-trailing-space'>\\x${code.toString(16).toUpperCase().padStart(2, '0')}</span>`;
      }
    } else if (code < 32 || code >= 127) {
      if (c === '\t') {
        html += "<span class='highlight-nonprintable'>[TAB]</span>";
      } else if (c === '\r') {
        html += "<span class='highlight-nonprintable'>[CR]</span>";
      } else if (c === '\n') {
        html += "<span class='highlight-nonprintable'>[LF]</span>";
      } else {
        html += `<span class='highlight-nonprintable'>\\x${code.toString(16).toUpperCase().padStart(2, '0')}</span>`;
      }
    } else {
      if (c === '&') html += '&amp;';
      else if (c === '<') html += '&lt;';
      else if (c === '>') html += '&gt;';
      else if (c === '"') html += '&quot;';
      else if (c === "'") html += '&#39;';
      else html += c;
    }
  }
  return html;
}

function App() {
  const [service, setService] = useState('');
  const [masterPassword, setMasterPassword] = useState('');
  const [repeatPassword, setRepeatPassword] = useState('');
  const [password, setPassword] = useState('');
  const [passwordLength, setPasswordLength] = useState(16);
  const [algorithm, setAlgorithm] = useState<number>(1);
  const [separator, setSeparator] = useState<string>('');
  const [capitalizeWords, setCapitalizeWords] = useState(true);
  const [pattern, setPattern] = useState<string>('');
  const [availablePatterns, setAvailablePatterns] = useState<string[]>([]);
  const [allowSubstitutions, setAllowSubstitutions] = useState(false);
  const [charClassesState, setCharClassesState] = useState({
    lowercase: true,
    uppercase: true,
    digits: true,
    symbols: true,
    custom: false,
  });
  const [customChars, setCustomChars] = useState('');
  const [saveService, setSaveService] = useState(true);
  const [savedServices, setSavedServices] = useState<Record<string, SavedService>>({});

  const [wasmModule, setWasmModule] = useState<MkPassModule | null>(null);
  const [isGenerating, setIsGenerating] = useState(false);
  const [isModalOpen, setIsModalOpen] = useState(false);
  const [isManagementOpen, setIsManagementOpen] = useState(false);
  const [isManualOpen, setIsManualOpen] = useState(false);
  const [isAboutOpen, setIsAboutOpen] = useState(false);
  const [managementFilter, setManagementFilter] = useState('');
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
    if (wasmModule && algorithm === wasmModule.Algorithm.Passphrase_Wordnet_Pattern.value) {
      const patterns = wasmModule.GetPassphrasePatterns(passwordLength);
      const list = [];
      for (let i = 0; i < patterns.size(); i++) {
        list.push(patterns.get(i));
      }
      setAvailablePatterns(list);
      patterns.delete();
    }
  }, [wasmModule, algorithm, passwordLength]);

  // Load saved services on mount
  useEffect(() => {
    const stored = localStorage.getItem('mkpass_services');
    if (stored) {
      try {
        setSavedServices(JSON.parse(stored));
      } catch (e) {
        console.error("Failed to parse saved services", e);
      }
    }
  }, []);

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
          setSeparator('');
          setCapitalizeWords(true);
        });
      }
    };
    document.body.appendChild(script);
    return () => {
      document.body.removeChild(script);
    };
  }, []);

  const handleServiceChange = (value: string) => {
    setService(value);
    if (savedServices[value]) {
      const s = savedServices[value];
      setAlgorithm(s.algorithm);
      setPasswordLength(s.length);
      setCharClassesState(s.charClasses);
      setCustomChars(s.customChars);
      if (s.separator !== undefined) {
        setSeparator(s.separator);
      }
      if (s.capitalizeWords !== undefined) {
        setCapitalizeWords(s.capitalizeWords);
      }
      if (s.pattern !== undefined) {
        setPattern(s.pattern);
      }
      if (s.allowSubstitutions !== undefined) {
        setAllowSubstitutions(s.allowSubstitutions);
      }
    }
  };

  const handleAlgorithmChange = (newAlgo: number) => {
    setAlgorithm(newAlgo);
    if (wasmModule) {
      if (newAlgo === wasmModule.Algorithm.Passphrase_Diceware_EFF_Large.value) {
        setPasswordLength(3);
        setAllowSubstitutions(false);
        setSeparator('');
        setCapitalizeWords(true);
      } else if (newAlgo === wasmModule.Algorithm.Passphrase_Wordnet_Pattern.value) {
        setPattern(''); // Random
        setPasswordLength(3);
        setAllowSubstitutions(false);
        setSeparator('');
        setCapitalizeWords(true);
      } else if (newAlgo === wasmModule.Algorithm.Old.value) {
        setPasswordLength(8);
      } else if (newAlgo === wasmModule.Algorithm.Argon2.value || newAlgo === wasmModule.Algorithm.SlowSha512.value) {
        setPasswordLength(16);
      }
    }
  };

  const isPassphraseAlgo = () => {
    if (!wasmModule) return false;
    return algorithm === wasmModule.Algorithm.Passphrase_Diceware_EFF_Large.value ||
           algorithm === wasmModule.Algorithm.Passphrase_Wordnet_Pattern.value;
  };

  const isOldAlgo = () => {
    if (!wasmModule) return false;
    return algorithm === wasmModule.Algorithm.Old.value;
  };

  const handleGenerate = () => {
    if (!wasmModule) {
      alert("WASM module not loaded yet.");
      return;
    }

    const trimmedService = service.replace(/\s+$/, '');
    setService(trimmedService);

    setIsGenerating(true);

    // Save service settings if enabled
    if (saveService && trimmedService) {
      const newSaved = {
        ...savedServices,
        [trimmedService]: {
          service: trimmedService,
          algorithm,
          length: passwordLength,
          charClasses: charClassesState,
          customChars,
          separator,
          capitalizeWords,
          pattern,
          allowSubstitutions
        }
      };
      setSavedServices(newSaved);
      localStorage.setItem('mkpass_services', JSON.stringify(newSaved));
    }

    setTimeout(() => {
      const charClasses = new wasmModule.VectorCharacterClass();
      if (!isPassphraseAlgo() && !isOldAlgo()) {
        if (charClassesState.lowercase) charClasses.push_back(wasmModule.CharacterClass.LOWERCASE);
        if (charClassesState.uppercase) charClasses.push_back(wasmModule.CharacterClass.UPPERCASE);
        if (charClassesState.digits) charClasses.push_back(wasmModule.CharacterClass.DIGITS);
        if (charClassesState.symbols) charClasses.push_back(wasmModule.CharacterClass.SYMBOLS);
        if (charClassesState.custom) charClasses.push_back(wasmModule.CharacterClass.CUSTOM);
      } else if (isPassphraseAlgo()) {
        if (charClassesState.digits) charClasses.push_back(wasmModule.CharacterClass.DIGITS);
        if (charClassesState.symbols) charClasses.push_back(wasmModule.CharacterClass.SYMBOLS);
      }

      try {
        const result = wasmModule.MkPass(masterPassword, trimmedService, charClasses, algorithm, passwordLength, customChars, separator, capitalizeWords, pattern, allowSubstitutions);
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
        setIsGenerating(false);
      }
    }, 50);
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

  const handleDeleteService = (serviceName: string) => {
    if (window.confirm(`Delete service '${serviceName}'?`)) {
      const newSaved = { ...savedServices };
      delete newSaved[serviceName];
      setSavedServices(newSaved);
      localStorage.setItem('mkpass_services', JSON.stringify(newSaved));
    }
  };

  return (
    <div className="App">
      <header className="App-header">
        <div className="header-menu">
          <button className="menu-btn" onClick={() => setIsManagementOpen(true)}>Database</button>
          <button className="menu-btn" onClick={() => setIsManualOpen(true)}>Manual</button>
          <button className="menu-btn" onClick={() => setIsAboutOpen(true)}>About</button>
        </div>
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
              list="services-list"
              value={service}
              onChange={(e) => handleServiceChange(e.target.value)}
              onBlur={() => setService(service.replace(/\s+$/, ''))}
            />
            <datalist id="services-list">
              {Object.keys(savedServices).map(s => <option key={s} value={s} />)}
            </datalist>
            <label className="save-checkbox">
              <input
                type="checkbox"
                checked={saveService}
                onChange={(e) => setSaveService(e.target.checked)}
              />
              Save settings
            </label>
          </div>

          <div className="input-group">
            <label>Algorithm:</label>
            <select
              value={algorithm}
              onChange={(e) => handleAlgorithmChange(Number(e.target.value))}
              className="select-algorithm"
              disabled={!wasmModule}
            >
              {wasmModule ? (
                <>
                  <option value={wasmModule.Algorithm.Argon2.value}>Password (Argon2)</option>
                  <option value={wasmModule.Algorithm.SlowSha512.value}>Password (SHA512 HMAC)</option>
                  <option value={wasmModule.Algorithm.Old.value}>OldPassword</option>
                  <option value={wasmModule.Algorithm.Passphrase_Diceware_EFF_Large.value}>Passphrase Diceware EFF Large (Argon2)</option>
                  <option value={wasmModule.Algorithm.Passphrase_Wordnet_Pattern.value}>Passphrase Wordnet Pattern (Argon2)</option>
                </>
              ) : (
                <option value={1}>Loading algorithms...</option>
              )}
            </select>
          </div>

          {!isPassphraseAlgo() && !isOldAlgo() && (
            <div className="character-classes">
              <label>Character Classes:</label>
              <div className="checkbox-grid">
                <label>
                  <input
                    type="checkbox"
                    checked={charClassesState.lowercase}
                    onChange={(e) => setCharClassesState({ ...charClassesState, lowercase: e.target.checked })}
                  />
                  Lower-case
                </label>
                <label>
                  <input
                    type="checkbox"
                    checked={charClassesState.uppercase}
                    onChange={(e) => setCharClassesState({ ...charClassesState, uppercase: e.target.checked })}
                  />
                  Upper-case
                </label>
                <label>
                  <input
                    type="checkbox"
                    checked={charClassesState.digits}
                    onChange={(e) => {
                        const val = e.target.checked;
                        setCharClassesState({ ...charClassesState, digits: val });
                        if (!val && !charClassesState.symbols) setAllowSubstitutions(false);
                    }}
                  />
                  Digits
                </label>
                <label>
                  <input
                    type="checkbox"
                    checked={charClassesState.symbols}
                    onChange={(e) => {
                        const val = e.target.checked;
                        setCharClassesState({ ...charClassesState, symbols: val });
                        if (!val && !charClassesState.digits) setAllowSubstitutions(false);
                    }}
                  />
                  Symbols
                </label>
                <label>
                  <input
                    type="checkbox"
                    checked={charClassesState.custom}
                    onChange={(e) => setCharClassesState({ ...charClassesState, custom: e.target.checked })}
                  />
                  Custom
                </label>
              </div>
              {charClassesState.custom && (
                <input
                  type="text"
                  placeholder="Custom characters"
                  value={customChars}
                  onChange={(e) => setCustomChars(e.target.value)}
                  className="custom-chars-input"
                />
              )}
            </div>
          )}

          {isPassphraseAlgo() && wasmModule && (
            <div className="input-group">
              {algorithm === wasmModule.Algorithm.Passphrase_Wordnet_Pattern.value && (
                <>
                  <label>Pattern:</label>
                  <select
                    value={pattern}
                    onChange={(e) => setPattern(e.target.value)}
                    className="select-algorithm"
                  >
                    <option value="">Random</option>
                    {availablePatterns.map(p => (
                        <option key={p} value={p}>{p}</option>
                    ))}
                  </select>
                </>
              )}
              <label>Separator:</label>
              <select
                value={separator}
                onChange={(e) => setSeparator(e.target.value)}
                className="select-algorithm"
              >
                <option value="">None</option>
                <option value="-">Hyphen (-)</option>
                <option value=" ">Space ( )</option>
                <option value="/">Slash (/)</option>
              </select>
              <div className="checkbox-grid">
                <label>
                  <input
                    type="checkbox"
                    checked={charClassesState.digits}
                    onChange={(e) => {
                        const val = e.target.checked;
                        setCharClassesState({ ...charClassesState, digits: val });
                        if (!val && !charClassesState.symbols) setAllowSubstitutions(false);
                    }}
                  />
                  Digits
                </label>
                <label>
                  <input
                    type="checkbox"
                    checked={charClassesState.symbols}
                    onChange={(e) => {
                        const val = e.target.checked;
                        setCharClassesState({ ...charClassesState, symbols: val });
                        if (!val && !charClassesState.digits) setAllowSubstitutions(false);
                    }}
                  />
                  Symbols
                </label>
                <label>
                  <input
                    type="checkbox"
                    checked={capitalizeWords}
                    onChange={(e) => setCapitalizeWords(e.target.checked)}
                  />
                  Capitalize words
                </label>
                <label>
                  <input
                    type="checkbox"
                    checked={allowSubstitutions}
                    disabled={!charClassesState.digits && !charClassesState.symbols}
                    onChange={(e) => setAllowSubstitutions(e.target.checked)}
                  />
                  Allow substitutions
                </label>
              </div>
            </div>
          )}

          <div className="input-group">
            <label>{isPassphraseAlgo() ? 'Number of words' : 'Password Length'}: {passwordLength}</label>
            <input
              type="range"
              min={isPassphraseAlgo() ? "1" : "8"}
              max={algorithm === (wasmModule?.Algorithm.Passphrase_Wordnet_Pattern.value || -1)
                   ? (wasmModule?.GetMaxPassphrasePatternLength() || 6)
                   : (isPassphraseAlgo() ? "20" : "64")}
              value={passwordLength}
              onChange={(e) => setPasswordLength(parseInt(e.target.value, 10))}
            />
          </div>
          <button
            onClick={handleGenerate}
            disabled={!wasmModule || !passwordMatchStatus.isValid || !masterPassword || !service || isGenerating}
          >
            {!wasmModule ? "Loading..." : isGenerating ? "Generating..." : "Generate"}
          </button>
        </div>
      </header>

      {isGenerating && (
        <div className="modal-overlay">
          <div className="loading-content">
            <div className="spinner"></div>
            <p>Generating Password...</p>
          </div>
        </div>
      )}

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

      {isManagementOpen && (
        <div className="modal-overlay">
          <div className="modal-content">
            <h2>Database Management</h2>
            <div className="management-container">
              <input
                type="text"
                placeholder="Filter services..."
                className="search-input"
                value={managementFilter}
                onChange={(e) => setManagementFilter(e.target.value)}
              />
              <div className="service-list">
                {Object.keys(savedServices)
                  .filter(s => s.toLowerCase().includes(managementFilter.toLowerCase()))
                  .map(s => {
                    const info = savedServices[s];
                    return (
                      <div key={s} className="service-item">
                        <div className="service-details">
                          <span className="service-name" dangerouslySetInnerHTML={{ __html: highlightServiceName(s) }} />
                          {info && (
                            <span className="service-params">
                              {getAlgorithmName(info.algorithm)} • Length: {info.length}
                            </span>
                          )}
                        </div>
                        <button className="icon-button delete-btn" onClick={() => handleDeleteService(s)} title="Delete">
                          <DeleteIcon />
                        </button>
                      </div>
                    );
                  })}
                {Object.keys(savedServices).length === 0 && (
                  <div className="no-records">No records found.</div>
                )}
                {Object.keys(savedServices).length > 0 &&
                 Object.keys(savedServices).filter(s => s.toLowerCase().includes(managementFilter.toLowerCase())).length === 0 && (
                  <div className="no-records">No matches found.</div>
                )}
              </div>
            </div>
            <div className="modal-actions">
              <button className="close-button" onClick={() => setIsManagementOpen(false)}>Close</button>
            </div>
          </div>
        </div>
      )}

      {isManualOpen && (
        <div className="modal-overlay">
          <div className="modal-content manual-modal-content">
            <h2>mkpass User Manual</h2>
            <div className="manual-iframe-container">
              <iframe src="/help.html" title="mkpass User Manual" className="manual-iframe" />
            </div>
            <div className="modal-actions">
              <button className="close-button" onClick={() => setIsManualOpen(false)}>Close</button>
            </div>
          </div>
        </div>
      )}

      {isAboutOpen && (
        <div className="modal-overlay">
          <div className="modal-content about-modal-content">
            <img src="/logo_mkpass.png" alt="mkpass logo" className="about-logo" />
            <h2>About mkpass</h2>
            <p className="about-description">
              mkpass - Password generator<br/><br/>
              Written in C++ with WebAssembly and React frontend.
            </p>
            <div className="modal-actions">
              <button className="close-button" onClick={() => setIsAboutOpen(false)}>Close</button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

export default App;
