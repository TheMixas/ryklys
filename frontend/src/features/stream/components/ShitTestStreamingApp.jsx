import { useEffect, useRef, useState, useCallback } from "react";
import star from "./../../../assets/star.png";
import earth from "./../../../assets/earth.mp4";
// ─────────────────────────────────────────────
// HELPERS
// ─────────────────────────────────────────────
function createVideoFromStream(stream) {
  const v = document.createElement("video");
  v.srcObject = stream;
  v.muted = true;
  v.playsInline = true;
  return v;
}

function createVideoFromFile(src) {
  const v = document.createElement("video");
  v.src = src;
  v.loop = true;
  v.muted = true;
  v.playsInline = true;
  return v;
}

function createImageElement(src) {
  const img = new Image();
  img.src = src;
  img.crossOrigin = "anonymous";
  return img;
}

function hexToRGB(hex) {
  const r = parseInt(hex.slice(1, 3), 16) / 255;
  const g = parseInt(hex.slice(3, 5), 16) / 255;
  const b = parseInt(hex.slice(5, 7), 16) / 255;
  return { r, g, b };
}

// ─────────────────────────────────────────────
// EASING
// ─────────────────────────────────────────────
const Easing = {
  linear: (t) => t,
  easeInQuad: (t) => t * t,
  easeOutQuad: (t) => t * (2 - t),
  easeInOutQuad: (t) => (t < 0.5 ? 2 * t * t : -1 + (4 - 2 * t) * t),
  easeInCubic: (t) => t * t * t,
  easeOutCubic: (t) => --t * t * t + 1,
  easeInOutCubic: (t) =>
    t < 0.5 ? 4 * t * t * t : (t - 1) * (2 * t - 2) * (2 * t - 2) + 1,
  easeOutBounce: (t) => {
    if (t < 1 / 2.75) return 7.5625 * t * t;
    if (t < 2 / 2.75) return 7.5625 * (t -= 1.5 / 2.75) * t + 0.75;
    if (t < 2.5 / 2.75) return 7.5625 * (t -= 2.25 / 2.75) * t + 0.9375;
    return 7.5625 * (t -= 2.625 / 2.75) * t + 0.984375;
  },
};

// ─────────────────────────────────────────────
// DATA STRUCTURES
// ─────────────────────────────────────────────
class Capture {
  constructor() {
    this.position = { x: 0, y: 0 };
    this.resolution = { width: 640, height: 360 };
    this.fallbackBackground = "#1e1e1e";
    this.mediaStream = null;
    this.videoElement = null;
    this.imageElement = null;
    this.isStaticImage = false;
    this.layer = 0;
  }
  setMediaStream(stream) {
    this.mediaStream = stream;
    this.videoElement = createVideoFromStream(stream);
    this.imageElement = null;
    this.isStaticImage = false;
  }
  setVideoFile(src) {
    this.videoElement = createVideoFromFile(src);
    this.imageElement = null;
    this.isStaticImage = false;
  }
  setImage(src) {
    this.imageElement = createImageElement(src);
    this.videoElement = null;
    this.isStaticImage = true;
  }
  getSource() {
    return this.isStaticImage ? this.imageElement : this.videoElement;
  }
  isSourceReady() {
    if (this.isStaticImage)
      return (
        this.imageElement &&
        this.imageElement.complete &&
        this.imageElement.naturalHeight !== 0
      );
    return (
      this.videoElement &&
      this.videoElement.readyState >= this.videoElement.HAVE_CURRENT_DATA
    );
  }
}

class PopupAnimation {
  constructor(property, from, to, duration, easing = "easeOutQuad", delay = 0) {
    this.property = property;
    this.from = from;
    this.to = to;
    this.duration = duration;
    this.easing = easing;
    this.delay = delay;
  }
  getValue(currentTime, startTime) {
    if (currentTime < startTime + this.delay) return this.from;
    const elapsed = currentTime - startTime - this.delay;
    if (elapsed >= this.duration) return this.to;
    const t = elapsed / this.duration;
    const fn = Easing[this.easing] || Easing.linear;
    return this.from + (this.to - this.from) * fn(t);
  }
  isComplete(currentTime, startTime) {
    return currentTime >= startTime + this.delay + this.duration;
  }
}

