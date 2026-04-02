import type {PopupElement} from "@/features/stream/models/PopupElement.ts";

export class Popup{
    elements: PopupElement[] = [];
    isActive: boolean = false;

    constructor(elements: PopupElement[]) {
        this.elements = elements;
    }

    addElement(element: PopupElement) {
        this.elements.push(element);
    }

    markElementsStarted(currentTimeMs: number) {
        this.isActive = true;
        this.elements.forEach(element => {
            element.markStarted(currentTimeMs);
        });
    }

    update(currentTimeMs: number) {

        //Update animation state
        this.elements.forEach(element => {
            element.updateAnimationState(currentTimeMs);
        })

        //Remoev elements that have finished their animations
        this.elements = this.elements.filter(element => !element.isFinished(currentTimeMs));

        //If no more elements, mark popup as inactive
        if (this.elements.length === 0) {
            this.isActive = false;
        }
    }

    getActiveElements(): PopupElement[] {
        return this.elements;
    }

}