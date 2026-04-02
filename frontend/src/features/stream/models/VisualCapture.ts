//A video or an image that can be streamed.
//Provides a fallback background color for when the video or image is not ready, and a layer to determine the rendering order of multiple captures.
export abstract class VisualCapture {
    position: { x: number, y: number } = {x: 0, y: 0};
    resolution: { width: number, height: number } = {width: 640, height: 360};
    fallbackBackgroundColor: string = '#1e1e1e';
    layer: number = 0;

    constructor(position: { x: number, y: number }, resolution: { width: number, height: number },
                fallbackBackgroundColor: string, layer: number) {
        this.position = position;
        this.resolution = resolution;
        this.fallbackBackgroundColor = fallbackBackgroundColor;
        this.layer = layer;
    }

    getSourceElement(): HTMLImageElement | HTMLVideoElement | HTMLAudioElement | null {
        return null;
    }

    isSourceReady(): boolean {
        return false;
    }
}

export class VideoCapture extends VisualCapture {
    videoElement: HTMLVideoElement | null = null;

    constructor(videoElement: HTMLVideoElement | null,
                position: { x: number, y: number }, resolution: { width: number, height: number },
                fallbackBackgroundColor: string, layer: number) {
        super(position, resolution, fallbackBackgroundColor, layer);
        this.videoElement = videoElement;
    }

    setVideoElement(videoElement: HTMLVideoElement) {
        this.videoElement = videoElement;
        console.log('Video element set for capture:', videoElement);
    }

    getSourceElement(): HTMLVideoElement | null {
        return this.videoElement;
    }

    isSourceReady(): boolean {
        if (!this.videoElement) return false;
        return this.videoElement.readyState >= this.videoElement.HAVE_CURRENT_DATA;
    }
}

export class ImageCapture extends VisualCapture {
    imageElement: HTMLImageElement | null = null;
    isDirty: boolean = false; // Flag to indicate if the image source has changed and needs to be re-rendered
    constructor(imageElement: HTMLImageElement | null,
                position: { x: number, y: number }, resolution: { width: number, height: number },
                fallbackBackgroundColor: string, layer: number) {
        super(position, resolution, fallbackBackgroundColor, layer);
        this.imageElement = imageElement;
    }

    isSourceReady(): boolean {
        return this.imageElement !== null && this.imageElement.width > 0;
    }

    setImageElement(imageElement: HTMLImageElement) {
        if (this.imageElement) {
            // no blob URL to revoke, just null out
            this.imageElement = null;
        }

        console.log('Image element set for capture:', imageElement);
        this.imageElement = imageElement;
        this.isDirty = true;

    }

    getSourceElement(): HTMLImageElement | null {
        return this.imageElement;
    }
}