class PopupElement {
  constructor(config) {
    this.type = config.type;
    this.src = config.src;
    this.text = config.text;
    this.font = config.font || "48px Arial";
    this.color = config.color || "#ffffff";
    this.position = config.position || { x: 0, y: 0 };
    this.size = config.size || { width: 200, height: 200 };
    this.duration = config.duration || 3000;
    this.animations = config.animations || [];
    this.startTime = null;
    this.element = null;
    this.canvas = null;
    this.isReady = false;
    this.currentX = this.position.x;
    this.currentY = this.position.y;
    this.currentScale = 1.0;
    this.currentOpacity = 1.0;
    this.currentRotation = 0;
    this._initialize();
  }
  _initialize() {
    if (this.type === "image") {
      this.element = createImageElement(this.src);
      this.element.onload = () => (this.isSourceReady = true);
    } else if (this.type === "video") {
      this.element = createVideoFromFile(this.src);
      this.element.onloadeddata = () => {
        this.isSourceReady = true;
        this.element.play();
      };
    } else if (this.type === "text") {
      this.canvas = document.createElement("canvas");
      this.canvas.width = this.size.width;
      this.canvas.height = this.size.height;
      const ctx = this.canvas.getContext("2d");
      ctx.font = this.font;
      ctx.fillStyle = this.color;
      ctx.textAlign = "center";
      ctx.textBaseline = "middle";
      ctx.fillText(this.text, this.size.width / 2, this.size.height / 2);
      this.isSourceReady = true;
    }
  }
  start(currentTime) {
    this.startTime = currentTime;
  }
  update(currentTime) {
    if (!this.startTime) return;
    this.currentX = this.position.x;
    this.currentY = this.position.y;
    this.currentScale = 1.0;
    this.currentOpacity = 1.0;
    this.currentRotation = 0;
    for (const anim of this.animations) {
      const value = anim.getValue(currentTime, this.startTime);
      switch (anim.property) {
        case "x": this.currentX = value; break;
        case "y": this.currentY = value; break;
        case "scale": this.currentScale = value; break;
        case "opacity": this.currentOpacity = value; break;
        case "rotation": this.currentRotation = value; break;
      }
    }
  }
  isExpired(currentTime) {
    if (!this.startTime) return false;
    return currentTime - this.startTime >= this.duration;
  }
  getSource() {
    return this.type === "text" ? this.canvas : this.element;
  }
  isSourceReady() {
    return this.isSourceReady && this.getSource();
  }
}

class Popup {
  constructor() {
    this.elements = [];
    this.isActive = false;
    this.startTime = null;
  }
  addElement(el) { this.elements.push(el); }
  trigger(currentTime) {
    this.isActive = true;
    this.startTime = currentTime;
    this.elements.forEach((el) => el.start(currentTime));
  }
  update(currentTime) {
    if (!this.isActive) return;
    this.elements.forEach((el) => el.update(currentTime));
    this.elements = this.elements.filter((el) => !el.isExpired(currentTime));
    if (this.elements.length === 0) this.isActive = false;
  }
  getActiveElements() { return this.isActive ? this.elements : []; }
}

class PopupManager {
  constructor() { this.popups = []; }
  addPopup(p) { this.popups.push(p); }
  triggerPopup(index, currentTime) {
    if (index >= 0 && index < this.popups.length)
      this.popups[index].trigger(currentTime);
  }
  update(currentTime) { this.popups.forEach((p) => p.update(currentTime)); }
  getAllActiveElements() {
    return this.popups.flatMap((p) => p.getActiveElements());
  }
}

class Scene {
  constructor() {
    this.captures = [];
    this.backgroundColor = "#000000";
    this.resolution = { width: 1280, height: 720 };
    this.popupManager = new PopupManager();
  }
  getSortedCaptures() {
    return [...this.captures].sort((a, b) => a.layer - b.layer);
  }
}

