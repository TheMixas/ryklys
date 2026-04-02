import type {VisualCapture} from "@/features/stream/models/VisualCapture.ts";

export function generateVertexShader() {
    return `
        attribute vec2 a_pos;
        varying vec2 v_uv;
        void main() {
          v_uv = (a_pos + 1.0) * 0.5;
          gl_Position = vec4(a_pos, 0, 1);
        }
        `;
}

export function generateFragmentShader(sortedCaptures: VisualCapture[]) {
    const numCaptures = sortedCaptures.length;

    // Generate uniform declarations
    let uniforms = '';
    for (let i = 0; i < numCaptures; i++) {
        uniforms += `uniform sampler2D u_capture${i};\n`;
        uniforms += `uniform vec3 u_fallback${i};\n`;
        uniforms += `uniform float u_hasVideo${i};\n`;
    }
    uniforms += `uniform vec2 u_sceneRes;\n`;
    uniforms += `uniform vec3 u_bgColor;\n`;

    for (let i = 0; i < numCaptures; i++) {
        uniforms += `uniform vec2 u_pos${i};\n`;
        uniforms += `uniform vec2 u_res${i};\n`;
    }

    // Generate the fragment shader logic in REVERSE order (back to front)
    // Higher layers (later in sorted array) are checked first
    let captureLogic = '';
    for (let i = numCaptures - 1; i >= 0; i--) {
        captureLogic += `
  // Capture ${i} (layer ${sortedCaptures[i].layer})
  vec2 captureMin${i} = u_pos${i} / u_sceneRes;
  vec2 captureMax${i} = (u_pos${i} + u_res${i}) / u_sceneRes;
  
  if (pixel.x >= captureMin${i}.x && pixel.x < captureMax${i}.x &&
      pixel.y >= captureMin${i}.y && pixel.y < captureMax${i}.y) {
    if (u_hasVideo${i} > 0.5) {
      vec2 localUV = (pixel - captureMin${i}) / (captureMax${i} - captureMin${i});
      vec4 texColor = texture2D(u_capture${i}, localUV);
      
      // Only render if alpha is above threshold (skip transparent pixels)
      if (texColor.a > 0.01) {
        gl_FragColor = texColor;
        return;
      }
      // If transparent, continue checking lower layers
    } else {
      gl_FragColor = vec4(u_fallback${i}, 1.0);
      return;
    }
  }
`;
    }

    return `
precision mediump float;
${uniforms}
varying vec2 v_uv;

void main() {
  vec2 pixel = v_uv;
  
  ${captureLogic}
  
  // Default background color
  gl_FragColor = vec4(u_bgColor, 1.0);
}
`;
}

type ShaderType = WebGLRenderingContext['VERTEX_SHADER'] | WebGLRenderingContext['FRAGMENT_SHADER'];

export function createShader(webGLRenderingContext: WebGLRenderingContext, VERTEX_SHADER: ShaderType, VERT: string) {
    const shader: WebGLShader | null = webGLRenderingContext.createShader(VERTEX_SHADER);
    if (!shader) throw new Error("Failed to create vertex shader");
    webGLRenderingContext.shaderSource(shader, VERT);
    webGLRenderingContext.compileShader(shader);
    if (!webGLRenderingContext.getShaderParameter(shader, webGLRenderingContext.COMPILE_STATUS)) {
        const info = webGLRenderingContext.getShaderInfoLog(shader);
        webGLRenderingContext.deleteShader(shader);
        throw new Error("Failed to compile vertex shader: " + info);
    }
    return shader;
}

export function createProgram(webGLRenderingContext: WebGLRenderingContext, vs: WebGLShader, fs: WebGLShader) {
    const program: WebGLProgram | null = webGLRenderingContext.createProgram();
    if (!program) throw new Error("Failed to create WebGL program");
    webGLRenderingContext.attachShader(program, vs);
    webGLRenderingContext.attachShader(program, fs);
    webGLRenderingContext.linkProgram(program);
    if (!webGLRenderingContext.getProgramParameter(program, webGLRenderingContext.LINK_STATUS)) {
        const info = webGLRenderingContext.getProgramInfoLog(program);
        webGLRenderingContext.deleteProgram(program);
        throw new Error("Failed to link WebGL program: " + info);
    }
    return program;
}

export function createTexture(gl: WebGLRenderingContext): WebGLTexture {
    const texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    return texture;
}

export function updateTexture(webGLRenderingContext: WebGLRenderingContext, texture: WebGLTexture, source: HTMLImageElement | HTMLVideoElement) {
    webGLRenderingContext.bindTexture(webGLRenderingContext.TEXTURE_2D, texture);
    webGLRenderingContext.pixelStorei(webGLRenderingContext.UNPACK_FLIP_Y_WEBGL, true)// FLIP y WHEN UPLOADING
    webGLRenderingContext.texImage2D(
        webGLRenderingContext.TEXTURE_2D,
        0,
        webGLRenderingContext.RGBA,
        webGLRenderingContext.RGBA,
        webGLRenderingContext.UNSIGNED_BYTE,
        source // This can be an HTMLImageElement or HTMLVideoElement
    );
}