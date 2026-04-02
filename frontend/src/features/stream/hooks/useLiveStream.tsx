import {type Popup} from "@/features/stream/models/Popup.ts";
import {hexToRGB, Scene, type SceneConfig} from "@/features/stream/models/Scene.ts";
import {useCallback, useEffect, useRef, useState} from "react";
import {ImageCapture, VideoCapture} from "@/features/stream/models/VisualCapture.ts";
import {
    createProgram,
    createShader, createTexture,
    generateFragmentShader,
    generateVertexShader, updateTexture
} from "@/features/stream/utils/StreamHelpers.ts";
import {AudioCapture} from "@/features/stream/models/AudioCapture.ts";

interface UseStreamOptions {
    sceneConfig?: SceneConfig;
}

interface UseStreamReturn {
    // Output
    outputStream: MediaStream | null;

    // Capture controls
    // startScreenCapture: (slot: 0 | 1) => Promise<void>;
    loadScene: (config: SceneConfig) => void;
    videoCaptures: VideoCapture[]; // Expose current video captures so components can render controls for them
    imageCaptures: ImageCapture[]; // Expose current image captures so components can render controls for them
    audioCaptures: AudioCapture[]; // Expose current audio captures so components can render controls for them
    setVideoElement: (slot: number, videoElement: HTMLVideoElement) => void; // Allow setting the video element for a capture slot, which will be used as the source for that capture
    setImageElement: (slot: number, imageElement: HTMLImageElement) => void; // Allow setting the image element for a capture slot, which will be used as the source for that capture
    setAudioStream: (slot: number, stream: MediaStream) => void; // Allow setting the audio stream for a capture slot, which will be used as the source for that capture
    // Popup controls
    // triggerPopup: (index: number) => void;
    addPopup: (popup: Popup) => void;

    // LiveStreaming :))
    goLive: () => Promise<void>;
    stopLive: () => void;
    isLive: boolean;
    pokeWs: (message: string) => void; // For sending custom messages to the backend via WebSocket
}


