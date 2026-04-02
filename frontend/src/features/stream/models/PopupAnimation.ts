type EasingFn = (t: number) => number;

const Easing: Record<string, EasingFn> = {
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
// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-expect-error
export enum PopupAnimationProperty {
    x,
    y,
    scale,
    rotation,
    opacity
}

// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-expect-error
export enum EasingFunctionName {
    linear='linear',
    easeInQuad='easeInQuad',
    easeOutQuad='easeOutQuad',
    easeInOutQuad='easeInOutQuad',
    easeInCubic='easeInCubic',
    easeOutCubic='easeOutCubic',
    easeInOutCubic='easeInOutCubic',
    easeOutBounce='easeOutBounce'
}

export class PopupAnimation {
    property: PopupAnimationProperty;
    from: number;
    to: number;
    durationMs: number;
    delayMs: number;
    easingFunctionName: EasingFunctionName;

    constructor(property: PopupAnimationProperty, from: number, to: number, durationMs: number,
                delayMs: number, easingFunction: EasingFunctionName) {
        this.property = property;
        this.from = from;
        this.to = to;
        this.durationMs = durationMs;
        this.delayMs = delayMs;
        this.easingFunctionName = easingFunction;
    }

    getValueAtTime(currentTimeMs: number, startTimeMs:number): number {
        if (currentTimeMs < startTimeMs + this.delayMs) {
            return this.from;
        }

        const elapsedTimeMs = currentTimeMs - startTimeMs - this.delayMs;
        if (elapsedTimeMs >= this.durationMs) {
            return this.to;
        }

        const progress = elapsedTimeMs / this.durationMs;
        const easingFn = Easing[this.easingFunctionName];
        const progressEased = easingFn(progress);

        return this.from + (this.to - this.from) * progressEased;

    }


}