// ─────────────────────────────────────────────
// WEBGL SETUP
// ─────────────────────────────────────────────
function createGLCanvas(w, h) {
  const canvas = document.createElement("canvas");
  canvas.width = w;
  canvas.height = h;
  const gl = canvas.getContext("webgl", { alpha: true, premultipliedAlpha: false });
  if (!gl) throw new Error("WebGL not supported");
  return { canvas, gl };
}

function createShader(gl, type, src) {
  const s = gl.createShader(type);
  gl.shaderSource(s, src);
  gl.compileShader(s);
  if (!gl.getShaderParameter(s, gl.COMPILE_STATUS)) throw gl.getShaderInfoLog(s);
  return s;
}

function createProgram(gl, vs, fs) {
  const p = gl.createProgram();
  gl.attachShader(p, vs);
  gl.attachShader(p, fs);
  gl.linkProgram(p);
  if (!gl.getProgramParameter(p, gl.LINK_STATUS)) throw gl.getProgramInfoLog(p);
  return p;
}

function createTexture(gl) {
  const t = gl.createTexture();
  gl.bindTexture(gl.TEXTURE_2D, t);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
  gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
  return t;
}

function updateTexture(gl, tex, source) {
  gl.bindTexture(gl.TEXTURE_2D, tex);
  gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true);
  gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, source);
}

function generateVertexShader() {
  return `
attribute vec2 a_pos;
varying vec2 v_uv;
void main() {
  v_uv = (a_pos + 1.0) * 0.5;
  gl_Position = vec4(a_pos, 0, 1);
}`;
}

function generateFragmentShader(sortedCaptures) {
  const n = sortedCaptures.length;
  let uniforms = "";
  for (let i = 0; i < n; i++) {
    uniforms += `uniform sampler2D u_capture${i};\n`;
    uniforms += `uniform vec3 u_fallback${i};\n`;
    uniforms += `uniform float u_hasVideo${i};\n`;
    uniforms += `uniform vec2 u_pos${i};\n`;
    uniforms += `uniform vec2 u_res${i};\n`;
  }
  uniforms += `uniform vec2 u_sceneRes;\n`;
  uniforms += `uniform vec3 u_bgColor;\n`;

  let logic = "";
  for (let i = n - 1; i >= 0; i--) {
    logic += `
  vec2 cMin${i} = u_pos${i} / u_sceneRes;
  vec2 cMax${i} = (u_pos${i} + u_res${i}) / u_sceneRes;
  if (pixel.x >= cMin${i}.x && pixel.x < cMax${i}.x &&
      pixel.y >= cMin${i}.y && pixel.y < cMax${i}.y) {
    if (u_hasVideo${i} > 0.5) {
      vec2 localUV = (pixel - cMin${i}) / (cMax${i} - cMin${i});
      vec4 tc = texture2D(u_capture${i}, localUV);
      if (tc.a > 0.01) { gl_FragColor = tc; return; }
    } else {
      gl_FragColor = vec4(u_fallback${i}, 1.0); return;
    }
  }`;
  }

  return `
precision mediump float;
${uniforms}
varying vec2 v_uv;
void main() {
  vec2 pixel = v_uv;
  ${logic}
  gl_FragColor = vec4(u_bgColor, 1.0);
}`;
}