export function useLiveStream(options: UseStreamOptions): UseStreamReturn {
    const sceneRef = useRef<Scene | null>(null);
    const videoCaptureSlotsRef = useRef<(VideoCapture | null)[]>([null, null]);
    const glCanvasRef = useRef<HTMLCanvasElement | null>(null);
    const glRef = useRef<WebGLRenderingContext | null>(null);
    const rafIdRef = useRef<number | null>(null);
    const [outputStream, setOutputStream] = useState<MediaStream | null>(null);
    const streamRef = useRef<MediaStream | null>(null);
    const pcRef = useRef<RTCPeerConnection | null>(null);
    const wsRef = useRef<WebSocket | null>(null);
    const [isLive, setIsLive] = useState(false);
    const [audioContext, setAudioContext] = useState(new (window.AudioContext || window.webkitAudioContext)());
    const [audioDestination, setAudioDestination] = useState(audioContext.createMediaStreamDestination());

    const [, forceRender] = useState(0); // Add this near your other states

    const earlyCandidatesRef = useRef<RTCIceCandidateInit[]>([]);

    const pokeWs = useCallback((message: string) => {
        if (wsRef.current && wsRef.current.readyState === WebSocket.OPEN) {
            console.log("Sending message to WebSocket:", message);
            wsRef.current.send(JSON.stringify(message));
        } else {
            console.warn("WebSocket is not open. Cannot send message:", message);
        }

    }, []);
    const goLive = useCallback(async () => {
        if (!streamRef.current) {
            console.error("Output stream is not ready yet.");
            return;
        }
        if (audioContext.state === 'suspended') {
            await audioContext.resume();
            console.log("AudioContext resumed!");
        }

        // 2. MIX THE AUDIO NOW
        // Grab all the audio streams currently in the scene
        const activeAudioStreams = sceneRef.current?.getAudioCaptureMediaStreams() ?? [];

        // Mix them and get the single master track
        const mixedAudioTrack = mergeAudioStreams(activeAudioStreams, audioContext, audioDestination);

        // 3. Create a brand new stream specifically for WebRTC
        // We clone the video tracks from the canvas stream, and append our mixed audio track
        const webRtcStream = new MediaStream();

        streamRef.current!.getVideoTracks().forEach(track => webRtcStream.addTrack(track));

        if (mixedAudioTrack) {
            webRtcStream.addTrack(mixedAudioTrack);
        } else {
            console.warn("No audio tracks found. Going live with video only.");
        }

        // 1. Establish the Signaling WebSocket
        // Connect to your Zvejys backend WebSockets route
        const wsUrl = `ws://localhost:8080/sig`;
        const ws = new WebSocket(wsUrl);
        wsRef.current = ws;
        ws.onopen = async () => {
            console.log("Signaling Server connected. Initializing WebRTC...");
            // 2. Initialize the Peer Connection
            const webRtcConfig: RTCConfiguration = {
                iceServers: [
                    {urls: "stun:stun.l.google.com:19302"},
                    {urls: "stun:stun1.l.google.com:19302"},
                ]
            };

            const pc = new RTCPeerConnection(webRtcConfig);
            pcRef.current = pc;

            // // 3. Feed your WebGL Canvas stream tracks to the Peer Connection
            // webRtcStream.getTracks().forEach(track => {
            //     pc.addTrack(track, webRtcStream);
            // });
            // 3. Feed your WebGL Canvas stream tracks to the Peer Connection
            webRtcStream.getTracks().forEach(track => {
                const sender = pc.addTrack(track, webRtcStream);

                if (track.kind === "video") {
                    const transceiver = pc.getTransceivers().find(t => t.sender === sender);
                    if (transceiver) {
                        const codecs = RTCRtpSender.getCapabilities("video")?.codecs ?? [];
                        const h264Codecs = codecs.filter(c =>
                            c.mimeType.toLowerCase() === "video/h264"
                        );
                        if (h264Codecs.length > 0) {
                            transceiver.setCodecPreferences(h264Codecs);
                            console.log("Forced H264 codec preference:", h264Codecs);
                        } else {
                            console.warn("H264 not available in this browser — FFmpeg may fail to probe.");
                        }
                    }
                }
            });

            // 4. Handle ICE Candidates
            // WebRTC will discover network routes (ICE candidates). Send them to the backend.
            pc.onicecandidate = (event) => {
                console.log("New ICE candidate discovered:", event.candidate + "Sending to backend...");
                if (event.candidate) {
                    wsRef.current!.send(JSON.stringify({
                        type: "candidate",
                        candidate: event.candidate
                    }));
                }
            };

            // 5. Create the SDP Offer and send it to the backend
            try {
                const offer = await pc.createOffer();
                await pc.setLocalDescription(offer);

                wsRef.current!.send(JSON.stringify({
                    type: "offer",
                    sdp: pc.localDescription?.sdp
                }));
                console.log("WebRTC offer created and sent to backend:", offer);
            } catch (error) {
                console.error("Failed to create WebRTC offer. Error:", error);
            }

            // FORCE KEYFRAMES from the client side
            // This is necessary because the server can't send PLI without a media handler
            const videoSender = pc.getSenders().find(s => s.track?.kind === 'video');
            if (videoSender) {
                const params = videoSender.getParameters();
                if (!params.encodings || params.encodings.length === 0) {
                    params.encodings = [{}];
                }
                params.encodings[0].maxBitrate = 2_500_000; // 2.5 Mbps cap
                params.encodings[0].maxFramerate = 30;       // 30fps is plenty for HLS
                await videoSender.setParameters(params);
                console.log("Capped video encoder bitrate to 2.5 Mbps");

                const keyframeInterval = setInterval(async () => {
                    try {
                        const videoSender = pc.getSenders().find(s => s.track?.kind === 'video');
                        if (videoSender && typeof videoSender.generateKeyFrame === 'function') {
                            await videoSender.generateKeyFrame();
                        }
                    } catch (e) {
                        console.warn("Failed to generate keyframe:", e);
                    }
                }, 2000); // Every 2 seconds — ensures HLS can cut storage at ~4s

                // Clean up on close

                pc.onconnectionstatechange = () => {
                    if (pc.connectionState === 'disconnected' ||
                        pc.connectionState === 'failed' ||
                        pc.connectionState === 'closed') {
                        clearInterval(keyframeInterval);
                    }
                };

            }

        };

        // 6. Handle Incoming Signaling Messages from Backend
        ws.onmessage = async (event) => {
            console.log("Received signaling message from backend:", event.data);
            const data = JSON.parse(event.data);
            const pc = pcRef.current;
            if (!pc) return;

            try {
                if (data.type === "answer") {
                    await pc.setRemoteDescription(new RTCSessionDescription({
                        type: "answer",
                        sdp: data.sdp
                    }));
                    // Process any queued candidates immediately after the description is set
                    for (const candidate of earlyCandidatesRef.current) {
                        await pc.addIceCandidate(new RTCIceCandidate(candidate));
                        console.log("Added queued ICE candidate after setting remote description:", candidate);
                    }
                    earlyCandidatesRef.current = []; // clear the queue
                    setIsLive(true);
                    console.log("WebRTC connection established!");

                } else if (data.type === "candidate") { // Changed to match C++
                    // Properly construct the RTCIceCandidate using the object format
                    const cleanCandidate: string = data.candidate.replace(/^a=/, "");
                    const rtcIceCandidateInit: RTCIceCandidate = {
                        candidate: cleanCandidate,
                        sdpMid: data.mid,
                        sdpMLineIndex: 0
                    };
                    if (!pc || !pc.remoteDescription?.type) {
                        console.warn("Received ICE candidate before remote description is set. Ignoring candidate:", data.candidate);
                        earlyCandidatesRef.current.push(rtcIceCandidateInit);
                        return;
                    }


                    console.log("Adding ICE candidate to PeerConnection:", rtcIceCandidateInit);
                    await pc.addIceCandidate(new RTCIceCandidate(rtcIceCandidateInit));
                }
            } catch (err) {
                console.error("!Encountered an issue when dealing with a WS backend message :", err);
            }
        };

        ws.onclose = () => {
            console.log("Signaling WebSocket disconnected.");
            //stopLive();
        };

        ws.onerror = (error) => {
            console.error("WebSocket signaling error:", error);
            //();
        };

    }, []);
    const setVideoElement = useCallback((slot: number, videoElement: HTMLVideoElement) => {
        if (!sceneRef.current) {
            console.error("Scene not loaded yet. Cannot set video element.");
            return;
        }
        if (slot < 0 || slot > sceneRef.current.videoCaptures.length - 1) {
            console.error("Invalid slot number. Must be between 0 and " + (sceneRef.current.videoCaptures.length - 1));
            return;
        }
        const capture = sceneRef.current.videoCaptures[slot];
        capture.setVideoElement(videoElement);


        //Split the audio tracks and create audioCaptures for them
        const audioTracks = videoElement.srcObject instanceof MediaStream ? videoElement.srcObject.getAudioTracks() : [];
        if (audioTracks.length > 0) {
            audioTracks.forEach((track) => {
                const stream = new MediaStream([track]);
                const audioCapture = new AudioCapture(audioContext, stream); // Make sure your AudioCapture constructor accepts these args!
                sceneRef.current!.audioCaptures.push(audioCapture);
            });
            forceRender(x => x + 1); // <--- Wakes up React to draw the new audio sliders!
        } else {
            console.log(`Video element in slot ${slot} has no audio tracks.`);
        }
    }, [audioContext]);

    const setAudioStream = useCallback((slot: number, stream: MediaStream) => {
        if (!sceneRef.current) {
            console.error("Scene not loaded yet. Cannot set audio stream.");
            return;
        }
        if (slot < 0 || slot > sceneRef.current.audioCaptures.length - 1) {
            console.error("Invalid slot number. Must be between 0 and " + (sceneRef.current.audioCaptures.length - 1));
            return;
        }
        const capture = sceneRef.current.audioCaptures[slot];
        capture.setAudioContext(audioContext); // Ensure the capture has the current audio context
        capture.setStream(stream);
    }, [audioContext]);

    const setImageElement = useCallback((slot: number, imageElement: HTMLImageElement) => {
        if (!sceneRef.current) {
            console.error("Scene not loaded yet. Cannot set image element.");
            return;
        }
        if (slot < 0 || slot > sceneRef.current.imageCaptures.length - 1) {
            console.error("Invalid slot number. Must be between 0 and " + (sceneRef.current.imageCaptures.length - 1));
            return;
        }
        const capture = sceneRef.current.imageCaptures[slot];
        capture.setImageElement(imageElement);
    }, []);
    const setupSceneRenderer = useCallback((scene: Scene, canvas: HTMLCanvasElement, webGLRenderingContext: WebGLRenderingContext): CanvasRenderingContext2D => {
        // Enable alpha blending for transparency
        webGLRenderingContext.enable(webGLRenderingContext.BLEND)
        webGLRenderingContext.blendFunc(webGLRenderingContext.SRC_ALPHA, webGLRenderingContext.ONE_MINUS_SRC_ALPHA);

        // Sort captures by layer
        const sortedCaptures = scene.getSortedVisualCaptures();
        console.log("Sorted captures:", sortedCaptures);
        // Create a map from sorted index to original capture
        const captureIndexMap = new Map();
        sortedCaptures.forEach((capture, index) => {
            captureIndexMap.set(capture, index);
        });

        // Generate shaders based on sorted captures
        const VERT = generateVertexShader();
        const FRAG = generateFragmentShader(sortedCaptures);

        const vs = createShader(webGLRenderingContext, webGLRenderingContext.VERTEX_SHADER, VERT);
        const fs = createShader(webGLRenderingContext, webGLRenderingContext.FRAGMENT_SHADER, FRAG);
        const program = createProgram(webGLRenderingContext, vs, fs);
        webGLRenderingContext.useProgram(program);

        // Fullscreen quad
        const buf = webGLRenderingContext.createBuffer();
        webGLRenderingContext.bindBuffer(webGLRenderingContext.ARRAY_BUFFER, buf);
        webGLRenderingContext.bufferData(
            webGLRenderingContext.ARRAY_BUFFER,
            new Float32Array([
                -1, -1,
                1, -1,
                -1, 1,
                -1, 1,
                1, -1,
                1, 1
            ]),
            webGLRenderingContext.STATIC_DRAW
        );

        const posLoc = webGLRenderingContext.getAttribLocation(program, "a_pos");
        webGLRenderingContext.enableVertexAttribArray(posLoc);
        webGLRenderingContext.vertexAttribPointer(posLoc, 2, webGLRenderingContext.FLOAT, false, 0, 0);

        // Set scene background color
        webGLRenderingContext.uniform3f(
            webGLRenderingContext.getUniformLocation(program, "u_bgColor"),
            ...hexToRGB(scene.backgroundColor)
        );

        const textures: WebGLTexture[] = [];

        for (let i = 0; i < sortedCaptures.length; i++) {
            const capture = sortedCaptures[i];

            const tex = createTexture(webGLRenderingContext);
            textures.push(tex);

            // Bind texture unit
            webGLRenderingContext.uniform1i(webGLRenderingContext.getUniformLocation(program, `u_capture${i}`), i);

            // Set fallback background color
            webGLRenderingContext.uniform3f(
                webGLRenderingContext.getUniformLocation(program, `u_fallback${i}`),
                ...hexToRGB(capture.fallbackBackgroundColor)
            );

            // Set position and resolution uniforms
            webGLRenderingContext.uniform2f(
                webGLRenderingContext.getUniformLocation(program, `u_pos${i}`),
                capture.position.x,
                capture.position.y
            );
            webGLRenderingContext.uniform2f(
                webGLRenderingContext.getUniformLocation(program, `u_res${i}`),
                capture.resolution.width,
                capture.resolution.height
            );
        }

        //Set scene resolution uniform
        webGLRenderingContext.uniform2f(
            webGLRenderingContext.getUniformLocation(program, `u_sceneRes`),
            scene.resolution.width,
            scene.resolution.height
        );

        //Create a 2d canvas for composing popups on top of we
        const composite2DCanvas = document.createElement('canvas');
        composite2DCanvas.width = scene.resolution.width;
        composite2DCanvas.height = scene.resolution.height;
        const compositeCtx = composite2DCanvas.getContext('2d')!;

        function render() {
            const currentTime = performance.now();

            // Update all capture textures (in sorted order)
            for (let i = 0; i < sortedCaptures.length; i++) {
                const capture = sortedCaptures[i];
                const source = capture.getSourceElement();

                if (source && capture.isSourceReady()) {
                    // Source is ready
                    webGLRenderingContext.uniform1f(webGLRenderingContext.getUniformLocation(program, `u_hasVideo${i}`), 1.0);

                    // For static images, only upload once
                    if (capture instanceof ImageCapture) {
                        if (capture.isDirty) {
                            webGLRenderingContext.activeTexture(webGLRenderingContext.TEXTURE0 + i);
                            updateTexture(webGLRenderingContext, textures[i], source);
                            capture.isDirty = false;
                        }
                    } else if (capture instanceof VideoCapture) {
                        // For videos, update every frame
                        webGLRenderingContext.activeTexture(webGLRenderingContext.TEXTURE0 + i);
                        updateTexture(webGLRenderingContext, textures[i], source);
                    }
                } else {
                    // No source or not ready yet - use fallback color
                    webGLRenderingContext.uniform1f(webGLRenderingContext.getUniformLocation(program, `u_hasVideo${i}`), 0.0);
                }
            }

            // Draw WebGL scene
            webGLRenderingContext.drawArrays(webGLRenderingContext.TRIANGLES, 0, 6);

            // Update popups
            scene.popupManager.update(currentTime);

            // Clear 2D canvas
            compositeCtx.clearRect(0, 0, composite2DCanvas.width, composite2DCanvas.height);

            // Draw WebGL canvas to 2D canvas
            compositeCtx.drawImage(canvas, 0, 0);

            // Draw popups on top
            const activeElements = scene.popupManager.getAllActivePopupElements();
            for (const element of activeElements) {
                if (!element.getIsSourceReady()) continue;

                compositeCtx.save();

                // Apply transformations
                const centerX = element.currentX + (element.resolution.width * element.currentScale) / 2;
                const centerY = element.currentY + (element.resolution.height * element.currentScale) / 2;

                compositeCtx.translate(centerX, centerY);
                compositeCtx.rotate(element.currentRotation);
                compositeCtx.scale(element.currentScale, element.currentScale);
                compositeCtx.globalAlpha = element.currentOpacity;

                // Draw the element
                const source = element.getSourceElement();
                if (source != null) {
                    compositeCtx.drawImage(
                        source,
                        -element.resolution.width / 2,
                        -element.resolution.height / 2,
                        element.resolution.width,
                        element.resolution.height
                    );
                }


                compositeCtx.restore();
            }
            compositeCtx.fillStyle = `rgba(${Math.random() * 255}, 0, 0, 0.01)`;
            compositeCtx.fillRect(0, 0, 1, 1);
            //The Bouncing Rainbow Square
            // This forces massive pixel entropy, guaranteeing VP8 sends continuous
            // data and regular keyframes so FFmpeg can lock onto the resolution!
            // compositeCtx.fillStyle = `hsl(${(currentTime / 10) % 360}, 100%, 50%)`;
            // compositeCtx.fillRect(
            //     (currentTime / 5) % composite2DCanvas.width,
            //     (currentTime / 7) % composite2DCanvas.height,
            //     150,
            //     150
            // );
            requestAnimationFrame(render);
        }

        render();

        return compositeCtx;
    }, []);
    const loadScene = useCallback((config: SceneConfig) => {
        // Stop existing RAF loop before swapping
        if (rafIdRef.current) {
            cancelAnimationFrame(rafIdRef.current);
        }

        const scene = Scene.create(config);
        sceneRef.current = scene;
        videoCaptureSlotsRef.current = scene.videoCaptures ? [...scene.videoCaptures] : [null, null];

        // Re-run your renderer setup with the new scene
        const compositeMF = setupSceneRenderer(scene, glCanvasRef.current!, glRef.current!);
        // const mixedAudioTrack = mergeAudioStreams(scene.getAudioCaptureMediaStreams(), audioContext, audioDestination);
        // const finalOutputStream = glCanvasRef.current!.captureStream(60);
        const finalOutputStream = compositeMF.canvas.captureStream(30);

        // if (mixedAudioTrack) {
        //     finalOutputStream.addTrack(mixedAudioTrack);
        // } else {
        //     console.warn("No audio tracks found in the scene. Output stream will be video-only.");
        // }
        setOutputStream(finalOutputStream);
        streamRef.current = finalOutputStream;

    }, [audioContext, audioDestination, setupSceneRenderer]);

    useEffect(() => {
        const canvas = document.createElement('canvas');

        canvas.width = options.sceneConfig?.resolution?.width ?? 1280;
        canvas.height = options.sceneConfig?.resolution?.height ?? 720;

        const gl = canvas.getContext("webgl", {alpha: true, premultipliedAlpha: false});
        if (!gl) throw new Error("WebGL not supported");

        glCanvasRef.current = canvas;
        glRef.current = gl;

        // Load initial scene on mount
        loadScene(options.sceneConfig ?? {});

        return () => {

        }
    }, [loadScene, options.sceneConfig]);

    /**
     * Merges the audio tracks from multiple MediaStreams into a single Audio Track.
     * @param {MediaStream[]} streams - An array of MediaStreams (e.g., mic, desktop audio)
     * @param audioContext
     * @param audioDestination
     * @returns {MediaStreamTrack} - The single, mixed audio track
     */
    function mergeAudioStreams(streams: MediaStream[], audioContext: AudioContext, audioDestination: MediaStreamAudioDestinationNode): MediaStreamTrack {

        // 3. Loop through every stream provided
        streams.forEach(stream => {
            // Only process streams that actually have audio
            if (stream.getAudioTracks().length > 0) {
                const source = audioContext.createMediaStreamSource(stream);
                source.connect(audioDestination);
            }
        });

        // 4. Return the single mixed audio track from the destination
        return audioDestination.stream.getAudioTracks()[0];
    }

    return {
        addPopup(): void {
        }, isLive,
        videoCaptures: sceneRef.current?.videoCaptures ?? [],
        imageCaptures: sceneRef.current?.imageCaptures ?? [],
        audioCaptures: sceneRef.current?.audioCaptures ?? [],
        goLive,
        pokeWs,
        setImageElement,
        setVideoElement,
        setAudioStream,
        loadScene, // ✅ expose it so components can swap scenes
        outputStream
    };
}