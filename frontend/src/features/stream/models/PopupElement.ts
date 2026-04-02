import {type PopupAnimation, PopupAnimationProperty} from "@/features/stream/models/PopupAnimation.ts";

export class PopupElement {
    position: { x: number, y: number } = {x: 0, y: 0};
    resolution: { width: number, height: number } = {width: 200, height: 200};
    durationMs = 5000; // Duration in milliseconds for how long the popup should be displayed
    animations: PopupAnimation[] = [];

    //Runtime state
    startTimeMs: number | null = null; // Time when the popup was shown, used for animation timing
    // htmlElement: HTMLElement | null = null; // The actual HTML element representing the popup, to be created when the popup is shown
    isSourceReady: boolean = false;

    //Animation state
    currentX: number = this.position.x;
    currentY: number = this.position.y;
    currentScale: number = 1;
    currentRotation: number = 0;
    currentOpacity: number = 1;

    constructor(position: { x: number, y: number }, resolution: {
        width: number,
        height: number
    }, durationMs: number, animations: PopupAnimation[]) {
        this.position = position;
        this.resolution = resolution;
        this.durationMs = durationMs;
        this.animations = animations;
    }

    markStarted(currentTimeMs: number) {
        this.startTimeMs = currentTimeMs;
    }

    updateAnimationState(currentTimeMs: number) {
        if (this.startTimeMs === null) return; // Not started yet

        this.currentX = this.position.x;
        this.currentY = this.position.y;
        this.currentScale = 1;
        this.currentRotation = 0;
        this.currentOpacity = 1;

        for (const anim of this.animations) {
            const animValue = anim.getValueAtTime(currentTimeMs, this.startTimeMs);
            switch (anim.property) {
                case PopupAnimationProperty.x:
                    this.currentX = animValue;
                    break;
                case PopupAnimationProperty.y:
                    this.currentY = animValue;
                    break;
                case PopupAnimationProperty.scale:
                    this.currentScale = animValue;
                    break;
                case PopupAnimationProperty.rotation:
                    this.currentRotation = animValue;
                    break;
                case PopupAnimationProperty.opacity:
                    this.currentOpacity = animValue;
                    break;
            }
        }
    }

    isFinished(currentTimeMs: number): boolean {
        if (this.startTimeMs === null) return false;
        return currentTimeMs >= this.startTimeMs + this.durationMs;
    }

    getSourceElement(): HTMLImageElement | HTMLVideoElement | HTMLCanvasElement | null {
        return null; // To be overridden by subclasses to return the actual media element to be rendered
    }

    getIsSourceReady(): boolean {
        return this.isSourceReady;
    }
}

export class ImagePopupElement extends PopupElement {
    imageUrl: string;
    imageHtmlElement: HTMLImageElement;

    constructor(imageUrl: string, position: { x: number, y: number }, resolution: {
        width: number,
        height: number
    }, durationMs: number, animations: PopupAnimation[]) {
        super(position, resolution, durationMs, animations);
        this.imageUrl = imageUrl;

        this.imageHtmlElement = this.createImageElement(imageUrl);
        this.imageHtmlElement.onload = () => {
            this.isSourceReady = true;
        }
    }

    getSourceElement(): HTMLImageElement | HTMLVideoElement | HTMLCanvasElement | null {
        return this.imageHtmlElement;
    }

    createImageElement(imageUrl: string): HTMLImageElement {
        const img = new Image();
        img.src = imageUrl;
        img.crossOrigin = "anonymous"; // Allow CORS for transparency
        return img;
    }
}

export class TextPopupElement extends PopupElement {
    text: string = 'Default Text';
    font: string = '48px Arial';
    colour: string = '#ffffff';
    canvasElement: HTMLCanvasElement | null = null;

    constructor(text: string, font: string, colour: string, position: { x: number, y: number }, resolution: {
        width: number,
        height: number
    }, durationMs: number, animations: PopupAnimation[]) {
        super(position, resolution, durationMs, animations);
        this.text = text;
        this.font = font;
        this.colour = colour;

        this.canvasElement = this.createCanvasElement();
        this.isSourceReady = true;
    }

    getSourceElement(): HTMLImageElement | HTMLVideoElement | HTMLCanvasElement | null {
        return this.canvasElement;
    }


    private createCanvasElement() {
        const canvas = document.createElement('canvas');
        canvas.width = this.resolution.width;
        canvas.height = this.resolution.height;
        const ctx = canvas.getContext('2d');
        if (ctx) {
            ctx.font = this.font;
            ctx.fillStyle = this.colour;
            ctx.textAlign = 'center';
            ctx.textBaseline = 'middle';
            ctx.fillText(this.text, canvas.width / 2, canvas.height / 2);
        } else {
            throw new Error('Could not get canvas context in TextPopupElement');
        }
        return canvas;
    }
}

export class VideoPopupElement extends PopupElement {
    videoUrl: string;
    muted: boolean = true;
    videoHtmlElement: HTMLVideoElement;

    constructor(videoUrl: string, muted: boolean, position: { x: number, y: number }, resolution: {
        width: number,
        height: number
    }, durationMs: number, animations: PopupAnimation[]) {
        super(position, resolution, durationMs, animations);
        this.videoUrl = videoUrl;
        this.muted = muted;

        this.videoHtmlElement = this.createVideoElement(videoUrl);
        this.videoHtmlElement.onloadeddata = () => {
            this.isSourceReady = true;
            this.videoHtmlElement.play();
        }
    }

    getSourceElement(): HTMLImageElement | HTMLVideoElement | HTMLCanvasElement | null {
        return this.videoHtmlElement;
    }

    private createVideoElement(videoUrl: string) {
        const video = document.createElement('video');
        video.src = videoUrl;
        video.crossOrigin = "anonymous";
        video.loop = true;
        video.muted = this.muted;
        video.playsInline = true;
        return video;
    }
}