// ─────────────────────────────────────────────
// SCENE RENDERER (returns composite 2D canvas)
// ─────────────────────────────────────────────
function setupSceneRenderer(scene) {
  const { canvas, gl } = createGLCanvas(scene.resolution.width, scene.resolution.height);
  gl.enable(gl.BLEND);
  gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);

  const sortedCaptures = scene.getSortedCaptures();

  const vs = createShader(gl, gl.VERTEX_SHADER, generateVertexShader());
  const fs = createShader(gl, gl.FRAGMENT_SHADER, generateFragmentShader(sortedCaptures));
  const program = createProgram(gl, vs, fs);
  gl.useProgram(program);

  const buf = gl.createBuffer();
  gl.bindBuffer(gl.ARRAY_BUFFER, buf);
  gl.bufferData(
    gl.ARRAY_BUFFER,
    new Float32Array([-1, -1, 1, -1, -1, 1, -1, 1, 1, -1, 1, 1]),
    gl.STATIC_DRAW
  );
  const posLoc = gl.getAttribLocation(program, "a_pos");
  gl.enableVertexAttribArray(posLoc);
  gl.vertexAttribPointer(posLoc, 2, gl.FLOAT, false, 0, 0);

  const bg = hexToRGB(scene.backgroundColor);
  gl.uniform3f(gl.getUniformLocation(program, "u_bgColor"), bg.r, bg.g, bg.b);

  const textures = [];
  const textureNeedsUpdate = [];

  for (let i = 0; i < sortedCaptures.length; i++) {
    const cap = sortedCaptures[i];
    const tex = createTexture(gl);
    textures.push(tex);
    textureNeedsUpdate.push(cap.isStaticImage);
    gl.uniform1i(gl.getUniformLocation(program, `u_capture${i}`), i);
    const fb = hexToRGB(cap.fallbackBackground);
    gl.uniform3f(gl.getUniformLocation(program, `u_fallback${i}`), fb.r, fb.g, fb.b);
    gl.uniform2f(gl.getUniformLocation(program, `u_pos${i}`), cap.position.x, cap.position.y);
    gl.uniform2f(gl.getUniformLocation(program, `u_res${i}`), cap.resolution.width, cap.resolution.height);
  }

  gl.uniform2f(
    gl.getUniformLocation(program, "u_sceneRes"),
    scene.resolution.width,
    scene.resolution.height
  );

  const composite2DCanvas = document.createElement("canvas");
  composite2DCanvas.width = scene.resolution.width;
  composite2DCanvas.height = scene.resolution.height;
  const ctx2D = composite2DCanvas.getContext("2d");

  let rafId;
  function render() {
    const now = performance.now();
    for (let i = 0; i < sortedCaptures.length; i++) {
      const cap = sortedCaptures[i];
      const src = cap.getSource();
      if (src && cap.isSourceReady()) {
        gl.uniform1f(gl.getUniformLocation(program, `u_hasVideo${i}`), 1.0);
        if (cap.isStaticImage) {
          if (textureNeedsUpdate[i]) {
            gl.activeTexture(gl.TEXTURE0 + i);
            updateTexture(gl, textures[i], src);
            textureNeedsUpdate[i] = false;
          }
        } else {
          gl.activeTexture(gl.TEXTURE0 + i);
          updateTexture(gl, textures[i], src);
        }
      } else {
        gl.uniform1f(gl.getUniformLocation(program, `u_hasVideo${i}`), 0.0);
      }
    }
    gl.drawArrays(gl.TRIANGLES, 0, 6);

    scene.popupManager.update(now);
    ctx2D.clearRect(0, 0, composite2DCanvas.width, composite2DCanvas.height);
    ctx2D.drawImage(canvas, 0, 0);

    for (const el of scene.popupManager.getAllActiveElements()) {
      if (!el.isSourceReady()) continue;
      ctx2D.save();
      const cx = el.currentX + (el.size.width * el.currentScale) / 2;
      const cy = el.currentY + (el.size.height * el.currentScale) / 2;
      ctx2D.translate(cx, cy);
      ctx2D.rotate(el.currentRotation);
      ctx2D.scale(el.currentScale, el.currentScale);
      ctx2D.globalAlpha = el.currentOpacity;
      ctx2D.drawImage(
        el.getSource(),
        -el.size.width / 2,
        -el.size.height / 2,
        el.size.width,
        el.size.height
      );
      ctx2D.restore();
    }
    rafId = requestAnimationFrame(render);
  }
  render();

  sortedCaptures.forEach((cap) => {
    if (cap.videoElement && !cap.isStaticImage)
      cap.videoElement.play().catch(() => {});
  });

  return {
    compositeCanvas: composite2DCanvas,
    stop: () => cancelAnimationFrame(rafId),
  };
}

