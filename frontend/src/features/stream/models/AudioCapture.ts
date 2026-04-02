export class AudioCapture {
    public stream: MediaStream | undefined;

    // Web Audio API components instead of HTML elements
    private audioContext: AudioContext | undefined;
    private sourceNode: MediaStreamAudioSourceNode | null = null;
    public gainNode: GainNode | undefined;

    constructor(audioContext: AudioContext | null, stream: MediaStream | null) {
        if(audioContext) {
            this.audioContext = audioContext;
            // Create a volume controller (GainNode) for this specific capture
            this.gainNode = this.audioContext.createGain();
            this.gainNode.gain.value = 1.0; // Default volume: 100%
        }
        if (stream) {
            this.setStream(stream);
        }
    }

    setAudioContext(audioContext: AudioContext) {
        this.audioContext = audioContext;
        // Recreate the gain node with the new audio context
        this.gainNode = this.audioContext.createGain();
        this.gainNode.gain.value = 1.0; // Reset to default volume
    }

    // Call after audioContext is set to connect the stream to the gain node
    setStream(stream: MediaStream) {
        this.stream = stream;
        if (this.sourceNode) {
            this.sourceNode.disconnect();
        }

        // Create a new source node from the raw MediaStream
        this.sourceNode = this.audioContext!.createMediaStreamSource(stream);

        // Plug the raw audio into this capture's volume slider
        this.sourceNode.connect(this.gainNode!);
    }

    // Change the volume (e.g., from a slider in your UI)
    // 0.0 is muted, 1.0 is full volume, 2.0 is 200% boost
    public setVolume(volume: number) {
        this.gainNode!.gain.value = volume;
    }

    getMediaStream(): MediaStream | null {
        if (!this.stream) return null;

        return this.stream;
    }
}