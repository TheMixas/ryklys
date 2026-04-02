import type {Popup} from "@/features/stream/models/Popup.ts";
import type {PopupElement} from "@/features/stream/models/PopupElement.ts";

export class PopupManager {
    popups: Popup[] = [];

    constructor() {
    }

    addPopup(popup: Popup): void {
        this.popups.push(popup);
    }

    markPopupStarted(index: number, currentTimeMs: number): void {
        if (index >= 0 && index < this.popups.length) {
            this.popups[index].markElementsStarted(currentTimeMs);
        }
    }

    update(currentTimeMs: number): void {
        this.popups.forEach(popup => {
            if (popup.isActive) {
                popup.update(currentTimeMs);
            }
        });

    }

    getAllActivePopupElements(): PopupElement[] {
        let activeElements: PopupElement[] = [];
        this.popups.forEach(popup => {
            if (popup.isActive) {
                activeElements = activeElements.concat(popup.getActiveElements());
            }
        });
        return activeElements;
    }

}