// ─────────────────────────────────────────────
// BUILD DEFAULT SCENE
// ─────────────────────────────────────────────
function buildScene() {
  const scene = new Scene();

  const capture1 = new Capture();
  capture1.position = { x: 0, y: 0 };
  capture1.resolution = { width: 640, height: 720 };
  capture1.layer = 0;

  const capture2 = new Capture();
  capture2.position = { x: 640, y: 0 };
  capture2.resolution = { width: 640, height: 720 };
  capture2.layer = 0;

  scene.captures.push(capture1);
  scene.captures.push(capture2);

  // Popup 1 – star image
  const popup1 = new Popup();
  popup1.addElement(
    new PopupElement({
      type: "image",
      src: star,
      position: { x: -200, y: 300 },
      size: { width: 150, height: 150 },
      duration: 3000,
      animations: [
        new PopupAnimation("x", -200, 1130, 2000, "easeOutCubic"),
        new PopupAnimation("scale", 0.5, 1.5, 1000, "easeOutBounce"),
        new PopupAnimation("rotation", 0, Math.PI * 4, 2000, "linear"),
        new PopupAnimation("opacity", 1, 0, 500, "linear", 2500),
      ],
    })
  );

  // Popup 2 – subscribe text
  const popup2 = new Popup();
  popup2.addElement(
    new PopupElement({
      type: "text",
      text: "SUBSCRIBE!",
      font: "72px Arial Bold",
      color: "#ff0000",
      position: { x: 440, y: 100 },
      size: { width: 400, height: 100 },
      duration: 2000,
      animations: [
        new PopupAnimation("scale", 0, 1.2, 500, "easeOutBounce"),
        new PopupAnimation("y", 100, 150, 1000, "easeInOutQuad", 500),
      ],
    })
  );

  scene.popupManager.addPopup();
  scene.popupManager.addPopup();

  return { scene, capture1, capture2 };
}

