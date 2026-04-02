import {type VisualCapture, VideoCapture, ImageCapture} from "@/features/stream/models/VisualCapture.ts";
import {PopupManager} from "@/features/stream/models/PopupManager.ts";
import type {AudioCapture} from "@/features/stream/models/AudioCapture.ts";

export interface SceneConfig {
    videoCaptures?: VideoCapture[];
    imageCaptures?: ImageCapture[];
    audioCaptures?: AudioCapture[];
    resolution?: { width: number; height: number };
    backgroundColor?: string;
}
export const hexToRGB = (hex: string): [number, number, number] =>
    hex.match(/\w\w/g)!.map(h => parseInt(h, 16) / 255) as [number, number, number];
export class Scene {
    videoCaptures: VideoCapture[] = [];
    imageCaptures: ImageCapture[] = [];
    audioCaptures: AudioCapture[] = [];

    // Will be seen if the captures dont cover the whole scene.
    backgroundColor: string = '#3e2fc3';

    resolution: { width: number, height: number } = {width: 1280, height: 720};
    popupManager: PopupManager;

    private constructor(config: Required<SceneConfig>) {
        this.videoCaptures = config.videoCaptures;
        this.imageCaptures = config.imageCaptures;
        this.audioCaptures = config.audioCaptures;
        this.resolution = config.resolution;
        this.backgroundColor = config.backgroundColor;
        this.popupManager = new PopupManager();
    }

    static readonly DEFAULTS: Required<SceneConfig> = {
        videoCaptures: [],
        imageCaptures: [],
        audioCaptures: [],
        resolution: {width: 1280, height: 720},
        backgroundColor: '#3e2fc3',
    };

    static create(config: SceneConfig={}): Scene {
        return new Scene({ ...Scene.DEFAULTS, ...config });
    }

    /**
     * Returns video/image captures sorted by their layer, with lower layers first (background) and higher layers last (foreground).
     */
    getSortedVisualCaptures(): VisualCapture[] {
        let captures: VisualCapture[] = [];
        captures = captures.concat(this.videoCaptures);
        captures = captures.concat(this.imageCaptures);
        return captures.sort((a, b) => a.layer - b.layer);
    }

    getAudioCaptureMediaStreams(): MediaStream[] {
        const streams: MediaStream[] = [];
        for (const audioCapture of this.audioCaptures) {
            const stream = audioCapture.getMediaStream();
            if (stream) {
                streams.push(stream);
            }
        }
        return streams;
    }

}