// ─────────────────────────────────────────────
// REACT COMPONENT
// ─────────────────────────────────────────────
export default function ShitTestStreamingApp() {
  const outputVideoRef = useRef(null);
  const rendererRef = useRef(null);
  const sceneRef = useRef(null);
  const capture1Ref = useRef(null);
  const capture2Ref = useRef(null);

  const [cap1Active, setCap1Active] = useState(false);
  const [cap2Active, setCap2Active] = useState(false);
  const [isLive, setIsLive] = useState(false);
  const [log, setLog] = useState([]);

  const pushLog = (msg) =>
    setLog((prev) => [`[${new Date().toLocaleTimeString()}] ${msg}`, ...prev.slice(0, 19)]);

  // Mount scene once
  useEffect(() => {
    const { scene, capture1, capture2 } = buildScene();
    sceneRef.current = scene;
    capture1Ref.current = capture1;
    capture2Ref.current = capture2;

    // Set a default video for capture 2
    capture2.setVideoFile("earth.mp4");

    const { compositeCanvas, stop } = setupSceneRenderer(scene);
    rendererRef.current = { compositeCanvas, stop };

    const mixedStream = compositeCanvas.captureStream(60);
    if (outputVideoRef.current) {
      outputVideoRef.current.srcObject = mixedStream;
    }
    setIsLive(true);
    pushLog("Scene.ts initialized. Renderer started at 60 fps.");

    // Trigger demo popups
    setTimeout(() => {
      scene.popupManager.triggerPopup(0, performance.now());
      pushLog("Popup 1 triggered (star animation)");
    }, 2000);
    setTimeout(() => {
      scene.popupManager.triggerPopup(1, performance.now());
      pushLog("Popup 2 triggered (SUBSCRIBE text)");
    }, 5000);

    return () => stop();
  }, []);

  const handleAddCapture1 = useCallback(async () => {
    try {
      const stream = await navigator.mediaDevices.getDisplayMedia({
        video: { frameRate: 60 },
        audio: false,
      });
      capture1Ref.current?.setMediaStream(stream);
      setCap1Active(true);
      pushLog("Capture 1: screen share started.");
    } catch (err) {
      pushLog(`Capture 1 error: ${err.message}`);
    }
  }, []);

  const handleAddCapture2 = useCallback(async () => {
    try {
      const stream = await navigator.mediaDevices.getDisplayMedia({
        video: { frameRate: 60 },
        audio: false,
      });
      capture2Ref.current?.setMediaStream(stream);
      setCap2Active(true);
      pushLog("Capture 2: screen share started.");
    } catch (err) {
      pushLog(`Capture 2 error: ${err.message}`);
    }
  }, []);

  const handleTriggerPopup = useCallback((index) => {
    sceneRef.current?.popupManager.triggerPopup(index, performance.now());
    pushLog(`Popup ${index + 1} triggered manually.`);
  }, []);

  return (
    <div style={styles.root}>
      {/* Header */}
      <header style={styles.header}>
        <div style={styles.logo}>
          <span style={styles.logoAccent}>◈</span> STREAMFORGE
        </div>
        <div style={styles.liveBadge}>
          <span style={{ ...styles.liveDot, background: isLive ? "#00ff88" : "#555" }} />
          {isLive ? "LIVE" : "OFFLINE"}
        </div>
      </header>

      <main style={styles.main}>
        {/* Preview */}
        <section style={styles.previewSection}>
          <div style={styles.previewLabel}>OUTPUT PREVIEW</div>
          <div style={styles.videoWrapper}>
            <video
              ref={outputVideoRef}
              autoPlay
              playsInline
              style={styles.video}
            />
            <div style={styles.scanline} />
          </div>
        </section>

        {/* Controls */}
        <aside style={styles.sidebar}>
          <div style={styles.panel}>
            <div style={styles.panelTitle}>CAPTURES</div>
            <ControlButton
              label="CAPTURE 1"
              sublabel="Screen share → left slot"
              active={cap1Active}
              onClick={handleAddCapture1}
            />
            <ControlButton
              label="CAPTURE 2"
              sublabel="Screen share → right slot"
              active={cap2Active}
              onClick={handleAddCapture2}
            />
          </div>

          <div style={styles.panel}>
            <div style={styles.panelTitle}>POPUPS</div>
            <ControlButton
              label="POPUP 1"
              sublabel="Star fly-across animation"
              onClick={() => handleTriggerPopup(0)}
            />
            <ControlButton
              label="POPUP 2"
              sublabel="SUBSCRIBE text bounce"
              onClick={() => handleTriggerPopup(1)}
            />
          </div>

          <div style={styles.panel}>
            <div style={styles.panelTitle}>EVENT LOG</div>
            <div style={styles.log}>
              {log.length === 0 ? (
                <div style={styles.logEmpty}>No events yet…</div>
              ) : (
                log.map((entry, i) => (
                  <div key={i} style={{ ...styles.logEntry, opacity: 1 - i * 0.04 }}>
                    {entry}
                  </div>
                ))
              )}
            </div>
          </div>
        </aside>
      </main>
    </div>
  );
}

// ─────────────────────────────────────────────
// SUB-COMPONENTS
// ─────────────────────────────────────────────
function ControlButton({ label, sublabel, active, onClick }) {
  const [hover, setHover] = useState(false);
  return (
    <button
      onClick={onClick}
      onMouseEnter={() => setHover(true)}
      onMouseLeave={() => setHover(false)}
      style={{
        ...styles.btn,
        ...(hover ? styles.btnHover : {}),
        ...(active ? styles.btnActive : {}),
      }}
    >
      <span style={styles.btnLabel}>{label}</span>
      {sublabel && <span style={styles.btnSub}>{sublabel}</span>}
      {active && <span style={styles.btnDot} />}
    </button>
  );
}

// ─────────────────────────────────────────────
// STYLES
// ─────────────────────────────────────────────
const styles = {
  root: {
    minHeight: "100vh",
    background: "#0a0a0f",
    color: "#e0e0e0",
    fontFamily: "'JetBrains Mono', 'Courier New', monospace",
    display: "flex",
    flexDirection: "column",
  },
  header: {
    display: "flex",
    alignItems: "center",
    justifyContent: "space-between",
    padding: "14px 28px",
    borderBottom: "1px solid #1e1e2e",
    background: "#0d0d15",
  },
  logo: {
    fontSize: "18px",
    fontWeight: 700,
    letterSpacing: "4px",
    color: "#ffffff",
  },
  logoAccent: {
    color: "#00ff88",
    marginRight: "6px",
  },
  liveBadge: {
    display: "flex",
    alignItems: "center",
    gap: "7px",
    fontSize: "11px",
    letterSpacing: "3px",
    fontWeight: 700,
    color: "#aaa",
  },
  liveDot: {
    width: "8px",
    height: "8px",
    borderRadius: "50%",
    boxShadow: "0 0 6px #00ff88",
  },
  main: {
    display: "flex",
    flex: 1,
    gap: "0",
    overflow: "hidden",
  },
  previewSection: {
    flex: 1,
    display: "flex",
    flexDirection: "column",
    padding: "24px",
    gap: "10px",
  },
  previewLabel: {
    fontSize: "10px",
    letterSpacing: "3px",
    color: "#444",
    fontWeight: 700,
  },
  videoWrapper: {
    position: "relative",
    borderRadius: "4px",
    overflow: "hidden",
    border: "1px solid #1e1e2e",
    background: "#000",
    boxShadow: "0 0 40px rgba(0,255,136,0.05)",
  },
  video: {
    width: "100%",
    display: "block",
    maxHeight: "calc(100vh - 140px)",
  },
  scanline: {
    position: "absolute",
    inset: 0,
    background:
      "repeating-linear-gradient(0deg, transparent, transparent 2px, rgba(0,0,0,0.08) 2px, rgba(0,0,0,0.08) 4px)",
    pointerEvents: "none",
  },
  sidebar: {
    width: "280px",
    flexShrink: 0,
    borderLeft: "1px solid #1e1e2e",
    display: "flex",
    flexDirection: "column",
    overflowY: "auto",
    background: "#0d0d15",
  },
  panel: {
    padding: "18px 20px",
    borderBottom: "1px solid #1a1a26",
  },
  panelTitle: {
    fontSize: "9px",
    letterSpacing: "4px",
    color: "#00ff88",
    fontWeight: 700,
    marginBottom: "12px",
  },
  btn: {
    width: "100%",
    background: "transparent",
    border: "1px solid #2a2a3e",
    borderRadius: "3px",
    color: "#ccc",
    padding: "10px 12px",
    cursor: "pointer",
    display: "flex",
    flexDirection: "column",
    alignItems: "flex-start",
    gap: "3px",
    marginBottom: "8px",
    position: "relative",
    transition: "border-color 0.15s, background 0.15s",
    fontFamily: "inherit",
  },
  btnHover: {
    borderColor: "#00ff88",
    background: "rgba(0,255,136,0.04)",
  },
  btnActive: {
    borderColor: "#00ff88",
    background: "rgba(0,255,136,0.06)",
  },
  btnLabel: {
    fontSize: "11px",
    fontWeight: 700,
    letterSpacing: "2px",
    color: "#fff",
  },
  btnSub: {
    fontSize: "9px",
    letterSpacing: "1px",
    color: "#555",
  },
  btnDot: {
    position: "absolute",
    top: "10px",
    right: "10px",
    width: "6px",
    height: "6px",
    borderRadius: "50%",
    background: "#00ff88",
    boxShadow: "0 0 6px #00ff88",
  },
  log: {
    maxHeight: "200px",
    overflowY: "auto",
    display: "flex",
    flexDirection: "column",
    gap: "4px",
  },
  logEmpty: {
    fontSize: "10px",
    color: "#333",
    fontStyle: "italic",
  },
  logEntry: {
    fontSize: "9px",
    color: "#88ffcc",
    letterSpacing: "0.5px",
    lineHeight: 1.5,
    borderLeft: "2px solid #00ff8844",
    paddingLeft: "8px",
  },
};
